#@XZBUILD
#@XZBUILD@description@A version of the TLS/crypto stack forked from OpenSSL
#@XZBUILD@version@2.9.0

BUILD_TYPE	:= static

include $(PROJPATH)/Makebase.mk

INCPATH		+= -I./include/ -I./ -I./include/compat/ -I./crypto/asn1/ -I./crypto/bn/ -I./crypto/dsa/ -I./crypto/evp/ -I./crypto/modes/ -I./crypto/

CSRCS		+= $(wildcard ssl/*.c) $(wildcard tls/*.c)
CSRCS		+= $(filter-out crypto/armcap.c crypto/crypto_lock_win.c, $(wildcard crypto/*.c))
DEL_NETROPY	 = $(patsubst %, crypto/compat/getentropy_%.c, aix freebsd hpux netbsd osx solaris win) 
DEL_WINSRYPTO = crypto/compat/posix_win.c crypto/compat/explicit_bzero_win.c crypto/ui/ui_openssl_win.c crypto/bio/b_win.c
CSRCS		+= $(filter-out $(DEL_NETROPY) $(DEL_WINSRYPTO) crypto/poly1305/poly1305-donna.c crypto/chacha/chacha-merged.c, $(wildcard crypto/*/*.c))

ASMSRCS		+= $(wildcard crypto/*/*-elf-x86_64.S) crypto/cpuid-elf-x86_64.S

CVERSION	 =
CXXFLAGS	:= $(filter-out -pedantic, $(CXXFLAGS))
CXXFLAGS	+= -Wno-pointer-sign -DAES_ASM -DBSAES_ASM -DGHASH_ASM -DHAVE_ASPRINTF -DHAVE_CLOCK_GETTIME -DHAVE_ERR_H -DHAVE_EXPLICIT_BZERO -DHAVE_GETAUXVAL -DHAVE_GETENTROPY -DHAVE_GETPAGESIZE -DHAVE_MEMMEM -DHAVE_REALLOCARRAY -DHAVE_STRCASECMP -DHAVE_STRNDUP -DHAVE_STRNLEN -DHAVE_STRSEP -DHAVE_TIMEGM 
CXXFLAGS	+= -DLIBRESSL_INTERNAL -DMD5_ASM -DOPENSSL_BN_ASM_GF2m -DOPENSSL_BN_ASM_MONT -DOPENSSL_BN_ASM_MONT5 -DOPENSSL_CPUID_OBJ -DOPENSSL_IA32_SSE2 -DOPENSSL_NO_HW_PADLOCK -DRSA_ASM -DSHA1_ASM -DSHA256_ASM -DSHA512_ASM -DSIZEOF_TIME_T=8 -DVPAES_ASM -DWHIRLPOOL_ASM -D_BSD_SOURCE -D_DEFAULT_SOURCE -D_GNU_SOURCE -D_POSIX_SOURCE -D__BEGIN_HIDDEN_DECLS="" -D__END_HIDDEN_DECLS=""

NAME	= libressl

include $(PROJPATH)/MakeTargets.mk
