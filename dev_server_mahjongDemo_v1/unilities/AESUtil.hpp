/*+==================================================================
* CopyRight (c) 2021 Boke
* All rights reserved
* This program is UNPUBLISHED PROPRIETARY property.
* Only for internal distribution.
*
* FileName: AESUtil.hpp
* 
* Purpose:  Implement AESUtil.
*
* Author:   fengjianhua(fengjianhua@boke.com)
* 
* Modify:   2021/7/29 20:31
==================================================================+*/
#include <string>
#include "AES.h"
#include "BASE64.h"

class AESUtil
{
public:
    // Define Pkcs5Padding
    static inline void Pkcs5Padding(std::string& strValue)
    {
        Pkcs7Padding(strValue, 8);
    }

    // Define Pkcs7Padding
    static inline void Pkcs7Padding(std::string& strValue, int nBlockSize)
    {
        if (nBlockSize < 1 || nBlockSize > 255)
        {
            printf("Pkcs7Padding: Invalid BlockSize\n");
            return;
        }
        int nPaddingCount = nBlockSize - strValue.length() % nBlockSize;
        if (0 == nPaddingCount)
        {
            nPaddingCount = nBlockSize;
        }
        char chPaddingValue = (char)nPaddingCount;
        for (int i = 0; i < nPaddingCount; ++i)
        {
            strValue.append(1, chPaddingValue);
        }
    }

    // AES encrypt method, padding as pkcs5, use CBC method, output as base64, 
    static std::string AESEncryptPKCS5CBCBase64(std::string strPlainText, const std::string& strKey, const std::string& strIVParam)
    {
        Pkcs5Padding(strPlainText);
        // Use pkcs5padding
        AES hAES(128);
        unsigned int nOutLen = 0;
        const unsigned char* pOut = hAES.EncryptCBC((const unsigned char*)strPlainText.c_str(), (unsigned int)strPlainText.length(), (const unsigned char*)strKey.c_str(), (const unsigned char*)strIVParam.c_str(), nOutLen);
        std::string strResult = BASE64::base64_encode((const char*)pOut, (int)nOutLen);
        delete pOut;
        return strResult;
    }

    // AES decrypt method, padding as pkcs5, use CBD method, input string should be base64
    static std::string AESDecryptPKCS5CBCBase64(const std::string& strBase64, const std::string& strKey, const std::string& strIVParam)
    {
        std::string strEncrypt = BASE64::base64_decode(strBase64);
        unsigned int nRstLen = (unsigned int)strEncrypt.length();
        AES hAES(128);
        const unsigned char* pOut = hAES.DecryptCBC((const unsigned char*)strEncrypt.c_str(), nRstLen, (const unsigned char*)strKey.c_str(), (const unsigned char*)strIVParam.c_str());
        std::string strResult;
        for (unsigned int i = 0; i < nRstLen; ++i)
        {
            char chCur = pOut[i];
            strResult.append(1, chCur);
        }
        // trip back end '\0'
        while (!strResult.empty())
        {
            if (strResult.back() != '\0')
            {
                break;
            }
            strResult.pop_back();
        }
        delete pOut;
        if (!strResult.empty())
        {
            char chBack = strResult.back();
            strResult.resize(strResult.length() - chBack);
        }
        return strResult;
    }
};
