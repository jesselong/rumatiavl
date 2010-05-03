/*
 * Rumati AVL
 * Copyright (c) 2010 Jesse Long <jpl@unknown.za.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
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
    RUMATI_AVL_OK,          /* all ok */
    RUMATI_AVL_ENOMEM,      /* malloc failure */
    RUMATI_AVL_EINVAL,      /* invalid parameter, probably NULL */
    RUMATI_AVL_ENOENT,      /* no such element */
    RUMATI_AVL_ETOOBIG      /* tree too big */
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

/*
 * rumati_avl_new() - creates a new AVL tree.
 *
 * Parameters:
 *      tree -  a pointer to a pointer to an AVL tree. This will be populated
 *              with a pointer to the new tree if create successfully.
 *      comparator -    a function that compares node values, for sorting.
 *      udata - a user defined pointer to be passed to the comparator and
 *              other user callback functions.
 *
 * Returns:
 *      RUMATI_AVL_OK       On success
 *      RUMATI_AVL_EINVAL   If comparator is NULL or tree is NULL
 *      RUMATI_AVL_ENOMEM   If there was a memory allocation error.
 */
RUMATI_AVL_API
RUMATI_AVL_ERROR rumati_avl_new(
        RUMATI_AVL_TREE **tree,
        RUMATI_AVL_COMPARATOR comparator,
        void *udata);

/*
 * rumati_avl_clear() - removes all nodes from the tree, using the destructor
 * provided.
 *
 * Parameters:
 *      tree -  the tree to clear
 *      destructor -    the destructor to use when destroying each node's data
 */
RUMATI_AVL_API
void rumati_avl_clear(
        RUMATI_AVL_TREE *tree,
        RUMATI_AVL_NODE_DESTRUCTOR destructor);

/*
 * rumati_avl_destroy() - destroys a tree, freeing all memory used by the tree,
 * and cleanly destrying node data using a destructor.
 * provided.
 *
 * Parameters:
 *      tree -  the tree to destroy
 *      destructor -    the destructor to use when destroying each node's data
 */
RUMATI_AVL_API
void rumati_avl_destroy(
        RUMATI_AVL_TREE *tree,
        RUMATI_AVL_NODE_DESTRUCTOR destructor);

/*
 * rumati_avl_put() - inserts an entry into the tree, replacing an existing
 * entry if one exists.
 *
 * Parameters:
 *      tree -      The tree to which to add the entry.
 *      entry -     The entry to add to the tree.
 *      old_value - A pointer to a pointer which will be populated with the 
 *                  the previous value for the entry if one exists, or NULL
 *                  if there was previously no matching entry.
 * 
 * Returns:
 *      RUMATI_AVL_OK       On success
 *      RUMATI_AVL_ETOOBIG  If the tree is to big.
 *      RUMATI_AVL_ENOMEM   If there was an error allocating memory.
 */
RUMATI_AVL_API
RUMATI_AVL_ERROR rumati_avl_put(
        RUMATI_AVL_TREE *tree,
        void *object,
        void **old_value);

RUMATI_AVL_API
void *rumati_avl_get(RUMATI_AVL_TREE *tree, void *key);

void *rumati_avl_get_greater_than_or_equal(RUMATI_AVL_TREE *tree, void *key);

void *rumati_avl_get_less_than_or_equal(RUMATI_AVL_TREE *tree, void *key);

void *rumati_avl_get_greater_than(RUMATI_AVL_TREE *tree, void *key);

void *rumati_avl_get_less_than(RUMATI_AVL_TREE *tree, void *key);

RUMATI_AVL_API
RUMATI_AVL_ERROR rumati_avl_delete(RUMATI_AVL_TREE *tree, void *object, void **old_value);

void *rumati_avl_get_smallest(RUMATI_AVL_TREE *tree);

void *rumati_avl_get_greatest(RUMATI_AVL_TREE *tree);

#endif /* RUMATI_AVL_H */
