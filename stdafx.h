#pragma once

#include "targetver.h"

#define VC_EXTRALEAN

#include <afxwin.h>
#include <afxext.h>
#include <afxdisp.h>
#include <afxdtctl.h>
#include <afxcmn.h>
#include <afxdialogex.h>

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <cstdint>

#include <obs.h>

// Use OpenCV Release library in Debug mode to avoid symbol mismatch
// Define this BEFORE including OpenCV headers to disable debug_build_guard namespace
#define CV_IGNORE_DEBUG_BUILD_GUARD

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect/face.hpp>

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif
