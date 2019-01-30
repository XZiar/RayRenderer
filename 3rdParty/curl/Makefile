#@XZBUILD
#@XZBUILD@description@A library for transferring data with URL syntax
#@XZBUILD@version@7.63.0
#@XZBUILD@dependency@zlib-ng, libressl

BUILD_TYPE	:= static

include $(PROJPATH)/Makebase.mk

INCPATH		+= -I./include/ -I./ -I./lib/ -I"../zlib-ng" -I"../libressl/include"

CSRCS		+= $(wildcard lib/*.c) $(wildcard lib/*/*.c)
CSRCS		+= $(filter-out src/tool_main.c, $(wildcard src/*.c))

CVERSION	 =
CXXFLAGS	+= -Wpointer-arith -Wwrite-strings -Wunused -Wshadow -Winline -Wnested-externs -Wmissing-declarations -Wmissing-prototypes -Wno-long-long -Wfloat-equal -Wno-multichar -Wsign-compare -Wundef -Wno-format-nonliteral -Wendif-labels -Wstrict-prototypes -Wdeclaration-after-statement -Wstrict-aliasing=3 -Wcast-align -Wtype-limits -Wold-style-declaration -Wmissing-parameter-type -Wempty-body -Wclobbered -Wignored-qualifiers -Wconversion -Wno-sign-conversion -Wvla -Wdouble-promotion -Wno-system-headers
CXXFLAGS	+= -DHAVE_CONFIG_H -DCURL_STATICLIB -DBUILDING_LIBCURL -DCURL_HIDDEN_SYMBOLS


ifeq ($(TARGET), Debug)
	CXXFLAGS	+= -DDEBUGBUILD
endif

NAME	= curl

include $(PROJPATH)/MakeTargets.mk
