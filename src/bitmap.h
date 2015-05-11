/* Licensed under LGPLv2+
   Originally from CCAN bitmap.h
*/

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <gc.h>


typedef uint64_t bitmap_word;

#define BITMAP_WORD_BITS        (sizeof(bitmap_word) * CHAR_BIT)
#define BITMAP_NWORDS(_n)       (((_n) + BITMAP_WORD_BITS - 1) / BITMAP_WORD_BITS)
#define BITMAP_WORD(_bm, _n)    ((_bm)[(_n) / BITMAP_WORD_BITS])
#define BITMAP_WORDBIT(_n)      (1UL << (BITMAP_WORD_BITS - ((_n) % BITMAP_WORD_BITS) - 1))
#define BITMAP_BIT_OFFSET(_n)   ((_n) % BITMAP_WORD_BITS)
#define BITMAP_BIT_MASK(_n)     (1UL << ((_n) % BITMAP_WORD_BITS))


static inline size_t bitmap_sizeof(unsigned long nbits) {
        return BITMAP_NWORDS(nbits) * sizeof(bitmap_word);
}

static inline bitmap_word *bitmap_alloc(unsigned long nbits) {
        return GC_MALLOC(bitmap_sizeof(nbits));
}

static inline void bitmap_zero(bitmap_word *bitmap, unsigned long nbits) {
        memset(bitmap, 0, bitmap_sizeof(nbits));
}

static inline bitmap_word *bitmap_alloc0(unsigned long nbits) {
        bitmap_word *bitmap;
        bitmap = bitmap_alloc(nbits);
        bitmap_zero(bitmap, nbits);
        return bitmap;
}

static inline void bitmap_set_bit(bitmap_word *bitmap, unsigned long n) {
        BITMAP_WORD(bitmap, n) |= BITMAP_BIT_MASK(n);
}

static inline bool bitmap_get_bit(bitmap_word *bitmap, unsigned long n) {
        return ((BITMAP_WORD(bitmap, n) & BITMAP_BIT_MASK(n))  > 0);
}

static inline void bitmap_clear_bit(bitmap_word *bitmap, unsigned long n) {
        BITMAP_WORD(bitmap, n) &= ~BITMAP_BIT_MASK(n);
}

static inline void bitmap_copy(bitmap_word *dst, const bitmap_word *src, unsigned long nbits) {
        memcpy(dst, src, bitmap_sizeof(nbits));
}
