#ifndef MONKEY_PARSEINT_H_
#define MONKEY_PARSEINT_H_

#include <stdbool.h>
#include <stdint.h>

#include "monkey/string.h"

struct parse_i64_result {
    bool ok;
    int64_t value;
};

extern struct parse_i64_result parse_i64(struct string str);

#endif  // MONKEY_PARSEINT_H_
