#include "monkey/parseint.h"

struct parse_i64_result parse_i64(struct string str) {
    struct parse_i64_result result = {0};
    if (str.length == 0) {
        return result;
    }
    int64_t value = 0;
    bool negative = false;
    size_t i = 0;
    if (str.data[0] == '-') {
        negative = true;
        i++;
    }
    for (; i < str.length; i++) {
        if (str.data[i] < '0' || str.data[i] > '9') {
            return result;
        }
        // check overflow
        if (value > (INT64_MAX - (str.data[i] - '0')) / 10) {
            return result;
        }
        value *= 10;
        value += str.data[i] - '0';
    }
    result.ok = true;
    result.value = negative ? -value : value;
    return result;
}
