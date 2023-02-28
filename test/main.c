#include <inttypes.h>

#include "monkey/test/ast.h"
#include "monkey/test/evaluator.h"
#include "monkey/test/framework.h"
#include "monkey/test/lexer.h"
#include "monkey/test/parser.h"

int main(int argc, char** argv) {
    bool verbose = false;
    for (int i = 1; i < argc; i++) {
        if (STRING_EQUAL(STRING_REF_FROM_C(argv[i]), STRING_REF("-v")) ||
            STRING_EQUAL(STRING_REF_FROM_C(argv[i]), STRING_REF("--verbose"))) {
            verbose = true;
        } else {
            printf("Usage: %s [--verbose]\n", argv[0]);
            return 1;
        }
    }
    struct test_state state = {.verbose = verbose};
    RUN_SUITE(&state, ast, STRING_REF("ast"));
    RUN_SUITE(&state, evaluator, STRING_REF("evaluator"));
    RUN_SUITE(&state, lexer, STRING_REF("lexer"));
    RUN_SUITE(&state, parser, STRING_REF("parser"));

    fprintf(
        stderr,
        "%" PRIu64 " tests, %" PRIu64 " passed, %" PRIu64 " failed, %" PRIu64 " skipped, %" PRIu64
        " assertions\n",
        state.passed + state.failed,
        state.passed,
        state.failed,
        state.skipped,
        state.assertions
    );

    return state.failed != 0;
}
