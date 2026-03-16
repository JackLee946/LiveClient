#pragma once

#include <string>
#include <Windows.h>

namespace StringUtil
{
    // Convert UTF-16 CString/wstring to UTF-8 std::string
    inline std::string WideToUtf8(const wchar_t* wide)
    {
        if (!wide || !wide[0])
            return std::string();

        int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0)
            return std::string();

        std::string result(len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, &result[0], len, nullptr, nullptr);
        return result;
    }

    inline std::string WideToUtf8(const CString& str)
    {
        return WideToUtf8((LPCTSTR)str);
    }

    // Convert UTF-8 std::string to UTF-16 CString
    inline CString Utf8ToWide(const char* utf8)
    {
        if (!utf8 || !utf8[0])
            return CString();

        int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
        if (len <= 0)
            return CString();

        CString result;
        LPTSTR buf = result.GetBuffer(len);
        MultiByteToWideChar(CP_UTF8, 0, utf8, -1, buf, len);
        result.ReleaseBuffer();
        return result;
    }

    inline CString Utf8ToWide(const std::string& str)
    {
        return Utf8ToWide(str.c_str());
    }
}
