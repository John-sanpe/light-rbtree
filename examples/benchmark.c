/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include "rbtree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/times.h>

#define RB_DEBUG    0
#define RB_CACHED   1
#define TEST_LEN    1000000

struct bench_node {
    struct rb_node rb;
    unsigned int num;
    unsigned long data;
};

#define rb_to_bench(node) \
    rb_entry_safe(node, struct bench_node, rb)

#if RB_DEBUG
static void node_dump(struct bench_node *node)
{
    printf("  %04d: ", node->num);
    printf("parent %-4d ", node->rb.parent ? rb_to_bench(node->rb.parent)->num : 0);
    printf("left %-4d ", node->rb.left ? rb_to_bench(node->rb.left)->num : 0);
    printf("right %-4d ", node->rb.right ? rb_to_bench(node->rb.right)->num : 0);
    printf("data 0x%16lx ", node->data);
    printf("color'%s' ", node->rb.color ? "black" : "red");
    printf("\n");
}
#else
# define node_dump(node) ((void)(node))
#endif

#if RB_CACHED
static RB_ROOT_CACHED(bench_root);
# define bc_insert                      rb_cached_insert
# define bc_delete                      rb_cached_delete
# define bc_for_each_entry              rb_cached_for_each_entry
# define bc_post_for_each_entry         rb_cached_post_for_each_entry
# define bc_post_for_each_entry_safe    rb_cached_post_for_each_entry_safe
# define bc_deepth(cached)              test_deepth((cached)->root.rb_node)
#else
static RB_ROOT(bench_root);
# define bc_insert                      rb_insert
# define bc_delete                      rb_delete
# define bc_for_each_entry              rb_for_each_entry
# define bc_post_for_each_entry         rb_post_for_each_entry
# define bc_post_for_each_entry_safe    rb_post_for_each_entry_safe
# define bc_deepth(root)                test_deepth((root)->rb_node)
#endif

static void time_dump(int ticks, clock_t start, clock_t stop, struct tms *start_tms, struct tms *stop_tms)
{
    printf("  real time: %lf\n", (stop - start) / (double)ticks);
    printf("  user time: %lf\n", (stop_tms->tms_utime - start_tms->tms_utime) / (double)ticks);
    printf("  kern time: %lf\n", (stop_tms->tms_stime - start_tms->tms_stime) / (double)ticks);
}

static unsigned int test_deepth(struct rb_node *node)
{
    unsigned int left_deepth, right_deepth;

    if (!node)
        return 0;

    left_deepth = test_deepth(node->left);
    right_deepth = test_deepth(node->right);
    return left_deepth > right_deepth ? (left_deepth + 1) : (right_deepth + 1);
}

static long demo_cmp(const struct rb_node *a, const struct rb_node *b)
{
    struct bench_node *demo_a = rb_to_bench(a);
    struct bench_node *demo_b = rb_to_bench(b);
    return demo_a->data - demo_b->data;
}

int main(void)
{
    struct bench_node *node, *tmp;
    struct tms start_tms, stop_tms;
    clock_t start, stop;
    unsigned int count, ticks;
    int ret = 0;

    ticks = sysconf(_SC_CLK_TCK);

    printf("Generate %d Node:\n", TEST_LEN);
    start = times(&start_tms);
    for (count = 0; count < TEST_LEN; ++count) {
        node = malloc(sizeof(*node));
        if ((ret = !node)) {
            printf("Insufficient Memory!\n");
            goto error;
        }

        node->num = count + 1;
        node->data = ((unsigned long)rand() << 32) | rand();

#if RB_DEBUG
        printf("  %08d: 0x%016lx\n", node->num, node->data);
#endif

        bc_insert(&bench_root, &node->rb, demo_cmp);
    }
    stop = times(&stop_tms);
    time_dump(ticks, start, stop, &start_tms, &stop_tms);

    count = bc_deepth(&bench_root);
    printf("  rb deepth: %d\n", count);

    /* Start detection middle order iteration. */
    start = times(&start_tms);
    printf("Middle Iteration:\n");
    bc_for_each_entry(node, &bench_root, rb)
        node_dump(node);
    stop = times(&stop_tms);
    time_dump(ticks, start, stop, &start_tms, &stop_tms);

    /* Start detection postorder order iteration. */
    start = times(&start_tms);
    printf("Postorder Iteration:\n");
    bc_post_for_each_entry(node, &bench_root, rb)
        node_dump(node);
    stop = times(&stop_tms);
    time_dump(ticks, start, stop, &start_tms, &stop_tms);

    printf("Deletion All Node...\n");
error:
    bc_post_for_each_entry_safe(node, tmp, &bench_root, rb) {
        bc_delete(&bench_root, &node->rb);
        free(node);
    }

    if (!ret)
        printf("Done.\n");
    else
        printf("Abort.\n");

    return ret;
}