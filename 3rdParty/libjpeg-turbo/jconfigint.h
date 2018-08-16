

/* libjpeg-turbo build number */
#define BUILD  "20180718"

/* Compiler's inline keyword */
#undef inline

/* How to obtain function inlining. */
#if defined(_MSC_VER)
#else
#endif

/* Define to the full name of this package. */
#define PACKAGE_NAME  "libjpeg-turbo"

/* Version number of package */
#define VERSION  "1.5.90"

#if defined(_MSC_VER)
#   define INLINE  __forceinline

/* Define if your compiler has __builtin_ctzl() and sizeof(unsigned long) == sizeof(size_t). */
/* #undef HAVE_BUILTIN_CTZL */

/* Define to 1 if you have the <intrin.h> header file. */
#define HAVE_INTRIN_H
#else
#   define INLINE  __inline__ __attribute__((always_inline))

#   define HAVE_BUILTIN_CTZL

/* Define to 1 if you have the <intrin.h> header file. */
/* #undef HAVE_INTRIN_H */
#endif

#include <stdint.h>
/* The size of `size_t', as computed by sizeof. */
#if SIZE_MAX == UINT64_MAX
#   define SIZEOF_SIZE_T  8
#else
#   define SIZEOF_SIZE_T  4
#endif

#if defined(_MSC_VER) && defined(HAVE_INTRIN_H)
#if (SIZEOF_SIZE_T == 8)
#define HAVE_BITSCANFORWARD64
#elif (SIZEOF_SIZE_T == 4)
#define HAVE_BITSCANFORWARD
#endif
#endif