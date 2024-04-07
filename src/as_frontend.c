#include "include/as_frontend.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char STRLEN_TEMPLATE[] =
".type strlen, @function\n"
"strlen:\n"
"   pushl %ebp\n"
"   movl %esp, %ebp\n"
"   movl $0, %edi\n"
"   movl 8(%esp), %eax\n"
"   jmp strlenloop\n"
"\n"
"strlenloop:\n"
"   movb (%eax, %edi, 1), %cl\n"
"   cmpb $0, %cl\n"
"   je strlenend\n"
"   addl $1, %edi\n"
"   jmp strlenloop\n"
"\n"
"strlenend:\n"
"   movl %edi, %eax\n"
"   movl %ebp, %esp\n"
"   popl %ebp\n"
"   ret\n";

static AST_T* var_lookup(list_T* list, const char* name)
{
    for(int i = 0; i < (int) list->size; i++)
    {
        AST_T* child_ast = (AST_T*) list->items[i];

        if(child_ast->type != AST_VARIABLE || !child_ast->name)
        {
            continue;
        }

        if(strcmp(child_ast->name, name) == 0)
        {
            return child_ast;
        }
    }

    return 0;
}

char* as_f_compound(AST_T* ast, list_T* list)
{
    char* value = calloc(1, sizeof(char));

    for(unsigned int i = 0; i < ast->children->size; i++)
    {
        AST_T* child_ast = (AST_T*) ast->children->items[i];
        char* next_value = as_f(child_ast, list);
        value = realloc(value, (strlen(value) + strlen(next_value) + 1) * sizeof(char));
        strcat(value, next_value);
    }

    return value;
}

char* as_f_assignment(AST_T* ast, list_T* list)
{
    char* s = calloc(1, sizeof(char));

    if(ast->value->type == AST_FUNCTION)
    {
        const char* template = ".globl %s\n"
                                "%s:\n"
                                "pushl %%ebp\n"
                                "movl %%esp, %%ebp\n";
        
        s = realloc(s, (strlen(template) + (strlen(ast->name)*2) + 1) * sizeof(char));
        sprintf(s, template, ast->name, ast->name);

        AST_T* as_val = ast->value;

        for(unsigned int i = 0; i < as_val->children->size; i++)
        {
            AST_T* farg = (AST_T*) as_val->children->items[i];
            AST_T* arg_variable = init_ast(AST_VARIABLE);
            arg_variable->name = farg->name;
            arg_variable->int_value = (4 * as_val->children->size) - (i*4);
            list_push(list, arg_variable);
        }

        char* as_val_val = as_f(as_val->value, list);

        s = realloc(s, (strlen(s) + strlen(as_val_val) + 1) * sizeof(char));
        strcat(s, as_val_val);
    }

    return s;
}

char* as_f_variable(AST_T* ast, list_T* list)
{
    char* s = calloc(1, sizeof(char));

    AST_T* var = var_lookup(list, ast->name);

    if(!var)
    {
        printf("[AS Frontend]: %s is not defined\n", var->name);
        exit(1);
    }

    const char* template = "pushl %d(%%esp)\n";
    s = realloc(s, (strlen(template) + 8) * sizeof(char));
    sprintf(s, template, var->int_value);

    return s;
}

char* as_f_call(AST_T* ast, list_T* list)
{
    char* s = calloc(1, sizeof(char));

    if(strcmp(ast->name, "return") == 0)
    {
        AST_T* first_arg = (AST_T*) ast->value->children->size ? ast->value->children->items[0] : (void*) 0;
        char* var_s = calloc(3, sizeof(char));
        var_s[0] = '$';
        var_s[1] = '0';
        var_s[2] = '\0';

        if(first_arg)
        {
            char* as_var_s = as_f(first_arg, list);
            var_s = realloc(var_s, (strlen(as_var_s) + 1) * sizeof(char));
            strcpy(var_s, as_var_s);
            free(as_var_s);
        }

        const char* template = "%s\n"
                                "popl %%eax\n"
                                "movl %%ebp, %%esp\n"
                                "popl %%ebp\n\n"
                                "ret\n";

        char* ret_s = calloc(strlen(template) + 128, sizeof(char));
        sprintf(ret_s, template, var_s);
        s = realloc(s, (strlen(ret_s) + 1) * sizeof(char));
        strcat(s, ret_s);
    }
    
    return s;
}

char* as_f_int(AST_T* ast, list_T* list)
{
    const char* template = "pushl $%d\n";
    char* s = calloc(strlen(template) + 128, sizeof(char));
    sprintf(s, template, ast->int_value);

    return s;
}

char* as_f_string(AST_T* ast, list_T* list)
{
    return ast->string_value;
}

char* as_f_root(AST_T* ast, list_T* list)
{
    const char* section_text = ".section .text\n"
                                ".globl _start\n"
                                "_start:\n"
                                "movl \%esp, \%ebp\n"
                                "call main\n"
                                "movl \%eax, \%ebx\n"
                                "movl $1, \%eax\n"
                                "int $0x80\n\n";

    char* value = (char*) calloc((strlen(section_text) + 128), sizeof(char));
    strcpy(value, section_text);

    char* next_value = as_f(ast, list);
    value = (char*) realloc(value, (strlen(value) + strlen(next_value) + 1) * sizeof(char));
    strcat(value, next_value);

    value = realloc(value, (strlen(value) + strlen(STRLEN_TEMPLATE) + 1) * sizeof(char));
    strcat(value, STRLEN_TEMPLATE);

    return value;
}

char* as_f_access(AST_T* ast, list_T* list)
{
    //int stackpos = ast->id;
    int offset = 12 + (ast->int_value * 4);

    const char* template = "# access\n"
                            "pushl %d(%%ebp)\n";

    //int pos = (stackpos) * 8;
    char* s = calloc(strlen(template) + 128, sizeof(char));
    sprintf(s, template, offset);

    return s;
}

char* as_f(AST_T* ast, list_T* list)
{
    char* value = calloc(1, sizeof(char));
    char* next_value = 0;

    switch(ast->type)
    {
        case AST_COMPOUND:
            next_value = as_f_compound(ast, list);
            break;
        case AST_ASSIGNMENT:
            next_value = as_f_assignment(ast, list);
            break;
        case AST_VARIABLE:
            next_value = as_f_variable(ast, list);
            break;
        case AST_CALL:
            next_value = as_f_call(ast, list);
            break;
        case AST_INT:
            next_value = as_f_int(ast, list);
            break;
        case AST_STRING:
            next_value = as_f_string(ast, list);
            break;
        case AST_ACCESS:
            next_value = as_f_access(ast, list);
            break;
        default:
            printf("[AS Frontend]: No frontend for AST of type %d\n", ast->type);
            exit(1);
            break;
    }

    value = realloc(value, (strlen(next_value) + 1) * sizeof(char));
    strcat(value, next_value);

    return value;
}