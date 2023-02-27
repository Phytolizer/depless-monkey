#include <monkey/repl.h>

int main(void) {
    printf(
        "Hello! This is the Monkey programming language!\n"
        "Feel free to type in commands\n"
    );
    repl_start(stdin, stdout);
}
