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

#define BITMAP_WORD_BITS        64
#define LOG_BITMAP_WORD_BITS    6
#define BITMAP_BIT_PLACE(_n)    ((_n) >> LOG_BITMAP_WORD_BITS)
#define BITMAP_BIT_OFFSET(_n)   ((_n) & (BITMAP_WORD_BITS - 1))
#define BITMAP_NWORDS(_n)       (((_n) + BITMAP_WORD_BITS - 1) >> LOG_BITMAP_WORD_BITS)
#define BITMAP_HEADWORDS(_n)    ((_n) / BITMAP_WORD_BITS)
#define BITMAP_TAILWORD(_bm,_n) ((_bm)[BITMAP_HEADWORDS(_n)])
#define BITMAP_HASTAIL(_n)      (((_n) % BITMAP_WORD_BITS) != 0)
#define BITMAP_TAILBITS(_n)     (~(-1UL >> ((_n) % BITMAP_WORD_BITS)))
#define BITMAP_TAIL(_bm,_n)     (BITMAP_TAILWORD(_bm, _n) & BITMAP_TAILBITS(_n))
#define BITMAP_WORD(_bm,_n)     ((_bm)[(_n) >> LOG_BITMAP_WORD_BITS])
#define BITMAP_BIT_MASK(_n)     (1UL << (BITMAP_BIT_OFFSET(_n)))

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

static inline bool bitmap_includes(bitmap_word *src1, const bitmap_word *src2, unsigned long nbits) {
        unsigned long i;
        for (i = 0; i < BITMAP_HEADWORDS(nbits); i++) {
                if (src1[i]  & ~src2[i])
                        return false;
        }

        if (BITMAP_HASTAIL(nbits) && (BITMAP_TAIL(src1, nbits) & ~BITMAP_TAIL(src2, nbits)))
                return false;
        return true;
}


/**
   \brief true if the first array includes the second
*/
static inline bool
bool_array_includes(bool* a1, bool* a2, uint32_t n) {
        for (uint32_t i = 0; i < n; i++)
                if (a2[i] && !a1[i])
                        return false;
        return true;
}
