#include "avl.c"

#include <stdio.h>

static int intcmp(void *udata, void *value1, void *value2)
{
    int *i1 = (int*)value1;
    int *i2 = (int*)value2;

    (void)udata;

    if (*i1 > *i2){
        return 1;
    }else if (*i1 < *i2){
        return -1;
    }

    return 0;
}

static void nulldestructor(void *udata, void *value)
{
    (void)udata;
    (void)value;
}

int main(int argc, char *argv[])
{
    RUMATI_AVL_TREE *tree;
    RUMATI_AVL_ERROR err;

    (void)argc;
    (void)argv;

    if ((err = rumati_avl_new(&tree, intcmp, NULL)) != RUMATI_AVL_OK){
        printf("Error creating AVL tree: %d\n", err);
        return 1;
    }

    rumati_avl_put(tree, &i, NULL);

    rumati_avl_destroy(tree, nulldestructor);

    return 0;
}
