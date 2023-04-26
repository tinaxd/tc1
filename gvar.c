#include "tc1.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

GVar *globals = NULL;

void register_gvar(const char *name, int name_len, Type ty)
{
    GVar *new_gvar = malloc(sizeof(GVar));
    new_gvar->name = malloc(name_len);
    memcpy(new_gvar->name, name, name_len);
    new_gvar->len = name_len;
    new_gvar->ty = ty;

    new_gvar->next = globals;
    globals = new_gvar;
}

Node *new_node_gvar(Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_GVAR;

    char *tok_name = malloc(tok->len + 1);
    memcpy(tok_name, tok->str, tok->len);
    tok_name[tok->len] = '\0';

    GVar *var = find_gvar_str(tok_name);
    if (var) {
        node->ty = var->ty;
        node->gname = var->name;
        node->gname_len = var->len;
    } else {
        return NULL;
    }
    // fprintf(stderr, "tok %c offset %d\n", tok->str[0], var->offset);

    return node;
}

GVar *find_gvar_str(const char *name){
    GVar *g = globals;
    int name_len = strlen(name);
    while (g != NULL) {
        if (g->len == name_len && memcmp(g->name, name, name_len) == 0) {
            return g;
        }
    }
    return NULL;
}