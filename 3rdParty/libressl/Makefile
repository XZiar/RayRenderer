#@XZBUILD
#@XZBUILD@description@A version of the TLS/crypto stack forked from OpenSSL
#@XZBUILD@version@2.9.0

BUILD_TYPE	:= static

include $(PROJPATH)/Makebase.mk

INCPATH		+= -I./include/ -I./ -I./include/compat/ -I./crypto/asn1/ -I./crypto/bn/ -I./crypto/dsa/ -I./crypto/evp/ -I./crypto/modes/ -I./crypto/
DEBUGLEVEL	= -g

CSRCS		+= $(wildcard ssl/*.c) $(wildcard tls/*.c)
CSRCS		+= $(filter-out crypto/armcap.c crypto/crypto_lock_win.c, $(wildcard crypto/*.c))
CSRCS		+= $(filter-out $(wildcard crypto/compat/*.c) $(wildcard crypto/*/*_win.c) crypto/poly1305/poly1305-donna.c crypto/chacha/chacha-merged.c, $(wildcard crypto/*/*.c))
CSRCS		+= $(patsubst %, crypto/compat/%.c, arc4random arc4random_uniform explicit_bzero freezero getentropy_linux recallocarray reallocarray strlcat strlcpy timingsafe_bcmp timingsafe_memcmp)

ifeq ($(PLATFORM), x64)
	ASMSRCS		+= $(wildcard crypto/*/*-elf-x86_64.S) crypto/cpuid-elf-x86_64.S
	DEL_ASMC	 = $(patsubst %, crypto/%.c, camellia/camellia camellia/cmll_cbc aes/aes_core des/ncbc_enc rc4/rc4_enc rc4/rc4_skey aes/aes_cbc whrlpool/wp_block)
	CSRCS		:= $(filter-out $(DEL_ASMC), $(CSRCS))
endif

ifneq ($(TARGET), Release)
	CXXOPT	+= -O1 
endif

CVERSION	 =
CXXFLAGS	:= $(filter-out -pedantic, $(CXXFLAGS))
CXXFLAGS	+= -Wno-pointer-sign -DAES_ASM -DBSAES_ASM -DGHASH_ASM -DHAVE_ASPRINTF -DHAVE_CLOCK_GETTIME -DHAVE_ERR_H -DHAVE_GETAUXVAL -DHAVE_GETPAGESIZE -DHAVE_MEMMEM -DHAVE_STRCASECMP -DHAVE_STRNDUP -DHAVE_STRNLEN -DHAVE_STRSEP -DHAVE_TIMEGM 
CXXFLAGS	+= -DLIBRESSL_INTERNAL -DMD5_ASM -DOPENSSL_BN_ASM_GF2m -DOPENSSL_BN_ASM_MONT -DOPENSSL_BN_ASM_MONT5 -DOPENSSL_CPUID_OBJ -DOPENSSL_IA32_SSE2 -DOPENSSL_NO_HW_PADLOCK -DRSA_ASM -DSHA1_ASM -DSHA256_ASM -DSHA512_ASM -DSIZEOF_TIME_T=8 -DVPAES_ASM -DWHIRLPOOL_ASM -D_BSD_SOURCE -D_DEFAULT_SOURCE -D_GNU_SOURCE -D_POSIX_SOURCE -D__BEGIN_HIDDEN_DECLS="" -D__END_HIDDEN_DECLS=""

NAME	= libressl

include $(PROJPATH)/MakeTargets.mk
