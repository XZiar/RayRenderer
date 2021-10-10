#include <stdlib.h>
#if defined(__GLIBC__) && __GLIBC__ >= 2
// https://man7.org/linux/man-pages/man3/getenv.3.html
// secure_getenv() first appeared in glibc 2.17.
# if __GLIBC_MINOR__ >= 17
#   define HAVE_SECURE_GETENV
# else
#   define HAVE___SECURE_GETENV
# endif
#endif
