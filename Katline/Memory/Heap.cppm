module;

#include <stdint.h>

export module Katline:Heap;

import CommonLib;
import :Sync;

export {
	namespace Katline {

	namespace Memory {

	void mrvn_memory_init(void *mem, usize size);
	bool mrvn_memory_add(void *mem, usize size);
	void *mrvn_malloc(usize size);
	void mrvn_free(void *mem);

	}

	}
}

namespace Katline::Memory {

static Sync::SpinLock g_heap_lock;

typedef struct DList DList;
struct DList {
	DList *next;
	DList *prev;
};

static inline auto dlist_init(DList *dlist) -> void
{
	dlist->next = dlist;
	dlist->prev = dlist;
}

static inline auto dlist_insert_after(DList *d1, DList *d2) -> void
{
	DList *n1 = d1->next;
	DList *e2 = d2->prev;

	d1->next = d2;
	d2->prev = d1;
	e2->next = n1;
	n1->prev = e2;
}

static inline auto dlist_insert_before(DList *d1, DList *d2) -> void
{
	DList *e1 = d1->prev;
	DList *e2 = d2->prev;

	e1->next = d2;
	d2->prev = e1;
	e2->next = d1;
	d1->prev = e2;
}

static inline auto dlist_remove(DList *d) -> void
{
	d->prev->next = d->next;
	d->next->prev = d->prev;
	d->next = d;
	d->prev = d;
}

static inline auto dlist_push(DList **d1p, DList *d2) -> void
{
	if (*d1p != nullptr)
		dlist_insert_before(*d1p, d2);
	*d1p = d2;
}

static inline auto dlist_pop(DList **dp) -> DList *
{
	DList *d1 = *dp;
	DList *d2 = d1->next;
	dlist_remove(d1);
	if (d1 == d2)
		*dp = nullptr;
	else
		*dp = d2;
	return d1;
}

static inline auto dlist_remove_from(DList **d1p, DList *d2) -> void
{
	if (*d1p == d2)
		dlist_pop(d1p);
	else
		dlist_remove(d2);
}

#define CONTAINER(C, l, v) ((C *)(((char *)v) - (uptr) & (((C *)0)->l)))
#define OFFSETOF(TYPE, MEMBER) __builtin_offsetof(TYPE, MEMBER)
#define DLIST_INIT(v, l) dlist_init(&v->l)
#define DLIST_REMOVE_FROM(h, d, l) \
	{ \
		__typeof__(**h) **h_ = h, *d_ = d; \
		DList *head = &(*h_)->l; \
		dlist_remove_from(&head, &d_->l); \
		if (head == nullptr) { \
			*h_ = nullptr; \
		} else { \
			*h_ = CONTAINER(__typeof__(**h), l, head); \
		} \
	}
#define DLIST_PUSH(h, v, l) \
	{ \
		__typeof__(*v) **h_ = h, *v_ = v; \
		DList *head = &(*h_)->l; \
		if (*h_ == nullptr) \
			head = nullptr; \
		dlist_push(&head, &v_->l); \
		*h_ = CONTAINER(__typeof__(*v), l, head); \
	}
#define DLIST_POP(h, l) \
	({ \
		__typeof__(**h) **h_ = h; \
		DList *head = &(*h_)->l; \
		DList *res = dlist_pop(&head); \
		if (head == nullptr) { \
			*h_ = nullptr; \
		} else { \
			*h_ = CONTAINER(__typeof__(**h), l, head); \
		} \
		CONTAINER(__typeof__(**h), l, res); \
	})
#define DLIST_ITERATOR_BEGIN(h, l, it) \
	{ \
		__typeof__(*h) *h_ = h; \
		DList *last_##it = h_->l.prev, *iter_##it = &h_->l, *next_##it; \
		do { \
			if (iter_##it == last_##it) { \
				next_##it = nullptr; \
			} else { \
				next_##it = iter_##it->next; \
			} \
			__typeof__(*h) *it = CONTAINER(__typeof__(*h), l, iter_##it);
#define DLIST_ITERATOR_END(it) \
	} \
	while ((iter_##it = next_##it)) \
		; \
	}
#define DLIST_ITERATOR_REMOVE_FROM(h, it, l) DLIST_REMOVE_FROM(h, iter_##it, l)

typedef struct Chunk Chunk;
struct Chunk {
	DList all;
	int used;
	union {
		char data[0];
		DList free;
	};
};

enum {
	NUM_SIZES = 32,
	ALIGN = 4,
	MIN_SIZE = sizeof(DList),
	HEADER_SIZE = OFFSETOF(Chunk, data),
};

Chunk *free_chunk[NUM_SIZES] = { nullptr };
usize mem_free = 0;
usize mem_used = 0;
usize mem_meta = 0;
Chunk *first = nullptr;
Chunk *last = nullptr;

static auto memory_chunk_init(Chunk *chunk) -> void;
static auto memory_chunk_size(Chunk const *chunk) -> usize;
static auto memory_chunk_slot(usize size) -> int;

static auto memory_chunk_bucket(usize size) -> int
{
	int n = memory_chunk_slot(size);
	if (n < 0)
		return 0;
	if (n >= NUM_SIZES)
		return NUM_SIZES - 1;
	return n;
}

static auto memory_add_region(void *mem, usize size) -> bool
{
	uptr const align_mask = static_cast<uptr>(ALIGN - 1);
	char *mem_start = (char *)(((uptr)mem + align_mask) & ~align_mask);
	char *mem_end
	    = (char *)(((unsigned long)mem + size) & ((unsigned long)~(ALIGN - 1)));

	if (mem_end <= mem_start)
		return false;
	if (static_cast<usize>(mem_end - mem_start) < (sizeof(Chunk) * 3))
		return false;

	Chunk *region_first = (Chunk *)mem_start;
	Chunk *second = region_first + 1;
	Chunk *region_last = ((Chunk *)mem_end) - 1;

	if (second >= region_last)
		return false;

	memory_chunk_init(region_first);
	memory_chunk_init(second);
	memory_chunk_init(region_last);
	dlist_insert_after(&region_first->all, &second->all);
	dlist_insert_after(&second->all, &region_last->all);

	region_first->used = 1;
	region_last->used = 1;

	usize len = memory_chunk_size(second);
	int n = memory_chunk_bucket(len);
	DLIST_PUSH(&free_chunk[n], second, free);
	mem_free += len - HEADER_SIZE;
	mem_meta += sizeof(Chunk) * 2 + HEADER_SIZE;

	if (first == nullptr)
		first = region_first;
	last = region_last;
	return true;
}

static auto memory_chunk_init(Chunk *chunk) -> void
{
	DLIST_INIT(chunk, all);
	chunk->used = 0;
	DLIST_INIT(chunk, free);
}

static auto memory_chunk_size(Chunk const *chunk) -> usize
{
	char *end = (char *)(chunk->all.next);
	char *start = (char *)(&chunk->all);
	return static_cast<usize>((end - start) - HEADER_SIZE);
}

static auto memory_chunk_slot(usize size) -> int
{
	int n = -1;
	while (size > 0) {
		++n;
		size /= 2;
	}
	return n;
}

auto mrvn_memory_init(void *mem, usize size) -> void
{
	Sync::ScopedIrqSpinLock guard { g_heap_lock };

	for (int i = 0; i < NUM_SIZES; i++)
		free_chunk[i] = nullptr;

	mem_free = 0;
	mem_used = 0;
	mem_meta = 0;
	first = nullptr;
	last = nullptr;

	(void)memory_add_region(mem, size);
}

auto mrvn_memory_add(void *mem, usize size) -> bool
{
	Sync::ScopedIrqSpinLock guard { g_heap_lock };
	return memory_add_region(mem, size);
}

auto mrvn_malloc(usize size) -> void *
{
	Sync::ScopedIrqSpinLock guard { g_heap_lock };

	size = (size + ALIGN - 1) & (unsigned long)(~(ALIGN - 1));
	if (size < MIN_SIZE)
		size = MIN_SIZE;

	int n = memory_chunk_slot(size - 1) + 1;
	if (n >= NUM_SIZES)
		return nullptr;

	while (!free_chunk[n]) {
		++n;
		if (n >= NUM_SIZES)
			return nullptr;
	}

	Chunk *chunk = DLIST_POP(&free_chunk[n], free);
	usize size2 = memory_chunk_size(chunk);
	usize len = 0;

	if (size + sizeof(Chunk) <= size2) {
		Chunk *chunk2 = (Chunk *)(((char *)chunk) + HEADER_SIZE + size);
		memory_chunk_init(chunk2);
		dlist_insert_after(&chunk->all, &chunk2->all);
		len = memory_chunk_size(chunk2);
		int n = memory_chunk_bucket(len);
		DLIST_PUSH(&free_chunk[n], chunk2, free);
		mem_meta += HEADER_SIZE;
		mem_free += len - HEADER_SIZE;
	}

	chunk->used = 1;
	mem_free -= size2;
	mem_used += size2 - len - HEADER_SIZE;
	return chunk->data;
}

static auto remove_free(Chunk *chunk) -> void
{
	usize len = memory_chunk_size(chunk);
	int n = memory_chunk_bucket(len);
	DLIST_REMOVE_FROM(&free_chunk[n], chunk, free);
	mem_free -= len - HEADER_SIZE;
}

static auto push_free(Chunk *chunk) -> void
{
	usize len = memory_chunk_size(chunk);
	int n = memory_chunk_bucket(len);
	DLIST_PUSH(&free_chunk[n], chunk, free);
	mem_free += len - HEADER_SIZE;
}

auto mrvn_free(void *mem) -> void
{
	Sync::ScopedIrqSpinLock guard { g_heap_lock };

	Chunk *chunk = (Chunk *)((char *)mem - HEADER_SIZE);
	Chunk *next = CONTAINER(Chunk, all, chunk->all.next);
	Chunk *prev = CONTAINER(Chunk, all, chunk->all.prev);

	mem_used -= memory_chunk_size(chunk);

	if (next->used == 0) {
		remove_free(next);
		dlist_remove(&next->all);
		mem_meta -= HEADER_SIZE;
		mem_free += HEADER_SIZE;
	}
	if (prev->used == 0) {
		remove_free(prev);
		dlist_remove(&chunk->all);
		push_free(prev);
		mem_meta -= HEADER_SIZE;
		mem_free += HEADER_SIZE;
	} else {
		chunk->used = 0;
		DLIST_INIT(chunk, free);
		push_free(chunk);
	}
}

}
