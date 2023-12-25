#include <utils.h>
#include <string.h>
#include <stdlib.h>

/**
 * URL解码
 * @param encoded 编码过的URL字符串
 * @return 解码后的字符串
 */
char *url_docode(const char *encoded)
{
    char *decoded = malloc(strlen(encoded) + 1);
    int i, j = 0;
    for (i = 0; i < strlen(encoded); i++)
    {
        if (encoded[i] == '%')
        {
            // 判断是否是URL编码字符
            if (i + 2 < strlen(encoded))
            {
                char hex[3];
                hex[0] = encoded[i + 1];
                hex[1] = encoded[i + 2];
                hex[2] = '\0';
                // 将十六进制字符串转换为整数
                int value = strtol(hex, NULL, 16);
                decoded[j++] = (char)value;
                i += 2;
            }
        }
        else if (encoded[i] == '+')
        {
            // 将加号替换为空格
            decoded[j++] = ' ';
        }
        else
        {
            // 复制其他字符
            decoded[j++] = encoded[i];
        }
    }
    decoded[j] = '\0';
    return decoded;
}