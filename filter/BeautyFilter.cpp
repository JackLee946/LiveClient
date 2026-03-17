#include "stdafx.h"
#include "BeautyFilter.h"

#include <obs.h>

#include <string>
#include <mutex>

// ---------------------------------------------------------------------------
// Helpers: get executable directory for loading model files
// ---------------------------------------------------------------------------
static std::string GetExeDir()
{
    TCHAR buf[MAX_PATH];
    GetModuleFileName(NULL, buf, MAX_PATH);
    std::wstring ws(buf);
    size_t pos = ws.rfind(L'\\');
    if (pos != std::wstring::npos)
        ws = ws.substr(0, pos);
    // Convert wstring to UTF-8
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &result[0], len, nullptr, nullptr);
    return result;
}

// ---------------------------------------------------------------------------
// Filter context
// ---------------------------------------------------------------------------
struct BeautyFilterContext
{
    obs_source_t* source;          // the filter source itself

    // Parameters
    bool    enabled;
    int     smoothLevel;           // 0-10
    int     whiteLevel;            // 0-10

    // Face detection
    cv::Ptr<cv::FaceDetectorYN> faceDetector;
    std::string modelPath;         // path to face detection model
    cv::Rect lastFaceRect;
    bool     hasFace;
    uint64_t lastDetectTimeMs;
    int      detectIntervalMs;

    // Pre-allocated buffers
    cv::Mat bgrBuf;
    int     lastWidth;
    int     lastHeight;

    // Thread safety for parameter updates
    std::mutex paramMutex;
};

// ---------------------------------------------------------------------------
// Color space conversion: NV12 -> BGR
// ---------------------------------------------------------------------------
static void NV12ToBGR(const struct obs_source_frame* frame, cv::Mat& bgr)
{
    int w = frame->width;
    int h = frame->height;

    // NV12: Y plane full size, UV plane half height interleaved
    cv::Mat yuv(h + h / 2, w, CV_8UC1);

    // Copy Y plane
    for (int y = 0; y < h; y++)
        memcpy(yuv.ptr(y), frame->data[0] + y * frame->linesize[0], w);

    // Copy UV plane
    for (int y = 0; y < h / 2; y++)
        memcpy(yuv.ptr(h + y), frame->data[1] + y * frame->linesize[1], w);

    cv::cvtColor(yuv, bgr, cv::COLOR_YUV2BGR_NV12);
}

// ---------------------------------------------------------------------------
// Color space conversion: I420 -> BGR
// ---------------------------------------------------------------------------
static void I420ToBGR(const struct obs_source_frame* frame, cv::Mat& bgr)
{
    int w = frame->width;
    int h = frame->height;

    cv::Mat yuv(h + h / 2, w, CV_8UC1);

    // Y plane
    for (int y = 0; y < h; y++)
        memcpy(yuv.ptr(y), frame->data[0] + y * frame->linesize[0], w);

    // U plane (quarter size)
    int halfW = w / 2;
    int halfH = h / 2;
    // Pack U and V into planar format for I420
    for (int y = 0; y < halfH; y++)
        memcpy(yuv.ptr(h + y), frame->data[1] + y * frame->linesize[1], halfW);

    for (int y = 0; y < halfH; y++)
        memcpy(yuv.ptr(h + halfH + y), frame->data[2] + y * frame->linesize[2], halfW);

    cv::cvtColor(yuv, bgr, cv::COLOR_YUV2BGR_I420);
}

// ---------------------------------------------------------------------------
// Color space conversion: BGR -> NV12 (write back to frame)
// ---------------------------------------------------------------------------
static void BGRToNV12(const cv::Mat& bgr, struct obs_source_frame* frame)
{
    int w = frame->width;
    int h = frame->height;

    cv::Mat yuv;
    cv::cvtColor(bgr, yuv, cv::COLOR_BGR2YUV_I420);

    // Copy Y plane
    for (int y = 0; y < h; y++)
        memcpy(frame->data[0] + y * frame->linesize[0], yuv.ptr(y), w);

    // Convert I420 UV to NV12 interleaved UV
    int halfW = w / 2;
    int halfH = h / 2;
    const uint8_t* uPlane = yuv.ptr(h);
    const uint8_t* vPlane = yuv.ptr(h + halfH);

    for (int y = 0; y < halfH; y++)
    {
        uint8_t* dst = frame->data[1] + y * frame->linesize[1];
        const uint8_t* uRow = uPlane + y * halfW;
        const uint8_t* vRow = vPlane + y * halfW;
        for (int x = 0; x < halfW; x++)
        {
            dst[x * 2]     = uRow[x];
            dst[x * 2 + 1] = vRow[x];
        }
    }
}

// ---------------------------------------------------------------------------
// Color space conversion: BGR -> I420 (write back to frame)
// ---------------------------------------------------------------------------
static void BGRToI420(const cv::Mat& bgr, struct obs_source_frame* frame)
{
    int w = frame->width;
    int h = frame->height;

    cv::Mat yuv;
    cv::cvtColor(bgr, yuv, cv::COLOR_BGR2YUV_I420);

    // Y plane
    for (int y = 0; y < h; y++)
        memcpy(frame->data[0] + y * frame->linesize[0], yuv.ptr(y), w);

    // U plane
    int halfW = w / 2;
    int halfH = h / 2;
    for (int y = 0; y < halfH; y++)
        memcpy(frame->data[1] + y * frame->linesize[1], yuv.ptr(h + y), halfW);

    // V plane
    for (int y = 0; y < halfH; y++)
        memcpy(frame->data[2] + y * frame->linesize[2], yuv.ptr(h + halfH + y), halfW);
}

// ---------------------------------------------------------------------------
// Skin smoothing: bilateral filter on face ROI
// ---------------------------------------------------------------------------
static void ApplySmoothing(cv::Mat& bgr, const cv::Rect& roi, int level)
{
    if (level <= 0)
        return;

    // bilateralFilter parameters scale with level
    // d: diameter of pixel neighborhood (higher = stronger blur)
    // sigmaColor: filter sigma in color space
    // sigmaSpace: filter sigma in coordinate space
    int d = 5 + level;             // 6..15
    double sigmaColor = 20.0 + level * 8.0;   // 28..100
    double sigmaSpace = 20.0 + level * 8.0;

    cv::Mat roiMat = bgr(roi);
    cv::Mat smoothed;
    cv::bilateralFilter(roiMat, smoothed, d, sigmaColor, sigmaSpace);
    smoothed.copyTo(roiMat);
}

// ---------------------------------------------------------------------------
// Whitening: increase L channel in LAB color space on face ROI
// ---------------------------------------------------------------------------
static void ApplyWhitening(cv::Mat& bgr, const cv::Rect& roi, int level)
{
    if (level <= 0)
        return;

    cv::Mat roiMat = bgr(roi);
    cv::Mat lab;
    cv::cvtColor(roiMat, lab, cv::COLOR_BGR2Lab);

    // Split channels
    std::vector<cv::Mat> channels;
    cv::split(lab, channels);

    // Increase L channel (lightness)
    // Scale: level 1 = +3, level 10 = +30
    double alpha = 1.0 + level * 0.03;  // 1.03 .. 1.30
    int beta = level * 2;               // 2 .. 20
    channels[0].convertTo(channels[0], -1, alpha, beta);

    cv::merge(channels, lab);
    cv::cvtColor(lab, roiMat, cv::COLOR_Lab2BGR);
}

// ---------------------------------------------------------------------------
// OBS filter callbacks
// ---------------------------------------------------------------------------

static const char* beauty_get_name(void* unused)
{
    UNREFERENCED_PARAMETER(unused);
    return "Beauty Filter";
}

static void* beauty_create(obs_data_t* settings, obs_source_t* source)
{
    BeautyFilterContext* ctx = new BeautyFilterContext();
    ctx->source          = source;
    ctx->enabled         = obs_data_get_bool(settings, "enabled");
    ctx->smoothLevel     = (int)obs_data_get_int(settings, "smooth_level");
    ctx->whiteLevel      = (int)obs_data_get_int(settings, "white_level");
    ctx->hasFace         = false;
    ctx->lastDetectTimeMs = 0;
    ctx->detectIntervalMs = 200;
    ctx->lastWidth       = 0;
    ctx->lastHeight      = 0;

    // Load YuNet face detection model (lazy load to avoid blocking filter creation)
    std::string modelPath = GetExeDir() + "\\models\\face_detection_yunet_2023mar.onnx";
    if (GetFileAttributesA(modelPath.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        blog(LOG_INFO, "[BeautyFilter] Found model file at: %s", modelPath.c_str());
        // Don't load detector during creation - load it on first use instead
        // This prevents blocking the filter creation and allows better error handling
        ctx->faceDetector = nullptr;
        ctx->modelPath = modelPath; // Store for lazy loading
    }
    else
    {
        blog(LOG_WARNING, "[BeautyFilter] Face detection model not found at: %s", modelPath.c_str());
        ctx->faceDetector = nullptr;
    }

    return ctx;
}

static void beauty_destroy(void* data)
{
    BeautyFilterContext* ctx = static_cast<BeautyFilterContext*>(data);
    delete ctx;
}

static void beauty_update(void* data, obs_data_t* settings)
{
    BeautyFilterContext* ctx = static_cast<BeautyFilterContext*>(data);
    std::lock_guard<std::mutex> lock(ctx->paramMutex);
    ctx->enabled     = obs_data_get_bool(settings, "enabled");
    ctx->smoothLevel = (int)obs_data_get_int(settings, "smooth_level");
    ctx->whiteLevel  = (int)obs_data_get_int(settings, "white_level");
}

static obs_properties_t* beauty_get_properties(void* data)
{
    UNREFERENCED_PARAMETER(data);

    obs_properties_t* props = obs_properties_create();
    obs_properties_add_bool(props, "enabled", "Enable Beauty");
    obs_properties_add_int_slider(props, "smooth_level", "Smooth", 0, 10, 1);
    obs_properties_add_int_slider(props, "white_level", "Whiten", 0, 10, 1);
    return props;
}

static void beauty_get_defaults(obs_data_t* settings)
{
    obs_data_set_default_bool(settings, "enabled", false);
    obs_data_set_default_int(settings, "smooth_level", 5);
    obs_data_set_default_int(settings, "white_level", 3);
}

// ---------------------------------------------------------------------------
// Core frame processing
// ---------------------------------------------------------------------------
static struct obs_source_frame* beauty_filter_video(void* data,
                                                     struct obs_source_frame* frame)
{
    BeautyFilterContext* ctx = static_cast<BeautyFilterContext*>(data);

    // Quick exit if disabled
    bool enabled;
    int smoothLevel, whiteLevel;
    {
        std::lock_guard<std::mutex> lock(ctx->paramMutex);
        enabled     = ctx->enabled;
        smoothLevel = ctx->smoothLevel;
        whiteLevel  = ctx->whiteLevel;
    }

    if (!enabled || (smoothLevel == 0 && whiteLevel == 0))
        return frame;

    int w = frame->width;
    int h = frame->height;
    if (w == 0 || h == 0)
        return frame;

    // Convert frame to BGR
    bool isNV12 = (frame->format == VIDEO_FORMAT_NV12);
    bool isI420 = (frame->format == VIDEO_FORMAT_I420);

    if (!isNV12 && !isI420)
        return frame;  // Unsupported format, pass through

    cv::Mat bgr;
    if (isNV12)
        NV12ToBGR(frame, bgr);
    else
        I420ToBGR(frame, bgr);

    // Face detection (throttled)
    uint64_t now = GetTickCount64();
    bool runDetect = false;

    // Lazy load face detector on first use
    if (!ctx->faceDetector && !ctx->modelPath.empty())
    {
        try
        {
            ctx->faceDetector = cv::FaceDetectorYN::create(
                ctx->modelPath,
                "",
                cv::Size(320, 320),
                0.6f,
                0.3f,
                5000
            );
            blog(LOG_INFO, "[BeautyFilter] Face detector loaded successfully on first use");
        }
        catch (const cv::Exception& e)
        {
            blog(LOG_WARNING, "[BeautyFilter] Face detector load failed: %s", e.what());
            ctx->faceDetector = nullptr;
        }
        catch (...)
        {
            blog(LOG_WARNING, "[BeautyFilter] Face detector load failed (unknown error)");
            ctx->faceDetector = nullptr;
        }
    }

    if (ctx->faceDetector)
    {
        if (now - ctx->lastDetectTimeMs >= (uint64_t)ctx->detectIntervalMs ||
            w != ctx->lastWidth || h != ctx->lastHeight)
        {
            runDetect = true;
            ctx->lastDetectTimeMs = now;
            ctx->lastWidth = w;
            ctx->lastHeight = h;
        }

        if (runDetect)
        {
            try
            {
                ctx->faceDetector->setInputSize(cv::Size(w, h));
                cv::Mat faces;
                ctx->faceDetector->detect(bgr, faces);

                if (!faces.empty() && faces.rows > 0)
                {
                    // Use the first (largest/most confident) face
                    float x = faces.at<float>(0, 0);
                    float y = faces.at<float>(0, 1);
                    float fw = faces.at<float>(0, 2);
                    float fh = faces.at<float>(0, 3);

                    // Expand ROI by 1.5x for forehead/neck coverage
                    float cx = x + fw / 2.0f;
                    float cy = y + fh / 2.0f;
                    float ew = fw * 1.5f;
                    float eh = fh * 1.5f;

                    int rx = std::max(0, (int)(cx - ew / 2.0f));
                    int ry = std::max(0, (int)(cy - eh / 2.0f));
                    int rw = std::min(w - rx, (int)ew);
                    int rh = std::min(h - ry, (int)eh);

                    ctx->lastFaceRect = cv::Rect(rx, ry, rw, rh);
                    ctx->hasFace = true;
                }
                else
                {
                    ctx->hasFace = false;
                }
            }
            catch (const cv::Exception&)
            {
                ctx->hasFace = false;
            }
        }
    }

    // Determine processing region
    cv::Rect roi;
    if (ctx->hasFace && ctx->lastFaceRect.width > 0 && ctx->lastFaceRect.height > 0)
    {
        roi = ctx->lastFaceRect;
    }
    else
    {
        // No face detected or no detector: process full frame
        roi = cv::Rect(0, 0, w, h);
    }

    // Ensure ROI is within frame bounds
    roi &= cv::Rect(0, 0, w, h);
    if (roi.width <= 0 || roi.height <= 0)
        return frame;

    // Apply beauty effects
    ApplySmoothing(bgr, roi, smoothLevel);
    ApplyWhitening(bgr, roi, whiteLevel);

    // Convert back to original format
    if (isNV12)
        BGRToNV12(bgr, frame);
    else
        BGRToI420(bgr, frame);

    return frame;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

static struct obs_source_info beauty_filter_info = {};

void RegisterBeautyFilter()
{
    beauty_filter_info.id             = "beauty_filter";
    beauty_filter_info.type           = OBS_SOURCE_TYPE_FILTER;
    beauty_filter_info.output_flags   = OBS_SOURCE_ASYNC_VIDEO;
    beauty_filter_info.get_name       = beauty_get_name;
    beauty_filter_info.create         = beauty_create;
    beauty_filter_info.destroy        = beauty_destroy;
    beauty_filter_info.update         = beauty_update;
    beauty_filter_info.get_properties = beauty_get_properties;
    beauty_filter_info.get_defaults   = beauty_get_defaults;
    beauty_filter_info.filter_video   = beauty_filter_video;

    obs_register_source(&beauty_filter_info);
}
