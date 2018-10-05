#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef CUDAMIPMAP_EXPORT
#   define CUDAMIPMAPAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define CUDAMIPMAPAPI _declspec(dllimport)
# endif
#else
# ifdef CUDAMIPMAP_EXPORT
#   define CUDAMIPMAPAPI __attribute__((visibility("default")))
#   define COMMON_EXPORT
# else
#   define CUDAMIPMAPAPI
# endif
#endif

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif
void CUDAMIPMAPAPI DoMipmap(const void* src, void* dst, const uint32_t width, const uint32_t height);
#ifdef __cplusplus
}
#endif