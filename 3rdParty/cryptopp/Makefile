#@XZBUILD
#@XZBUILD@description@A free C++ Class Library of Cryptographic Schemes
#@XZBUILD@version@8.0.0

BUILD_TYPE	:= static

include $(PROJPATH)/Makebase.mk
DEBUGLEVEL	= -g

EXCEPT_CPP	 = eccrypto.cpp eprecomp.cpp simple.cpp algebra.cpp polynomi.cpp
EXCEPT_NASM  = $(wildcard *.asm)

ifneq ($(TARGET), Release)
	CXXOPT	+= -O1 
endif

ifneq ($(XZMK_HAS_SHA),1)
	CXXFLAGS	+= -DCRYPTOPP_DISABLE_SHANI
endif
ifneq ($(XZMK_HAS_AVX2),1)
	CXXFLAGS	+= -DCRYPTOPP_DISABLE_AVX2
endif
ifneq ($(XZMK_HAS_AVX),1)
	CXXFLAGS	+= -DCRYPTOPP_DISABLE_AVX
endif

ifeq ($(XZMK_CLANG),1)
	CXXFLAGS	+= -DCRYPTOPP_DISABLE_MIXED_ASM=1
endif

NAME	= cryptopp

include $(PROJPATH)/MakeTargets.mk
