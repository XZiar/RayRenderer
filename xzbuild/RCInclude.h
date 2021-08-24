#include <cstdint>
namespace common
{
    namespace detail
    {
        uint32_t RegistResource(const int32_t id, const char* ptrBegin, const char* ptrEnd);
    }
}

#if COMMON_COMPILER_MSVC
#   if defined(_M_ARM) || defined(_M_ARM64)
#       define COMMON_ARCH_ARM 1
#   elif defined(_M_IX86) || defined(_M_X64)
#       define COMMON_ARCH_X86 1
#   endif
#else
#   if defined(__arm__) || defined(__aarch64__)
#       define COMMON_ARCH_ARM 1
#   elif defined(__i386__) || defined(__amd64__) || defined(__x86_64__)
#       define COMMON_ARCH_X86 1
#   endif
#endif
#ifndef COMMON_ARCH_ARM
#   define COMMON_ARCH_ARM 0
#endif
#ifndef COMMON_ARCH_X86
#   define COMMON_ARCH_X86 0
#endif

#if UINTPTR_MAX == UINT64_MAX
#   define COMMON_OSBIT 64
#else
#   define COMMON_OSBIT 32
#endif

#if COMMON_ARCH_ARM
# define ALIGNMAX ".align 4\n"
# define ALIGNMIN ".align 0\n"
# if COMMON_OSBIT == 64
#   if defined(__APPLE__)
#     define REGMAIN(name) "adrp %0, " #name "_BEGIN@PAGE\n\tadrp %1, " #name "_END@PAGE\n\tadd %0, %0, " #name "_BEGIN@PAGEOFF\n\tadd %1, %1, " #name "_END@PAGEOFF" 
#   else
#     define REGMAIN(name) "adrp %0, " #name "_BEGIN\n\tadrp %1, " #name "_END\n\tadd %0, %0, :lo12:" #name "_BEGIN\n\tadd %1, %1, :lo12:" #name "_END" 
#   endif
# elif COMMON_OSBIT == 32
#   define REGMAIN(name) "ldr %0, =" #name "_BEGIN-1f-8; ldr %1, =" #name "_END-2f-8; 1:add %0, pc, %0; 2:add %1, pc, %1"
# endif
#else
# define ALIGNMAX ".balign 16\n"
# define ALIGNMIN ".balign 1\n"
# if COMMON_OSBIT == 64
#   define REGMAIN(name) "leaq " #name "_BEGIN(%%rip), %0\n\tleaq " #name "_END(%%rip), %1"
# elif COMMON_OSBIT == 32
#   define REGMAIN(name) "leal " #name "_BEGIN(%%eip), %0\n\tleal " #name "_END(%%eip), %1"
# endif
#endif
#if defined(__APPLE__)
#   define SYMBOLTYPE(name, sfx)
#   define INCHEADER __asm__(".const_data\n" 
#else
#   if COMMON_ARCH_ARM
#       define OBJTYPE "%object"
#   else
#       define OBJTYPE "@object"
#   endif
#   define SYMBOLTYPE(name, sfx) ".type " #name sfx ", " OBJTYPE "\n"
#   define INCHEADER __asm__(".section .rodata\n" 
#endif
#define INCFOOTER ".text\n");


#define INCDATA(name, file)     \
    SYMBOLTYPE(name, "_BEGIN")  \
    ALIGNMAX                    \
    #name "_BEGIN:\n"           \
    ".incbin " #file "\n"       \
    SYMBOLTYPE(name, "_END")    \
    ALIGNMIN                    \
    #name "_END:\n"             \
    ".byte 1\n"

#define REGDATA_(name) static uint32_t DUMMY_##name = []()          \
{                                                                   \
    const char *ptrBegin = nullptr, *ptrEnd = nullptr;              \
    __asm__(REGMAIN(name) : "=r"(ptrBegin), "=r"(ptrEnd));          \
    return common::detail::RegistResource(name, ptrBegin, ptrEnd);  \
}()
#define REGDATA(name)                                               \
{                                                                   \
    const char *ptrBegin = nullptr, *ptrEnd = nullptr;              \
    __asm__(REGMAIN(name) : "=r"(ptrBegin), "=r"(ptrEnd));          \
    common::detail::RegistResource(IDR_##name, ptrBegin, ptrEnd);   \
}
#define REGHEADER static uint32_t DUMMY_RC = []() {
#define REGFOOTER return 0; }();

