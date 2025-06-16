//===--- watch_list.h -------------------------------------------------------===
//
//                     satoko: Satisfiability solver
//
// This file is distributed under the BSD 2-Clause License.
// See LICENSE for details.
//
//===------------------------------------------------------------------------===
#ifndef satoko__watch_list_h
#define satoko__watch_list_h

#include "utils/mem.h"
#include "utils/misc.h"

#include "misc/util/abc_global.h"
ABC_NAMESPACE_HEADER_START

struct watcher {
    unsigned cref;
    unsigned blocker;
};

struct watch_list {
    unsigned cap;
    unsigned size;
    unsigned n_bin;
    struct watcher *watchers;
};

typedef struct vec_wl_t_ vec_wl_t;
struct vec_wl_t_ {
    unsigned cap;
    unsigned size;
    struct watch_list *watch_lists;
};

//===------------------------------------------------------------------------===
// Watch list Macros
//===------------------------------------------------------------------------===
#define watch_list_foreach(vec, watch, lit) \
    for (watch = watch_list_array(vec_wl_at(vec, lit)); \
         watch < watch_list_array(vec_wl_at(vec, lit)) + watch_list_size(vec_wl_at(vec, lit)); \
     watch++)

#define watch_list_foreach_bin(vec, watch, lit) \
    for (watch = watch_list_array(vec_wl_at(vec, lit)); \
         watch < watch_list_array(vec_wl_at(vec, lit)) + vec_wl_at(vec, lit)->n_bin; \
     watch++)
//===------------------------------------------------------------------------===
// Watch list API
//===------------------------------------------------------------------------===
static inline void watch_list_free(struct watch_list *wl)
{
    if (wl->watchers)
        satoko_free(wl->watchers);
}

static inline unsigned watch_list_size(struct watch_list *wl)
{
    return wl->size;
}

static inline void watch_list_shrink(struct watch_list *wl, unsigned size)
{
    assert(size <= wl->size);
    wl->size = size;
}

static inline void watch_list_grow(struct watch_list *wl)
{
    unsigned new_size = (wl->cap < 4) ? 4 : (wl->cap / 2) * 3;
    struct watcher *watchers =
        satoko_realloc(struct watcher, wl->watchers, new_size);
    if (watchers == NULL) {
        printf("Failed to realloc memory from %.1f MB to %.1f "
               "MB.\n",
               1.0 * wl->cap / (1 << 20),
               1.0 * new_size / (1 << 20));
        fflush(stdout);
        return;
    }
    wl->watchers = watchers;
    wl->cap = new_size;
}

static inline void watch_list_push(struct watch_list *wl, struct watcher w, unsigned is_bin)
{
    assert(wl);
    if (wl->size == wl->cap)
        watch_list_grow(wl);
    wl->watchers[wl->size++] = w;
    if (is_bin && wl->size > wl->n_bin) {
        stk_swap(struct watcher, wl->watchers[wl->n_bin], wl->watchers[wl->size - 1]);
        wl->n_bin++;
    }
}

static inline struct watcher *watch_list_array(struct watch_list *wl)
{
    return wl->watchers;
}

/* TODO: I still have mixed feelings if this change should be done, keeping the
 * old code commented after it. */
static inline void watch_list_remove(struct watch_list *wl, unsigned cref, unsigned is_bin)
{
    struct watcher *watchers = watch_list_array(wl);
    unsigned i;
    if (is_bin) {
        for (i = 0; watchers[i].cref != cref; i++);
        assert(i < watch_list_size(wl));
        wl->n_bin--;
        memmove((wl->watchers + i), (wl->watchers + i + 1),
                (wl->size - i - 1) * sizeof(struct watcher));
    } else {
        for (i = wl->n_bin; watchers[i].cref != cref; i++);
        assert(i < watch_list_size(wl));
        stk_swap(struct watcher, wl->watchers[i], wl->watchers[wl->size - 1]);
    }
    wl->size -= 1;
}

/*
static inline void watch_list_remove(struct watch_list *wl, unsigned cref, unsigned is_bin)
{
    struct watcher *watchers = watch_list_array(wl);
    unsigned i;
    if (is_bin) {
        for (i = 0; watchers[i].cref != cref; i++);
        wl->n_bin--;
    } else
        for (i = wl->n_bin; watchers[i].cref != cref; i++);
    assert(i < watch_list_size(wl));
    memmove((wl->watchers + i), (wl->watchers + i + 1),
            (wl->size - i - 1) * sizeof(struct watcher));
    wl->size -= 1;
}
*/

static inline vec_wl_t *vec_wl_alloc(unsigned cap)
{
    vec_wl_t *vec_wl = satoko_alloc(vec_wl_t, 1);

    if (cap == 0)
        vec_wl->cap = 4;
    else
        vec_wl->cap = cap;
    vec_wl->size = 0;
    vec_wl->watch_lists = satoko_calloc(
        struct watch_list, sizeof(struct watch_list) * vec_wl->cap);
    return vec_wl;
}

static inline void vec_wl_free(vec_wl_t *vec_wl)
{
    unsigned i;
    for (i = 0; i < vec_wl->cap; i++)
        watch_list_free(vec_wl->watch_lists + i);
    satoko_free(vec_wl->watch_lists);
    satoko_free(vec_wl);
}

static inline void vec_wl_clean(vec_wl_t *vec_wl)
{
    unsigned i;
    for (i = 0; i < vec_wl->size; i++) {
        vec_wl->watch_lists[i].size = 0;
        vec_wl->watch_lists[i].n_bin = 0;
    }
    vec_wl->size = 0;
}

static inline void vec_wl_push(vec_wl_t *vec_wl)
{
    if (vec_wl->size == vec_wl->cap) {
        unsigned new_size =
            (vec_wl->cap < 4) ? vec_wl->cap * 2 : (vec_wl->cap / 2) * 3;

        vec_wl->watch_lists = satoko_realloc(
            struct watch_list, vec_wl->watch_lists, new_size);
        memset(vec_wl->watch_lists + vec_wl->cap, 0,
               sizeof(struct watch_list) * (new_size - vec_wl->cap));
        if (vec_wl->watch_lists == NULL) {
            printf("failed to realloc memory from %.1f mb to %.1f "
                   "mb.\n",
                   1.0 * vec_wl->cap / (1 << 20),
                   1.0 * new_size / (1 << 20));
            fflush(stdout);
        }
        vec_wl->cap = new_size;
    }
    vec_wl->size++;
}

static inline struct watch_list *vec_wl_at(vec_wl_t *vec_wl, unsigned idx)
{
    assert(idx < vec_wl->cap);
    assert(idx < vec_wl->size);
    return vec_wl->watch_lists + idx;
}

ABC_NAMESPACE_HEADER_END
#endif /* satoko__watch_list_h */
