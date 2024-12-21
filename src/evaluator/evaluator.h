//
// Created by dgood on 12/5/24.
//

#ifndef EVALUATOR_H
#define EVALUATOR_H
#include "../ast/ast.h"
#include "../object/environment.h"
#include "../object/object.h"

object_object *evaluator_eval(ast_node *, environment *);
#endif //EVALUATOR_H
