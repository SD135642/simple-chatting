#include <stdlib.h>
#include <string.h>

int parse_int(char *str, int *value) {
    return ((*value = atoi(str)) != 0) || 
           ((strlen(str) == 1) && (str[0] == '0'));
}
