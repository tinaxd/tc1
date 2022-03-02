#include "tc1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void gen_unique_label(char *str) {
    static int number = 0;
    sprintf(str, ".L%d", number++);
}

static void gen_lval(Node *node) {
    switch (node->kind) {
    case ND_LVAR:
        printf("    mov rax, rbp\n");
        printf("    sub rax, %d\n", node->offset);
        printf("    push rax\n");
        return;
    case ND_DEREF:
        switch (node->lhs->ty.ty) {
        case T_ARRAY:
            gen_lval(node->lhs);
            break;
        default:
            gen(node->lhs);
            break;
        }
        return;
    default:
        gen(node);
        return;
        // error("lhs of assignment is neither LVAR nor DEREF: kind: %d\n", node->kind);
    }
}

static int calculate_arithmetic_step(Type ty) {
    switch (ty.ty) {
    case T_INT:
        return 1;
    case T_PTR:
    case T_ARRAY:
        switch (ty.ptr_to->ty) {
        case T_INT:
            return 4;
        case T_PTR:
        case T_ARRAY:
            return 8;
        }
    }
}

int calculate_offset(Type ty) {
    switch (ty.ty) {
    case T_INT:
        return 4;
    case T_PTR:
        return 8;
    case T_ARRAY: {
        // fprintf(stderr, "T_ARRAY: offset: %d * %d\n", ty.array_size, calculate_offset(*ty.ptr_to));
        return ty.array_size * calculate_offset(*ty.ptr_to);
    }
    }
}

void gen(Node *node) {
    switch (node->kind) {
    case ND_NUM:
        printf("    push %d\n", node->val);
        return;
    case ND_LVAR:
        gen_lval(node);
        if (node->ty.ty != T_ARRAY) {
            printf("    pop rax\n");
            switch (node->ty.ty) {
            case T_INT:
                printf("    push rdi\n");
                printf("    mov rdi, rax\n");
                printf("    xor rax, rax\n");
                printf("    mov eax, DWORD PTR [rdi]\n");
                printf("    pop rdi\n");
                break;
            case T_PTR:
                printf("    mov rax, [rax]\n");
                break;
            }
            printf("    push rax\n");
        }
        return;
    case ND_ASSIGN:
        gen_lval(node->lhs);
        gen(node->rhs);

        printf("    pop rdi\n");
        printf("    pop rax\n");
        switch (node->lhs->ty.ty) {
        case T_PTR:
        case T_ARRAY:
            printf("    mov QWORD PTR [rax], rdi\n");
            break;
        case T_INT:
            printf("    mov DWORD PTR [rax], edi\n");
            break;
        }
        printf("    push rdi\n");
        return;
    case ND_RETURN:
        gen(node->lhs);
        printf("    pop rax\n");
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
        return;
    case ND_IF: {
        // gen condition
        gen(node->children[0]);
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        char *end_label = malloc(10);
        gen_unique_label(end_label);
        if (node->n_children == 2) {
            // without else clause
            printf("    je %s\n", end_label);
            // gen statement
            gen(node->children[1]);
            printf("%s:\n", end_label);
        } else {
            // with else clause
            char *else_label = malloc(10);
            gen_unique_label(else_label);
            // gen statement
            printf("    je %s\n", else_label);
            gen(node->children[1]);
            printf("    jmp %s\n", end_label);
            printf("%s:\n", else_label);
            gen(node->children[2]);
            printf("%s:\n", end_label);
        }
        return;
    }
    case ND_WHILE: {
        char *begin_label = malloc(10);
        char *end_label = malloc(10);
        gen_unique_label(begin_label);
        gen_unique_label(end_label);
        printf("%s:\n", begin_label);
        gen(node->children[0]);
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je %s\n", end_label);
        gen(node->children[1]);
        // TODO: need to pop?
        printf("    jmp %s\n", begin_label);
        printf("%s:\n", end_label);
        return;
    }
    case ND_FOR: {
        char *begin_label = malloc(10);
        char *end_label = malloc(10);
        gen_unique_label(begin_label);
        gen_unique_label(end_label);
        if (node->children[0] != NULL) {
            gen(node->children[0]);
        }
        printf("%s:\n", begin_label);
        if (node->children[1] != NULL) {
            gen(node->children[1]);
        } else {
            printf("    mov rax, 1\n");
            printf("    push rax\n");
        }
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je %s\n", end_label);
        gen(node->children[3]);
        if (node->children[2] != NULL) {
            gen(node->children[2]);
            printf("    pop rax\n");
        }
        printf("    jmp %s\n", begin_label);
        printf("%s:\n", end_label);
        return;
    }
    case ND_BLOCK: {
        for (int i=0; i<node->n_children; i++) {
            gen(node->children[i]);
            printf("    pop rax\n");
        }
        return;
    }
    case ND_CALL: {
        char funcname[100];
        memcpy(funcname, node->funcname, node->funcname_len);
        funcname[node->funcname_len] = '\0';
        // arguments TODO: 7 or more arguments
        for (int i=0; i<node->n_children; i++) {
            gen(node->children[i]);
        }
        const char* registers[] = {
            "rdi", "rsi", "rdx", "rcx", "r8", "r9"
        };
        for (int i=node->n_children-1; i>=0; i--) {
            printf("    pop %s\n", registers[i]);
        }
        // call
        printf("    call %s\n", funcname);
        printf("    push rax\n");
        return;
    }
    case ND_DEF: {
        char funcname[100];
        memcpy(funcname, node->funcname, node->funcname_len);
        funcname[node->funcname_len] = '\0';

        printf("%s:\n", funcname);

        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");

        // count the number of LVars
        int offset = 8;
        for (LVar *var=locals; var; var=var->next) {
            offset += calculate_offset(var->ty);
        }
        printf("    sub rsp, %d\n", offset);

        // registers where arguments are stored
        const char* registers[] = {
            "edi", "esi", "edx", "ecx", "r8", "r9"
        };

        for (int i=0; i<node->n_parameters; i++) {
            char param_name[100];
            memcpy(param_name, node->parameters[i], node->parameters_len[i]);
            param_name[node->parameters_len[i]] = 0;
            LVar *param = find_lvar_str(param_name, funcname, node->funcname_len);
            printf("    mov rax, rbp\n");
            printf("    sub rax, %d\n", param->offset);
            printf("    mov DWORD PTR [rax], %s\n", registers[i]);
        }

        for (int i=0; i<node->lhs->n_children; i++) {
            gen(node->lhs->children[i]);
        }
        printf("    pop rax\n");
        return;
    }
    case ND_ADDR:
        gen_lval(node->lhs);
        return;
    case ND_DEREF:
        gen(node->lhs);
        printf("    pop rax\n");
        printf("    mov rax, [rax]\n");
        printf("    push rax\n");
        return;
    case ND_EMPTY:
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind) {
    case ND_ADD: {
        int step = calculate_arithmetic_step(node->lhs->ty);
        if (step != 1) {
            printf("    imul rdi, %d\n", step);
        }
        printf("    add rax, rdi\n");
        break;
        break;
    }
    case ND_SUB: {
        int step = calculate_arithmetic_step(node->lhs->ty);
        if (step != 1) {
            printf("    imul rdi, %d\n", step);
        }
        printf("    sub rax, rdi\n");
        break;
    }
    case ND_MUL:
        printf("    imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("    cqo\n");
        printf("    idiv rdi\n");
        break;
    case ND_LT:
        printf("    cmp rax, rdi\n");
        printf("    setl al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LE:
        printf("    cmp rax, rdi\n");
        printf("    setle al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_EQ:
        printf("    cmp rax, rdi\n");
        printf("    sete al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_NEQ:
        printf("    cmp rax, rdi\n");
        printf("    setne al\n");
        printf("    movzb rax, al\n");
        break;  
    }

    printf("    push rax\n");
}
