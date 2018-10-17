#@XZBUILD
#@XZBUILD@description@A free C++ Class Library of Cryptographic Schemes
#@XZBUILD@version@7.0.0

BUILD_TYPE	:= static

include $(PROJPATH)/Makebase.mk

EXCEPT_CPP	 = eccrypto.cpp eprecomp.cpp
EXCEPT_NASM  = $(wildcard *.asm)

ifneq ($(TARGET), Release)
	CXXOPT	+= -O1 
endif

ifneq ($(XZMK_HAS_SHA),1)
	CXXFLAGS	+= -DCRYPTOPP_DISABLE_SHA
endif

NAME	= cryptopp

include $(PROJPATH)/MakeTargets.mk
