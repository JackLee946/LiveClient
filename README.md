# LiveClient - 直播客户端

基于 C++ MFC 和 libobs 的全功能直播推流客户端，支持 RTMP 推流、本地录制、多场景管理、多源合成等功能。

## 技术架构

### 技术栈

| 项目 | 说明 |
|------|------|
| 语言 | C++17 |
| UI 框架 | MFC (Microsoft Foundation Classes) |
| 核心引擎 | libobs (OBS Studio 27.2.4) |
| 编译器 | MSVC v142 (Visual Studio 2019) |
| 目标平台 | Windows x86 (32-bit) |
| 字符集 | Unicode |

### 项目结构

```
LiveClient/
├── LiveClient.sln                  # VS2019 解决方案
├── LiveClient.vcxproj              # MSBuild 项目文件
├── obs_sdk.props                   # OBS SDK 路径配置（MSBuild 属性表）
│
├── LiveClient.h / .cpp             # MFC 应用程序入口 (CWinApp)
├── LiveClient.rc                   # 对话框资源模板
├── resource.h                      # 控件 ID 定义
├── stdafx.h / .cpp                 # 预编译头
├── targetver.h                     # Windows SDK 版本
│
├── obs/                            # OBS 封装层（纯 C++，无 MFC 依赖）
│   ├── ObsApp.h / .cpp             #   OBS 生命周期管理（单例）
│   ├── ObsScene.h / .cpp           #   场景管理
│   ├── ObsSource.h / .cpp          #   音视频源工厂
│   ├── ObsEncoder.h / .cpp         #   编码器（视频 x264/NVENC + 音频 AAC）
│   ├── ObsService.h / .cpp         #   RTMP 推流服务
│   ├── ObsOutput.h / .cpp          #   推流和录制输出
│   ├── ObsPreview.h / .cpp         #   视频预览渲染
│   └── ObsSettings.h / .cpp        #   持久化配置管理
│
├── dialogs/                        # MFC 对话框层
│   ├── LiveClientDlg.h / .cpp      #   主界面（预览、场景、源、混音、控制）
│   ├── SettingsDlg.h / .cpp        #   设置对话框（4 个 Tab 页）
│   ├── AddSourceDlg.h / .cpp       #   添加源对话框
│   └── SourcePropDlg.h / .cpp      #   源属性编辑（动态生成控件）
│
├── utils/
│   └── StringUtil.h / .cpp         # UTF-8 / UTF-16 转换工具
│
├── res/                            # 资源文件
│   ├── LiveClient.ico
│   └── LiveClient.rc2
│
└── third_party/
    └── obs-studio-sdk/             # OBS SDK（运行时依赖）
        ├── include/                #   头文件 (obs.h 等)
        ├── lib/32bit/              #   导入库 (obs.lib)
        ├── bin/32bit/              #   核心 DLL (obs.dll 等)
        ├── obs-plugins/32bit/      #   插件 DLL
        └── data/                   #   libobs 和插件数据文件
```

### 架构分层

```
┌─────────────────────────────────────────────────────────┐
│                    MFC 对话框层 (dialogs/)                │
│  CLiveClientDlg  CSettingsDlg  CAddSourceDlg  CSourcePropDlg │
├─────────────────────────────────────────────────────────┤
│                    OBS 封装层 (obs/)                      │
│  CObsApp  CObsScene  CObsSource  CObsEncoder            │
│  CObsService  CObsOutput  CObsPreview  CObsSettings     │
├─────────────────────────────────────────────────────────┤
│                    libobs C API                          │
│  obs_startup / obs_reset_video / obs_scene_create / ...  │
├─────────────────────────────────────────────────────────┤
│                    系统层                                 │
│  D3D11 渲染  ·  FFmpeg 编解码  ·  RTMP 网络传输          │
└─────────────────────────────────────────────────────────┘
```

**OBS 封装层**是项目的核心设计，它将 libobs 的 C API 封装为 C++ 类，对上层 MFC 对话框隐藏了 OBS 的实现细节：

| 类 | 封装的 libobs 功能 | 关键 API |
|----|-------------------|----------|
| `CObsApp` | OBS 生命周期管理（单例） | `obs_startup`, `obs_reset_video`, `obs_reset_audio`, `obs_load_all_modules` |
| `CObsScene` | 场景的创建/删除/切换，场景项管理 | `obs_scene_create`, `obs_set_output_source`, `obs_scene_add` |
| `CObsSource` | 各类型音视频源的创建 | `obs_source_create`, `obs_source_update`, `obs_source_properties` |
| `CObsEncoder` | 视频编码器 (x264/NVENC) + 音频编码器 (AAC) | `obs_video_encoder_create`, `obs_audio_encoder_create` |
| `CObsService` | RTMP 推流服务器配置 | `obs_service_create("rtmp_custom", ...)` |
| `CObsOutput` | 推流输出 + 录制输出，信号回调 | `obs_output_create("rtmp_output"/"ffmpeg_muxer")`, `obs_output_start/stop` |
| `CObsPreview` | 将 OBS 渲染输出到 MFC 窗口 | `obs_display_create`, `obs_render_main_texture` |
| `CObsSettings` | 所有配置的持久化（Windows 注册表） | `CWinApp::WriteProfileString/Int` |

### 线程模型

```
┌──────────────┐     PostMessage(WM_APP+N)     ┌────────────────┐
│  OBS 内部线程  │  ─────────────────────────>  │  MFC UI 线程    │
│  (编码/推流)   │                               │  (对话框消息循环) │
└──────────────┘                               └────────────────┘

┌──────────────┐     obs_display 回调           ┌────────────────┐
│  OBS 渲染线程  │  ───── DrawCallback ──────>  │  D3D11 渲染     │
│  (graphics)   │     obs_render_main_texture   │  (预览窗口 HWND) │
└──────────────┘                               └────────────────┘
```

- OBS 的推流/录制信号回调运行在 OBS 内部线程，通过 `PostMessage` 转发到 UI 线程
- OBS 的预览渲染回调在 OBS 的 graphics 线程执行，直接调用 `obs_render_main_texture`
- 所有 MFC 控件操作只在 UI 线程进行，避免线程安全问题

### 数据流

**推流数据流：**

```
音视频源 (Source)
    │
    ▼
场景合成 (Scene)  ──→  obs_display 预览渲染 ──→ 预览窗口
    │
    ▼
视频编码器 (x264/NVENC) + 音频编码器 (AAC)
    │
    ▼
RTMP 输出 (rtmp_output) ──→ 推流服务器 (如 Bilibili / Twitch / YouTube)
    │
    └──→ 录制输出 (ffmpeg_muxer) ──→ 本地文件 (.mkv / .mp4)
```

---

## 编译说明

### 环境要求

- Visual Studio 2019（需安装 "C++ 桌面开发" 工作负载）
- Windows 10 SDK
- OBS Studio 27.2.4 SDK（已包含在 `third_party/obs-studio-sdk/`）

### 编译步骤

1. 用 Visual Studio 2019 打开 `LiveClient.sln`
2. 确认配置为 **Debug | Win32** 或 **Release | Win32**
3. 确认 `obs_sdk.props` 中的 `OBS_SDK_DIR` 路径指向 `third_party\obs-studio-sdk`
4. 按 **Ctrl+Shift+B** 编译
5. 编译后 Post-Build 事件会自动将 OBS 运行时文件复制到输出目录

### OBS SDK 说明

项目已包含 OBS Studio 27.2.4 的 x86 SDK（`third_party/obs-studio-sdk/`），这是最后一个支持 32 位的 OBS 版本。SDK 包含：

- `include/` — 108 个头文件（obs.h、graphics/、util/ 等）
- `lib/32bit/obs.lib` — 导入库（从 obs.dll 生成）
- `bin/32bit/` — 25 个核心 DLL（obs.dll、libobs-d3d11.dll、FFmpeg 库等）
- `obs-plugins/32bit/` — 27 个插件 DLL（编码器、采集、输出等）
- `data/` — libobs 的 effect 着色器文件和插件数据

---

## 软件使用说明

### 启动

编译成功后，按 **F5** 或运行 `Debug/LiveClient.exe`。程序启动后会初始化 OBS 引擎并显示主界面。

### 主界面布局

```
┌──────────────────────────────────────────┬──────────┐
│                                          │ Scenes   │
│                                          │ ┌──────┐ │
│           视频预览区域                     │ │Scene1│ │
│      (实时显示合成后的画面)                 │ │Scene2│ │
│                                          │ └──────┘ │
│                                          │ [+] [-]  │
│                                          ├──────────┤
│                                          │ Sources  │
│                                          │ ┌──────┐ │
│                                          │ │Camera│ │
│                                          │ │Screen│ │
│                                          │ └──────┘ │
│                                          │[+][-][P] │
├──────────────────────────┬───────────────┤[^] [v]   │
│ Audio Mixer              │ Controls      ├──────────┤
│ Master [━━━━━━━━] □Mute  │ [推流] [录制] │          │
│                          │  [设置]       │          │
├──────────────────────────┴───────────────┴──────────┤
│ Offline                                    00:00:00 │
└─────────────────────────────────────────────────────┘
```

### 场景管理

场景（Scene）是多个音视频源的容器，可以在不同场景之间快速切换。

- **添加场景**：点击 Scenes 面板下方的 `+` 按钮
- **删除场景**：选中场景后点击 `-` 按钮（至少保留一个场景）
- **切换场景**：单击场景列表中的项目，预览和推流画面会立即切换

### 源管理

源（Source）是场景中的画面元素，支持以下类型：

| 源类型 | OBS 插件 ID | 说明 |
|--------|------------|------|
| Camera | `dshow_input` | 摄像头（DirectShow 视频采集设备） |
| Display Capture | `monitor_capture` | 整个显示器的屏幕采集 |
| Window Capture | `window_capture` | 指定窗口的画面采集 |
| Image | `image_source` | 静态图片（PNG、JPG 等） |
| Text | `text_ft2_source` | 文字叠加（FreeType 渲染） |

操作：

- **添加源**：点击 Sources 面板的 `+`，在弹窗中选择源类型和名称
- **删除源**：选中源后点击 `-`
- **编辑属性**：选中源后点击 `P`（Properties），弹出属性编辑对话框
- **调整层级**：使用 `^`（上移）和 `v`（下移）调整源的叠加顺序

### 音频混音

- **Master 滑块**：控制所有音频源的总音量（0-100）
- **Mute 复选框**：一键静音所有音频

### 推流

1. 点击 **Settings** 按钮，切换到 **Stream** 标签页
2. 填写 RTMP 服务器地址（如 `rtmp://live-push.example.com/live/`）
3. 填写推流密钥（Stream Key）
4. 点击 OK 保存设置
5. 点击 **Start Streaming** 按钮开始推流
6. 状态栏显示绿色 `LIVE` 标识、实时码率、帧率、丢帧数
7. 点击 **Stop Streaming** 停止推流

### 录制

1. 在 Settings → Output 中设置录制路径和格式（mkv/mp4/flv）
2. 点击 **Start Recording** 按钮开始录制
3. 录制文件自动按时间戳命名（如 `2026-03-16_14-30-00.mkv`）
4. 点击 **Stop Recording** 停止录制

> 推流和录制可以同时进行。

### 设置说明

点击 **Settings** 按钮打开设置对话框，包含 4 个标签页：

#### Stream（推流）

| 选项 | 说明 |
|------|------|
| Server | RTMP 推流服务器地址 |
| Stream Key | 推流密钥（密码遮掩显示） |

#### Output（输出）

| 选项 | 默认值 | 说明 |
|------|--------|------|
| Video Encoder | x264 (Software) | 视频编码器，可选 NVENC (NVIDIA 硬件加速) |
| Video Bitrate | 2500 kbps | 视频码率 |
| Rate Control | CBR | 码率控制模式（CBR 恒定 / VBR 可变） |
| Audio Bitrate | 128 kbps | 音频码率 |
| Recording Path | 我的视频 | 录制文件保存路径 |
| Format | mkv | 录制文件格式 |

#### Video（视频）

| 选项 | 默认值 | 说明 |
|------|--------|------|
| Base Resolution | 1920x1080 | 画布分辨率（源画面的合成尺寸） |
| Output Resolution | 1280x720 | 输出分辨率（编码和推流的实际尺寸） |
| FPS | 30 | 帧率 |

#### Audio（音频）

| 选项 | 默认值 | 说明 |
|------|--------|------|
| Sample Rate | 48000 Hz | 音频采样率 |
| Channels | Stereo | 声道（立体声/单声道） |

### 状态栏

推流/录制过程中，底部状态栏实时显示：

```
LIVE | 2500 kbps | 30.0 fps | 0 dropped                    01:23:45
```

- **LIVE** — 推流中（绿色），出错时显示红色
- **kbps** — 实时上行码率
- **fps** — 当前编码帧率
- **dropped** — 丢帧数（网络拥塞时增加）
- **REC** — 录制中（推流 + 录制同时进行时显示）
- **时间** — 推流或录制的持续时长

---

## 常见问题

### 启动时提示 "Failed to initialize OBS"

错误对话框会显示具体原因：

- **obs.dll not found** — Post-Build 事件未执行，重新编译一次
- **libobs-d3d11.dll not found** — 同上
- **obs_reset_video failed** — 检查 `data/libobs/` 目录下是否有 `.effect` 文件
- **obs_startup failed** — obs.dll 加载失败，确认输出目录中有完整的 OBS 运行时文件

### 推流失败

- 确认 RTMP 服务器地址格式正确（以 `rtmp://` 开头）
- 确认推流密钥无误
- 检查网络连接和防火墙设置

### 没有画面

- 确认已添加至少一个源（Display Capture 最容易测试）
- 检查源是否可见（Sources 列表中的勾选状态）
- 尝试调整源的属性（点击 P 按钮）
