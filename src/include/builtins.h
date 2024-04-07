#ifndef BUILTINS_H
#define BUILTINS_H

#include "visitor.h"

AST_T* fptr_output(visitor_T* visitor, AST_T* node, list_T* list);

void builtins_init(list_T* list);
#endif