#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256
#define MAX_SECTION_LENGTH 64
#define MAX_NAME_LENGTH 64


int GetValueInt(int *value, char *name, char *filename, char *section, char *defval) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        *value = atoi(defval);
        return -1; // 文件打开失败
    }

    char line[MAX_LINE_LENGTH];
    int in_desired_section = 0;
    int found = 0;

    while (fgets(line, sizeof(line), fp)) {
        // 去除行尾的换行符
        line[strcspn(line, "\n")] = 0;

        // 跳过空行和注释行
        if (line[0] == '#' || line[0] == ';' || line[0] == '\0') {
            continue;
        }

        // 检查是否是节声明 [section]
        if (line[0] == '[') {
            char *end = strchr(line, ']');
            if (end != NULL) {
                *end = '\0';
                char current_section[MAX_SECTION_LENGTH];
                strncpy(current_section, line + 1, sizeof(current_section) - 1);
                current_section[sizeof(current_section) - 1] = '\0';

                // 去除节名两边的空白
                char *start = current_section;
                while (isspace(*start)) start++;
                char *end_section = current_section + strlen(current_section) - 1;
                while (end_section > start && isspace(*end_section)) end_section--;
                *(end_section + 1) = '\0';

                in_desired_section = (strcmp(start, section) == 0);
            }
            continue;
        }

        // 如果在目标节中，查找键值对
        if (in_desired_section) {
            char *equal_sign = strchr(line, '=');
            if (equal_sign != NULL) {
                *equal_sign = '\0';
                char current_name[MAX_NAME_LENGTH];
                strncpy(current_name, line, sizeof(current_name) - 1);
                current_name[sizeof(current_name) - 1] = '\0';

                // 去除键名两边的空白
                char *start_name = current_name;
                while (isspace(*start_name)) start_name++;
                char *end_name = current_name + strlen(current_name) - 1;
                while (end_name > start_name && isspace(*end_name)) end_name--;
                *(end_name + 1) = '\0';

                if (strcmp(start_name, name) == 0) {
                    char *val = equal_sign + 1;
                    // 去除值两边的空白
                    while (isspace(*val)) val++;
                    char *end_val = val + strlen(val) - 1;
                    while (end_val > val && isspace(*end_val)) end_val--;
                    *(end_val + 1) = '\0';

                    *value = atoi(val);
                    found = 1;
                    break;
                }
            }
        }
    }

    fclose(fp);

    if (!found) {
        *value = atoi(defval);
        return -2; // 未找到指定键
    }

    return 0; // 成功
}