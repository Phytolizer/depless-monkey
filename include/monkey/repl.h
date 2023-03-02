#ifndef MONKEY_REPL_H_
#define MONKEY_REPL_H_

#include <monkey/environment.h>
#include <stdio.h>

extern void repl_start(FILE* in, FILE* out, struct environment* env);

#endif  // MONKEY_REPL_H_
