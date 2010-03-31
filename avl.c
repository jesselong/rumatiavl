/*
 * TODO:
 *  *   Add error code for when the tree gets too large, and check that we
 *      dontover run the updates array when inserting or deleting
 */
#include "avl.h"

#include <stdlib.h>     /* for malloc(), free() */
#include <stdint.h>     /* for int8_t */
#include <stdbool.h>    /* for bool */

#define RUMATI_AVL_MAX_HEIGHT   65

/*
 * Tree type
 */
struct rumati_avl_tree {
    /*
     * comparator, function to compare values
     */
    RUMATI_AVL_COMPARATOR comparator;
    /*
     * User provided pointer
     */
    void *udata;
    /*
     * Root node in tree, NULL initially
     */
    struct rumati_avl_node *root;
};

/*
 * Tree node structure. Each value stored in the tree has an associated node.
 */
struct rumati_avl_node {
    /*
     * Link to left child (smaller value), or NULL is there is no left child.
     */
    struct rumati_avl_node *left;
    /*
     * Link to right child (greater value), or NULL is there is no right child.
     */
    struct rumati_avl_node *right;
    /*
     * Difference in height of sub trees. If left subtree is 1 layer higher
     * than the right subtree, then balance is -1. Balance is +1 if the right
     * subtree is one layer higher then the left subtree. AVL rules specify
     * that one subtree may not be more than one layer higher than the other
     * subtree. However, during double rotations, it is possible for the first
     * node to have a balance >= +2 or <= -2.
     */
    int8_t balance;
    /*
     * The data held by this node.
     */
    void *data;
};

/*
 * A linked list of nodes to update after insert or delete. This represents
 * the path taken down the tree to the node that was added or deleted.
 */
struct rumati_avl_node_update {
    /*
     * A pointer to the pointer to the node. We need pointer to pointer
     * because the pointer to the node needs to be updated when we rotate
     * the node to rebalance it.
     */
    struct rumati_avl_node **node_ptr;
    /*
     * A boolean indicating if the inserted/deleted node is in the left subtree
     * of the node, or not. If false, the inserted/deleted node is in the right
     * subtree.
     */
    bool left;
};

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
        void *udata)
{
    RUMATI_AVL_TREE *retv;

    /*
     * comparator cannot be NULL, otherwise we would be unable to compare
     * node values.
     * tree cannot be NULL, other we are guarenteed to have a memory leak.
     */
    if (comparator == NULL || tree == NULL){
        return RUMATI_AVL_EINVAL;
    }

    retv = malloc(sizeof(*retv));
    if (retv == NULL){
        return RUMATI_AVL_ENOMEM;
    }

    retv->comparator = comparator;
    retv->udata = udata;
    retv->root = NULL;

    *tree = retv;
    return RUMATI_AVL_OK;
}

/*
 * rumati_avl_destroy_node() - destroys a single node by invoking a destructor
 * on the node's data, and free()ing the node.
 *
 * Parameters:
 *      n -     the node to destroy
 *      destructor -    the destrctor to use to destroy the nodes data
 *      udata - the user supplied pointer associated with the tree
 */
static void rumati_avl_destroy_node(
        struct rumati_avl_node *n,
        RUMATI_AVL_NODE_DESTRUCTOR destructor,
        void *udata)
{
    destructor(udata, n->data);
    free(n);
}

/*
 * rumati_avl_destroy_node_recursive() - recursively destroys a node, and all
 * its children using rumati_avl_node_destroy().
 *
 * Parameters:
 *      n -     the node to destroy, along with all its children
 *      destructor -    the destrctor to use to destroy the nodes data
 *      udata - the user supplied pointer associated with the tree
 */
static void rumati_avl_destroy_node_recursive (
        struct rumati_avl_node *n,
        RUMATI_AVL_NODE_DESTRUCTOR destructor,
        void *udata)
{
    if (n->left != NULL){
        rumati_avl_destroy_node_recursive(n->left, destructor, udata);
    }
    if (n->right != NULL){
        rumati_avl_destroy_node_recursive(n->right, destructor, udata);
    }
    rumati_avl_destroy_node(n, destructor, udata);
} 

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
        RUMATI_AVL_NODE_DESTRUCTOR destructor)
{
    if (tree->root != NULL){
        rumati_avl_destroy_node_recursive(tree->root, destructor, tree->udata);
    }
}

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
        RUMATI_AVL_NODE_DESTRUCTOR destructor)
{
    rumati_avl_clear(tree, destructor);
    free(tree);
}

/*
 * rumati_avl_rotate_right() - rotates a subtree to the right/clock wise
 *
 * Parameters:
 *      node_ptr -  A pointer to the pointer to the root of this subtree
 *                  this will be updated to point to the new root, after
 *                  rotation.
 */
static void rumati_avl_rotate_right(struct rumati_avl_node **node_ptr)
{
    int8_t orb, nrb;

    /* keep reference to old root */
    struct rumati_avl_node *old_root = *node_ptr;

    /* make root pointer point to new root, old root's left child */
    *node_ptr = old_root->left;
    /* 
     * old root inherits new root's right child as it's left child
     * remember, its previous left child is now its parent
     */
    old_root->left = (*node_ptr)->right;
    /* new root's right child is the old root */
    (*node_ptr)->right = old_root;

    /* old root balance pre rotate */
    orb = old_root->balance;
    /* new root balance pre rotate */
    nrb = (*node_ptr)->balance;

    /*
     * By default, we increment balance, or in other words, make the balance
     * one level heavier towards the right. This is because the left subtree
     * has lost a layer - its left child is the right child of its old left
     * child, hence we have lost the layer of the old left child.
     */
    old_root->balance++;
    if (nrb < 0){
        old_root->balance -= nrb;
    }

    (*node_ptr)->balance++;
    if (old_root->balance > 0){
        (*node_ptr)->balance += old_root->balance;
    }
}

static void rumati_avl_rotate_left(struct rumati_avl_node **node_ptr)
{
    int8_t orb, nrb;
    struct rumati_avl_node *old_root = *node_ptr;

    *node_ptr = old_root->right;
    old_root->right = (*node_ptr)->left;
    (*node_ptr)->left = old_root;

    /* old root balance */
    orb = old_root->balance;
    /* new root balance */
    nrb = (*node_ptr)->balance;

    old_root->balance--;
    if (nrb > 0){
        old_root->balance -= nrb;
    }

    (*node_ptr)->balance--;
    if (old_root->balance < 0){
        (*node_ptr)->balance += old_root->balance;
    }
}

RUMATI_AVL_API
RUMATI_AVL_ERROR rumati_avl_put(RUMATI_AVL_TREE *tree, void *object, void **old_value)
{
    struct rumati_avl_node *n = NULL;
    struct rumati_avl_node **parent_link = &tree->root;
    struct rumati_avl_node_update updates[RUMATI_AVL_MAX_HEIGHT];
    int update_size = 0;

    while (*parent_link != NULL){
        int cmp = tree->comparator(tree->udata, object, (*parent_link)->data);
        if (cmp == 0){
            if (old_value != NULL){
                *old_value = (*parent_link)->data;
            }
            (*parent_link)->data = object;
            goto success;
        }else if (cmp > 0){
            updates[update_size].node_ptr = parent_link;
            updates[update_size].left = false;
            update_size++;
            parent_link = &(*parent_link)->right;
        }else if (cmp < 0){
            updates[update_size].node_ptr = parent_link;
            updates[update_size].left = true;
            update_size++;
            parent_link = &(*parent_link)->left;
        }
    }

    n = malloc(sizeof(*n));
    if (n == NULL){
        return RUMATI_AVL_ENOMEM;
    }
    n->left = NULL;
    n->right = NULL;
    n->balance = 0;
    n->data = object;

    *parent_link = n;

    /*
     * Do updates
     */
    while (update_size > 0){
        update_size--;
        if (updates[update_size].left){
            (*updates[update_size].node_ptr)->balance--;
            if ((*updates[update_size].node_ptr)->balance < -1){
                /*
                 * Do rotations, and then break, because no further updates are needed
                 *
                 * our left child cannot possibly have even balance, as no addition
                 * below our left child could possibly leave our left child in even
                 * balance, and leave us inbalanced at the same time.
                 */
                if ((*updates[update_size].node_ptr)->left->balance > 0){
                    rumati_avl_rotate_left(&(*updates[update_size].node_ptr)->left);
                }
                rumati_avl_rotate_right(updates[update_size].node_ptr);
                break;
            }else if ((*updates[update_size].node_ptr)->balance >= 0){
                break;
            }
        }else{
            (*updates[update_size].node_ptr)->balance++;
            if ((*updates[update_size].node_ptr)->balance > 1){
                if ((*updates[update_size].node_ptr)->right->balance < 0){
                    rumati_avl_rotate_right(&(*updates[update_size].node_ptr)->right);
                }
                rumati_avl_rotate_left(updates[update_size].node_ptr);
                break;
            }else if ((*updates[update_size].node_ptr)->balance <= 0){
                break;
            }
        }
    }

success:
    return RUMATI_AVL_OK;
}

RUMATI_AVL_API
void *rumati_avl_get(RUMATI_AVL_TREE *tree, void *key)
{
    struct rumati_avl_node *n = tree->root;

    while (n != NULL){
        int cmp = tree->comparator(tree->udata, key, n->data);
        if (cmp > 0){
            n = n->right;
        }else if (cmp < 0){
            n = n->left;
        }else{
            return n->data;
        }
    }

    return NULL;
}

void *rumati_avl_get_greater_than_or_equal(RUMATI_AVL_TREE *tree, void *key)
{
    struct rumati_avl_node *n = tree->root;
    struct rumati_avl_node *prev = NULL;

    while (n != NULL){
        int cmp = tree->comparator(tree->udata, key, n->data);
        if (cmp > 0){
            n = n->right;
        }else if (cmp < 0){
            prev = n;
            n = n->left;
        }else{
            return n->data;
        }
    }

    if (prev != NULL){
        return prev->data;
    }

    return NULL;
}

void *rumati_avl_get_less_than_or_equal(RUMATI_AVL_TREE *tree, void *key)
{
    struct rumati_avl_node *n = tree->root;
    struct rumati_avl_node *prev = NULL;

    while (n != NULL){
        int cmp = tree->comparator(tree->udata, key, n->data);
        if (cmp > 0){
            prev = n;
            n = n->right;
        }else if (cmp < 0){
            n = n->left;
        }else{
            return n->data;
        }
    }

    if (prev != NULL){
        return prev->data;
    }

    return NULL;
}

void *rumati_avl_get_greater_than(RUMATI_AVL_TREE *tree, void *key)
{
    struct rumati_avl_node *n = tree->root;
    struct rumati_avl_node *prev = NULL;

    while (n != NULL){
        int cmp = tree->comparator(tree->udata, key, n->data);
        if (cmp > 0){
            n = n->right;
        }else if (cmp < 0){
            prev = n;
            n = n->left;
        }else{
            if (n->right == NULL){
                break;
            }
            n = n->right;
            while (n->left != NULL){
                n = n->left;
            }
            return n->data;
        }
    }

    if (prev != NULL){
        return prev->data;
    }

    return NULL;
}

void *rumati_avl_get_less_than(RUMATI_AVL_TREE *tree, void *key)
{
    struct rumati_avl_node *n = tree->root;
    struct rumati_avl_node *prev = NULL;

    while (n != NULL){
        int cmp = tree->comparator(tree->udata, key, n->data);
        if (cmp > 0){
            prev = n;
            n = n->right;
        }else if (cmp < 0){
            n = n->left;
        }else{
            if (n->left == NULL){
                break;
            }
            n = n->left;
            while (n->right != NULL){
                n = n->right;
            }
            return n->data;
        }
    }

    if (prev != NULL){
        return prev->data;
    }

    return NULL;
}

RUMATI_AVL_API
RUMATI_AVL_ERROR rumati_avl_delete(RUMATI_AVL_TREE *tree, void *object, void **old_value)
{
    struct rumati_avl_node **parent_link = &tree->root;
    struct rumati_avl_node *delnode = NULL;
    struct rumati_avl_node_update updates[RUMATI_AVL_MAX_HEIGHT];
    int update_size = 0;

    while (1){
        int cmp;

        if (*parent_link == NULL){
            return RUMATI_AVL_ENOENT;
        }

        cmp = tree->comparator(tree->udata, object, (*parent_link)->data);
        if (cmp == 0){
            delnode = *parent_link;
            /*
             * Try delete in place if at least one child is missing
             */
            if (delnode->balance <= 0 && delnode->right == NULL){
                *parent_link = delnode->left;
                *old_value = delnode->data;
                free(delnode);
            }else if (delnode->balance >= 0 && delnode->left == NULL){
                *parent_link = delnode->right;
                *old_value = delnode->data;
                free(delnode);
            }else{
                if (delnode->balance < 0){
                    updates[update_size].node_ptr = parent_link;
                    updates[update_size].left = true;
                    update_size++;
                    parent_link = &(*parent_link)->left;
                    while ((*parent_link)->right != NULL){
                        updates[update_size].node_ptr = parent_link;
                        updates[update_size].left = false;
                        update_size++;
                        parent_link = &(*parent_link)->right;
                    }
                    *old_value = delnode->data;
                    delnode->data = (*parent_link)->data;
                    delnode = *parent_link;
                    *parent_link = delnode->left;
                    free(delnode);
                }else{
                    updates[update_size].node_ptr = parent_link;
                    updates[update_size].left = false;
                    update_size++;
                    parent_link = &(*parent_link)->right;
                    while ((*parent_link)->left != NULL){
                        updates[update_size].node_ptr = parent_link;
                        updates[update_size].left = true;
                        update_size++;
                        parent_link = &(*parent_link)->left;
                    }
                    *old_value = delnode->data;
                    delnode->data = (*parent_link)->data;
                    delnode = *parent_link;
                    *parent_link = delnode->right;
                    free(delnode);
                }
            }
            break;
        }else if (cmp > 0){
            updates[update_size].node_ptr = parent_link;
            updates[update_size].left = false;
            update_size++;
            parent_link = &(*parent_link)->right;
        }else if (cmp < 0){
            updates[update_size].node_ptr = parent_link;
            updates[update_size].left = true;
            update_size++;
            parent_link = &(*parent_link)->left;
        }
    }

    /*
     * Do updates
     */
    while (update_size > 0){
        struct rumati_avl_node_update *update;
        update_size--;
        update = &updates[update_size];
        if (update->left){
            (*update->node_ptr)->balance++;
            if ((*update->node_ptr)->balance > 1){
                if ((*update->node_ptr)->right->balance < 0){
                    rumati_avl_rotate_right(&(*update->node_ptr)->right);
                }
                rumati_avl_rotate_left(update->node_ptr);
                if ((*update->node_ptr)->balance <= 0 &&
                        (*update->node_ptr)->left->balance > 0){
                    break;
                }
            }else if ((*update->node_ptr)->balance > 0){
                break;
            }
        }else{
            (*update->node_ptr)->balance--;
            if ((*update->node_ptr)->balance < -1){
                if ((*update->node_ptr)->left->balance > 0){
                    rumati_avl_rotate_left(&(*update->node_ptr)->left);
                }
                rumati_avl_rotate_right(update->node_ptr);
                if ((*update->node_ptr)->balance >= 0 &&
                        (*update->node_ptr)->right->balance < 0){
                    break;
                }
            }else if ((*update->node_ptr)->balance < 0){
                break;
            }
        }
    }

    return RUMATI_AVL_OK;
}

void *rumati_avl_get_smallest(RUMATI_AVL_TREE *tree)
{
    struct rumati_avl_node *n = tree->root;
    
    if (n == NULL){
        return NULL;
    }

    while (n->left != NULL){
        n = n->left;
    }

    return n->data;
}

void *rumati_avl_get_greatest(RUMATI_AVL_TREE *tree)
{
    struct rumati_avl_node *n = tree->root;
    
    if (n == NULL){
        return NULL;
    }

    while (n->right != NULL){
        n = n->right;
    }

    return n->data;
}

