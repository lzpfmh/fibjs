/*
 * encoding.cpp
 *
 *  Created on: Apr 10, 2012
 *      Author: lion
 */

#include "ifs/encoding.h"
#include "encoding.h"
#include "encoding_iconv.h"
#include "Url.h"

namespace fibjs
{

result_t encoding_base::base32Encode(Buffer_base *data, std::string &retVal)
{
    baseEncode("abcdefghijklmnopqrstuvwxyz234567", 5, data, retVal);
    return 0;
}

result_t encoding_base::base32Decode(const char *data,
                                     obj_ptr<Buffer_base> &retVal)
{
    static const char decodeTable[] =
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 2x  !"#$%&'()*+,-./   */
        14, 11, 26, 27, 28, 29, 30, 31, -1, 6, -1, -1, -1, -1, -1, -1, /* 3x 0123456789:;<=>?   */
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, /* 4x @ABCDEFGHIJKLMNO   */
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, /* 5X PQRSTUVWXYZ[\]^_   */
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, /* 6x `abcdefghijklmno   */
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1 /* 7X pqrstuvwxyz{\}~DEL */
    };

    baseDecode(decodeTable, 5, data, retVal);
    return 0;
}

result_t encoding_base::base64Encode(Buffer_base *data, std::string &retVal)
{
    baseEncode(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/",
        6, data, retVal);
    return 0;
}

result_t encoding_base::base64Decode(const char *data,
                                     obj_ptr<Buffer_base> &retVal)
{
    static const char decodeTable[] =
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, 62, -1, 63, /* 2x  !"#$%&'()*+,-./   */
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, /* 3x 0123456789:;<=>?   */
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, /* 4x @ABCDEFGHIJKLMNO   */
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, 63, /* 5X PQRSTUVWXYZ[\]^_   */
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* 6x `abcdefghijklmno   */
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1 /* 7X pqrstuvwxyz{\}~DEL */
    };

    baseDecode(decodeTable, 6, data, retVal);
    return 0;
}

result_t encoding_base::hexEncode(Buffer_base *data, std::string &retVal)
{
    std::string strData;
    static char HexChar[] = "0123456789abcdef";
    int32_t i, pos, len1;

    data->toString(strData);

    i = (int32_t) strData.length() * 2;
    retVal.resize(i);

    len1 = 0;
    pos = 0;

    for (i = 0; i < (int32_t) strData.length(); i++)
    {
        retVal[pos * 2] = HexChar[(unsigned char) strData[i] >> 4];
        retVal[pos * 2 + 1] = HexChar[(unsigned char) strData[i] & 0xf];
        pos++;
        len1 += 2;
    }

    return 0;
}

result_t encoding_base::hexDecode(const char *data,
                                  obj_ptr<Buffer_base> &retVal)
{
    int32_t pos, len = (int32_t) qstrlen(data);
    const char *end = data + len;
    std::string strBuf;
    uint32_t ch1, ch2;

    strBuf.resize(len / 2);

    pos = 0;
    while ((ch1 = utf8_getchar(data, end)) != 0)
    {
        if (qisxdigit(ch1))
            ch1 = qhex(ch1);
        else
            continue;

        ch2 = utf8_getchar(data, end);
        if (ch2 == 0)
            break;

        if (qisxdigit(ch2))
            ch2 = qhex(ch2);
        else
        {
            ch2 = ch1;
            ch1 = 0;
        }

        strBuf[pos++] = (ch1 << 4) + ch2;
    }

    retVal = new Buffer(strBuf);

    return 0;
}

result_t encoding_base::iconvEncode(const char *charset, const char *data,
                                    obj_ptr<Buffer_base> &retVal)
{
    return encoding_iconv(charset).encode(data, retVal);
}

result_t encoding_base::iconvDecode(const char *charset, Buffer_base *data,
                                    std::string &retVal)
{
    return encoding_iconv(charset).decode(data, retVal);
}

static const char *URITable =
    " ! #$ &'()*+,-./0123456789:; = ?@ABCDEFGHIJKLMNOPQRSTUVWXYZ    _ abcdefghijklmnopqrstuvwxyz   ~ ";
static const char *URIComponentTable =
    " !     '()*  -. 0123456789       ABCDEFGHIJKLMNOPQRSTUVWXYZ    _ abcdefghijklmnopqrstuvwxyz   ~ ";

result_t encoding_base::encodeURI(const char *url, std::string &retVal)
{
    Url::encodeURI(url, -1, retVal, URITable);
    return 0;
}

result_t encoding_base::encodeURIComponent(const char *url, std::string &retVal)
{
    Url::encodeURI(url, -1, retVal, URIComponentTable);
    return 0;
}

result_t encoding_base::decodeURI(const char *url, std::string &retVal)
{
    Url::decodeURI(url, -1, retVal);
    return 0;
}

}

