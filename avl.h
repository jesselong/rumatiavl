#ifndef RUMATI_AVL_H
#define RUMATI_AVL_H 1

/*
 * This macro allows you to define a prefix for all public functions
 * of this library. Useful for defining to static, or Windows DLL stuff.
 */
#ifndef RUMATI_AVL_API
#define RUMATI_AVL_API
#endif

/*
 * The basic type for AVL trees. This is the opaque context passed to all
 * library methods.
 */
typedef struct rumati_avl_tree RUMATI_AVL_TREE;

/*
 * Error codes returned by this library
 */
typedef enum {
    RUMATI_AVL_OK,         /* all ok */
    RUMATI_AVL_ENOMEM,     /* malloc failure */
    RUMATI_AVL_EINVAL,     /* invalid parameter, probably NULL */
    RUMATI_AVL_ENOENT      /* no such element */
} RUMATI_AVL_ERROR;

/*
 * A function to compare node values in a tree. This function should return
 * integers less than zero if value1 is ordered before value2, zero if the
 * values are equal, or values greater than zero if value1 if sorted after
 * value2.
 */
typedef int(*RUMATI_AVL_COMPARATOR)(void *udata, void *value1, void *value2);

/*
 * A function that destructs values in a tree. Typically used to destroy or
 * clear trees. Each node value is passed to the function to be destroyed.
 */
typedef void(*RUMATI_AVL_NODE_DESTRUCTOR)(void *udata, void *value);

RUMATI_AVL_API
RUMATI_AVL_ERROR rumati_avl_new(RUMATI_AVL_TREE **tree, RUMATI_AVL_COMPARATOR comparator, void *udata);

RUMATI_AVL_API
void rumati_avl_destroy(RUMATI_AVL_TREE *tree, RUMATI_AVL_NODE_DESTRUCTOR destructor);

RUMATI_AVL_API
void rumati_avl_clear(RUMATI_AVL_TREE *tree, RUMATI_AVL_NODE_DESTRUCTOR destructor);

RUMATI_AVL_API
void *rumati_avl_get(RUMATI_AVL_TREE *tree, void *key);

RUMATI_AVL_API
RUMATI_AVL_ERROR rumati_avl_put(RUMATI_AVL_TREE *tree, void *object, void **old_value);

RUMATI_AVL_API
RUMATI_AVL_ERROR rumati_avl_delete(RUMATI_AVL_TREE *tree, void *object, void **old_value);

void *rumati_avl_get_greater_than_or_equal(RUMATI_AVL_TREE *tree, void *key);
void *rumati_avl_get_less_than_or_equal(RUMATI_AVL_TREE *tree, void *key);
void *rumati_avl_get_greater_than(RUMATI_AVL_TREE *tree, void *key);
void *rumati_avl_get_less_than(RUMATI_AVL_TREE *tree, void *key);

void *rumati_avl_get_smallest(RUMATI_AVL_TREE *tree);
void *rumati_avl_get_greatest(RUMATI_AVL_TREE *tree);

#endif /* RUMATI_AVL_H */
