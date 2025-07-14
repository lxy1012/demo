/*+==================================================================
* CopyRight (c) 2021 Boke
* All rights reserved
* This program is UNPUBLISHED PROPRIETARY property.
* Only for internal distribution.
*
* FileName: BASE64.h
*
* Purpose:  Base 64 coder
*
* Author:   fengjianhua(fengjianhua@boke.com)
*
* Modify:   2021/8/30 21:33
==================================================================+*/
#ifndef _BASE64_h_
#define _BASE64_h_
#include <string>

class BASE64
{
public:
    static std::string base64_encode(const std::string& input);
    static std::string base64_decode(const std::string& input);
    static std::string base64_encode(const char* bytes_to_encode, int in_len);
    static std::string base64_decode(const char* encoded_string, int encoded_len);

private:
    static bool is_base64(unsigned char c);
};

#endif // _BASE64_h_
