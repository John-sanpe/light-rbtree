/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#ifndef _RBTREE_H_
#define _RBTREE_H_

#include "macro.h"

#define RB_RED    0
#define RB_BLACK  1

typedef struct rb_node {
    struct rb_node *parent;
    struct rb_node *left;
    struct rb_node *right;
    bool color;
} rb_node_t;

struct rb_root {
    struct rb_node *rb_node;
};

#define RB_ROOT(name) \
    struct rb_root name = {NULL};

#define	rb_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define rb_entry_safe(ptr, type, member) ({     \
    typeof(ptr) _ptr = (ptr);                   \
	_ptr ? rb_entry(_ptr, type, member) : NULL; \
})

extern void rb_fixup(struct rb_root *root, struct rb_node *node);
extern void rb_erase(struct rb_root *root, struct rb_node *parent);
extern void rb_replace(struct rb_root *root, struct rb_node *old, struct rb_node *new);
extern struct rb_node *rb_find(const struct rb_root *root, const void *key, long (*cmp)(const struct rb_node *, const void *key));
extern struct rb_node **rb_parent(struct rb_root *root, struct rb_node **parentp, struct rb_node *node, long (*cmp)(const struct rb_node *, const struct rb_node *));

/**
 * rb_link - link node to parent.
 * @parent: point to parent node.
 * @link: point to pointer to child node.
 * @node: new element to link.
 */
static inline void rb_link(struct rb_node *parent, struct rb_node **link, struct rb_node *node)
{
    /* link = &parent->left/right */
    *link = node;
    node->parent = parent;
    node->left = node->right = NULL;
}

/**
 * rb_insert_node - link node to parent and fixup rbtree.
 * @root: rbtree root of element.
 * @parent: parent node of element.
 * @link: point to pointer to child node.
 * @node: new element to link.
 */
static inline void rb_insert_node(struct rb_root *root, struct rb_node *parent,
                                  struct rb_node **link, struct rb_node *node)
{
    rb_link(parent, link, node);
    rb_fixup(root, node);
}

/**
 * rb_find_and_insert - find the parent node and insert new node.
 * @root: rbtree root of element.
 * @node: new element to insert.
 * @cmp: operator defining the node order.
 */
static inline state rb_insert(struct rb_root *root, struct rb_node *node,
                              long (*cmp)(const struct rb_node *, const struct rb_node *))
{
    struct rb_node *parent, **link;

    link = rb_parent(root, &parent, node, cmp);
    if (unlikely(!link))
        return -EINVAL;

    rb_insert_node(root, parent, link, node);
    return -ENOERR;
}

/* Middle iteration (Sequential) - find logical next and previous nodes */
extern struct rb_node *rb_first(const struct rb_root *root);
extern struct rb_node *rb_last(const struct rb_root *root);
extern struct rb_node *rb_prev(const struct rb_node *node);
extern struct rb_node *rb_next(const struct rb_node *node);

#define rb_first_entry(ptr, type, member) \
    rb_entry_safe(rb_first(ptr), type, member)

#define rb_next_entry(pos, member) \
    rb_entry_safe(rb_next(&(pos)->member), typeof(*(pos)), member)

#define rb_for_each_entry(pos, root, member)                    \
    for (pos = rb_first_entry(root, typeof(*pos), member);      \
         pos; pos = rb_next_entry(pos, member))

/* Postorder iteration (Depth-first) - always visit the parent after its children */
extern struct rb_node *rb_post_first(const struct rb_root *root);
extern struct rb_node *rb_post_next(const struct rb_node *node);

#define rb_post_first_entry(ptr, type, member) \
    rb_entry_safe(rb_post_first(ptr), type, member)

#define rb_post_next_entry(pos, member) \
    rb_entry_safe(rb_post_next(&(pos)->member), typeof(*(pos)), member)

#define rb_post_for_each_entry(pos, root, member)               \
    for (pos = rb_post_first_entry(root, typeof(*pos), member); \
         pos; pos = rb_post_next_entry(pos, member))

#define rb_post_for_each_entry_safe(pos, next, root, member)    \
    for (pos = rb_post_first_entry(root, typeof(*pos), member); \
         pos && ({ next = rb_post_next_entry(pos, member);      \
         1; }); pos = next)

#endif  /* _RBTREE_H_ */
