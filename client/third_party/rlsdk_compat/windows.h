#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <string>

using DWORD  = uint32_t;
using SIZE_T = size_t;

#define __thiscall
#define FORCEINLINE inline

inline constexpr unsigned int CP_UTF8 = 65001;

inline int WideCharToMultiByte(unsigned int codePage, uint32_t flags, const wchar_t *wideStr, int wideLen, char *outStr,
                                int outLen, const char *defaultChar, bool *usedDefaultChar)
{
    (void) codePage;
    (void) flags;
    (void) defaultChar;
    (void) usedDefaultChar;

    size_t length = wideLen < 0 ? wcslen(wideStr) : (size_t) wideLen;
    std::string encoded;

    for (size_t i = 0; i < length; i++)
    {
        uint32_t codepoint = (uint32_t) wideStr[i];

        if (codepoint <= 0x7F)
        {
            encoded += (char) codepoint;
        }
        else if (codepoint <= 0x7FF)
        {
            encoded += (char) (0xC0 | (codepoint >> 6));
            encoded += (char) (0x80 | (codepoint & 0x3F));
        }
        else if (codepoint <= 0xFFFF)
        {
            encoded += (char) (0xE0 | (codepoint >> 12));
            encoded += (char) (0x80 | ((codepoint >> 6) & 0x3F));
            encoded += (char) (0x80 | (codepoint & 0x3F));
        }
        else
        {
            encoded += (char) (0xF0 | (codepoint >> 18));
            encoded += (char) (0x80 | ((codepoint >> 12) & 0x3F));
            encoded += (char) (0x80 | ((codepoint >> 6) & 0x3F));
            encoded += (char) (0x80 | (codepoint & 0x3F));
        }
    }

    if (wideLen < 0)
    {
        encoded += '\0';
    }

    if (outLen == 0)
    {
        return (int) encoded.size();
    }

    size_t copyLen = encoded.size() < (size_t) outLen ? encoded.size() : (size_t) outLen;
    memcpy(outStr, encoded.data(), copyLen);
    return (int) copyLen;
}

inline int MultiByteToWideChar(unsigned int codePage, uint32_t flags, const char *multiByteStr, int multiByteLen,
                                wchar_t *outStr, int outLen)
{
    (void) codePage;
    (void) flags;

    size_t length = multiByteLen < 0 ? strlen(multiByteStr) : (size_t) multiByteLen;
    std::wstring decoded;

    for (size_t i = 0; i < length;)
    {
        unsigned char lead = (unsigned char) multiByteStr[i];
        uint32_t codepoint;
        size_t extraBytes;

        if (lead <= 0x7F)
        {
            codepoint  = lead;
            extraBytes = 0;
        }
        else if ((lead & 0xE0) == 0xC0)
        {
            codepoint  = lead & 0x1F;
            extraBytes = 1;
        }
        else if ((lead & 0xF0) == 0xE0)
        {
            codepoint  = lead & 0x0F;
            extraBytes = 2;
        }
        else
        {
            codepoint  = lead & 0x07;
            extraBytes = 3;
        }

        i++;
        for (size_t j = 0; j < extraBytes && i < length; j++, i++)
        {
            codepoint = (codepoint << 6) | ((unsigned char) multiByteStr[i] & 0x3F);
        }

        decoded += (wchar_t) codepoint;
    }

    if (multiByteLen < 0)
    {
        decoded += L'\0';
    }

    if (outLen == 0)
    {
        return (int) decoded.size();
    }

    size_t copyLen = decoded.size() < (size_t) outLen ? decoded.size() : (size_t) outLen;
    memcpy(outStr, decoded.data(), copyLen * sizeof(wchar_t));
    return (int) copyLen;
}
