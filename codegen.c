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
    case ND_GVAR:
        char gname[100];
        memcpy(gname, node->gname, node->gname_len);
        gname[node->gname_len] = '\0';
        printf("    lea rax, %s[rip]\n", gname);
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
        printf("    # ND_NUM\n");
        printf("    push %d\n", node->val);
        return;
    case ND_LVAR:
        printf("    # ND_LVAR\n");
        gen_lval(node);
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
        return;
    case ND_GVAR:
        printf("    # ND_GVAR\n");
        gen_lval(node);
        printf("    pop rax\n");
        printf("    mov rax, [rax]\n");
        printf("    push rax\n");
        return;
    case ND_ASSIGN:
        printf("    # ND_ASSIGN\n");
        printf("    # ND_ASSIGN lhs\n");
        gen_lval(node->lhs);
        printf("    # ND_ASSIGN rhs\n");
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
        printf("    # ND_RETURN\n");
        gen(node->lhs);
        printf("    pop rax\n");
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
        return;
    case ND_IF: {
        printf("    # ND_IF\n");
        // gen condition
        printf("    # ND_IF condition\n");
        gen(node->children[0]);
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        char *end_label = malloc(10);
        gen_unique_label(end_label);
        if (node->n_children == 2) {
            // without else clause
            printf("    je %s\n", end_label);
            // gen statement
            printf("    # ND_IF then\n");
            gen(node->children[1]);
            printf("    # ND_IF end\n");
            printf("%s:\n", end_label);
        } else {
            // with else clause
            char *else_label = malloc(10);
            gen_unique_label(else_label);
            // gen statement
            printf("    je %s\n", else_label);
            printf("    # ND_IF then\n");
            gen(node->children[1]);
            printf("    jmp %s\n", end_label);
            printf("    # ND_IF else\n");
            printf("%s:\n", else_label);
            gen(node->children[2]);
            printf("    # ND_IF end\n");
            printf("%s:\n", end_label);
        }
        return;
    }
    case ND_WHILE: {
        printf("    # ND_WHILE\n");
        char *begin_label = malloc(10);
        char *end_label = malloc(10);
        gen_unique_label(begin_label);
        gen_unique_label(end_label);
        printf("%s:\n", begin_label);
        printf("    # ND_WHILE stmt\n");
        gen(node->children[0]);
        printf("    # ND_WHILE cmp\n");
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
        printf("    # ND_FOR\n");
        char *begin_label = malloc(10);
        char *end_label = malloc(10);
        gen_unique_label(begin_label);
        gen_unique_label(end_label);
        if (node->children[0] != NULL) {
            printf("    # ND_FOR init\n");
            gen(node->children[0]);
        }
        printf("%s:\n", begin_label);
        if (node->children[1] != NULL) {
            printf("    # ND_FOR cond\n");
            gen(node->children[1]);
        } else {
            printf("    # ND_FOR cond (always 1)\n");
            printf("    mov rax, 1\n");
            printf("    push rax\n");
        }
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je %s\n", end_label);
        printf("    # ND_FOR stmt\n");
        gen(node->children[3]);
        if (node->children[2] != NULL) {
            printf("    # ND_FOR step\n");
            gen(node->children[2]);
            printf("    pop rax\n");
        }
        printf("    jmp %s\n", begin_label);
        printf("%s:\n", end_label);
        return;
    }
    case ND_BLOCK: {
        for (int i=0; i<node->n_children; i++) {
            printf("    # ND_BLOCK stmt %d\n", i);
            gen(node->children[i]);
            // printf("    pop rax\n");
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
        int offset = 0;
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
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_LT:
    case ND_LE:
    case ND_EQ:
    case ND_NEQ:
        goto SWITCH_END;
    case ND_GVAR_DECL:
        char gname[100];
        memcpy(gname, node->gname, node->gname_len);
        gname[node->gname_len] = 0;
        printf("%s:\n", gname);
        printf("\t.zero %d\n", calculate_sizeof(node->ty));
        return;
    default:
        // unknown node
        // show error to stderr and exit
        fprintf(stderr, "unknown node kind: %d\n", node->kind);
        exit(1);
    }
    SWITCH_END:

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind) {
    case ND_ADD: {
        int step = calculate_arithmetic_step(node->ty);
        if (step != 1) {
            if (node->lhs->ty.ty != T_PTR && node->lhs->ty.ty != T_ARRAY) {
                printf("    imul rax, %d\n", step);
            } else {
                printf("    imul rdi, %d\n", step);
            }
        }
        printf("    add rax, rdi\n");
        break;
        break;
    }
    case ND_SUB: {
        int step = calculate_arithmetic_step(node->ty);
        if (step != 1) {
            if (node->lhs->ty.ty != T_PTR && node->lhs->ty.ty != T_ARRAY) {
                printf("    imul rax, %d\n", step);
            } else {
                printf("    imul rdi, %d\n", step);
            }
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
