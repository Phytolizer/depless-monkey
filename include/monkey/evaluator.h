#ifndef MONKEY_EVALUATOR_H_
#define MONKEY_EVALUATOR_H_

#include "monkey/ast.h"
#include "monkey/object.h"

struct object* eval(struct ast_node* node);

#endif  // MONKEY_EVALUATOR_H_
