#include "tc1.h"
#include <assert.h>

int calculate_sizeof(Type ty) {
    switch (ty.ty) {
    case T_INT:
        return 4;
    case T_PTR:
        return 8;
    case T_ARRAY:
        // TODO:
        assert(ty.ptr_to != NULL);
        return ty.array_size * calculate_sizeof(*ty.ptr_to);
    }
}
