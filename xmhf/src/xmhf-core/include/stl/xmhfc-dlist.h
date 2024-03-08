/*
 * The following license and copyright notice applies to all content in
 * this repository where some other license does not take precedence. In
 * particular, notices in sub-project directories and individual source
 * files take precedence over this file.
 *
 * See COPYING for more information.
 *
 * eXtensible, Modular Hypervisor Framework 64 (XMHF64)
 * Copyright (c) 2023 Eric Li
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the copyright holder nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

// copy from http://c.learncodethehardway.org/book/ex32.html

#ifndef XMHF_STL_DOUBLE_LLIST
#define XMHF_STL_DOUBLE_LLIST

#ifndef __ASSEMBLY__

struct XMHFListNode;

typedef enum {
    LIST_ELEM_INVALID = 0,
    LIST_ELEM_VAL = 1,
    LIST_ELEM_PTR = 2,
} LIST_ELEM_TYPE;

typedef struct XMHFListNode {
    struct XMHFListNode *next;
    struct XMHFListNode *prev;
    void *value;
    size_t mem_sz;
    LIST_ELEM_TYPE type;
} XMHFListNode;

typedef struct XMHFList {
    uint32_t	count;
	bool 		in_iteration;
    XMHFListNode*	first;
    XMHFListNode*	last;
} XMHFList;


extern XMHFList* xmhfstl_list_create(void);
extern void xmhfstl_list_destroy(XMHFList *list);

//! @brief Free value pointers of all nodes of a given list
extern void xmhfstl_list_clear(XMHFList *list);
extern void xmhfstl_list_clear_destroy(XMHFList *list);

#define XMHFList_count(l) ((l)->count)
#define XMHFList_first(l) ((l)->first != NULL ? (l)->first->value : NULL)
#define XMHFList_last(l) ((l)->last != NULL ? (l)->last->value : NULL)
#define xmhfstl_list_enqueue(l, v, sz, type) xmhfstl_list_push((l), (v), (sz), (type))
#define is_list_empty(l) (l->count == 0)

extern XMHFListNode* xmhfstl_list_push(XMHFList *list, void *value, size_t mem_sz, LIST_ELEM_TYPE type);
extern void* xmhfstl_list_pop(XMHFList *list);
extern void* xmhfstl_list_dequeue(XMHFList *list);

extern XMHFListNode* xmhfstl_list_last(XMHFList *list);
extern XMHFListNode* xmhfstl_list_first(XMHFList *list);

extern XMHFListNode* xmhfstl_list_insert_before(XMHFList* list, XMHFListNode *node, void *value, size_t mem_sz);
extern XMHFListNode* xmhfstl_list_insert_after(XMHFList* list, XMHFListNode *node, void *value, size_t mem_sz);

extern void* xmhfstl_list_remove(XMHFList *list, XMHFListNode *node);
extern void xmhfstl_list_debug(XMHFList *list);

// [TODO][Ticket 179] LIST_FOREACH needs to avoid list modification

#define XMHFLIST_FOREACH(L, S, M, V) XMHFListNode *_node = NULL;\
    XMHFListNode *V = NULL;\
    L->in_iteration = true; \
    for(V = _node = L->S; _node != NULL; V = _node = _node->M)

#define END_XMHFLIST_FOREACH(L) L->in_iteration = false;

#endif // __ASSEMBLY__
#endif // XMHF_STL_DOUBLE_LLIST
