CPPCOMPILER		?= g++
CCOMPILER		?= gcc
ASMCOMPILER		?= g++
NASMCOMPILER	?= nasm
ISPCCOMPILER	?= ispc
STATICLINKER	?= ar
DYNAMICLINKER	?= g++
APPLINKER		?= g++

#ANSI Colors
CLR_BLACK	:= \033[90m
CLR_RED		:= \033[91m
CLR_GREEN	:= \033[92m
CLR_YELLOW	:= \033[93m
CLR_BLUE	:= \033[94m
CLR_MAGENTA	:= \033[95m
CLR_CYAN	:= \033[96m
CLR_WHITE	:= \033[97m
CLR_CLEAR	:= \033[39m
define CLR_TEXT
$(1)$(2)$(CLR_CLEAR)
endef

#X86 SIMD detect
XZMK_HAS_SSE 		:= $(shell echo | $(CPPCOMPILER) -march=native -dM -E - 2>/dev/null | grep -i -c __SSE__)
XZMK_HAS_SSE2 		:= $(shell echo | $(CPPCOMPILER) -march=native -dM -E - 2>/dev/null | grep -i -c __SSE2__)
XZMK_HAS_SSE3 		:= $(shell echo | $(CPPCOMPILER) -march=native -dM -E - 2>/dev/null | grep -i -c __SSE3__)
XZMK_HAS_SSSE3 		:= $(shell echo | $(CPPCOMPILER) -march=native -dM -E - 2>/dev/null | grep -i -c __SSSE3__)
XZMK_HAS_SSE4_1		:= $(shell echo | $(CPPCOMPILER) -march=native -dM -E - 2>/dev/null | grep -i -c __SSE4_1__)
XZMK_HAS_SSE4_2 	:= $(shell echo | $(CPPCOMPILER) -march=native -dM -E - 2>/dev/null | grep -i -c __SSE4_2__)
XZMK_HAS_AVX 		:= $(shell echo | $(CPPCOMPILER) -march=native -dM -E - 2>/dev/null | grep -i -c __AVX__)
XZMK_HAS_FMA 		:= $(shell echo | $(CPPCOMPILER) -march=native -dM -E - 2>/dev/null | grep -i -c __FMA__)
XZMK_HAS_AVX2 		:= $(shell echo | $(CPPCOMPILER) -march=native -dM -E - 2>/dev/null | grep -i -c __AVX2__)
XZMK_HAS_AES 		:= $(shell echo | $(CPPCOMPILER) -march=native -dM -E - 2>/dev/null | grep -i -c __AES__)
XZMK_HAS_SHA 		:= $(shell echo | $(CPPCOMPILER) -march=native -dM -E - 2>/dev/null | grep -i -c __SHA__)
XZMK_HAS_PCLMUL		:= $(shell echo | $(CPPCOMPILER) -march=native -dM -E - 2>/dev/null | grep -i -c __PCLMUL__)
XZMK_CLANG			:= $(shell echo | $(CPPCOMPILER) -dM -E - 2>/dev/null | grep -i -c __clang__)

TARGET		?= Debug
PLATFORM	?= x64
PROJPATH	?= ./
OBJPATH 	 = ./$(OBJPREFEX)$(TARGET)/
APPPATH		 = $(PROJPATH)$(OBJPREFEX)$(TARGET)/
INCPATH		 = -I"$(PROJPATH)" -I"$(PROJPATH)3rdParty"
LDPATH		 = -L"$(APPPATH)" -L.
CXXFLAGS	:= -g3 -Wall -pedantic -pthread -Wno-unknown-pragmas -Wno-ignored-attributes
CXXOPT		:=
CVERSION	:= -std=c11
CPPFLAGS	 = $(CXXFLAGS) -std=c++17
CPPPCH		:= 
CFLAGS		 = $(CXXFLAGS) $(CVERSION)
NASMFLAGS	:= -g 
LIBRARYS	:= 
DEPLIBS		:= 

ifeq ($(XZMK_CLANG), 1)
CXXFLAGS	+= -Wno-newline-eof
endif

ifneq ($(TARGET), Debug)
ifneq ($(TARGET), Release)
@echo "$(CLR_RED)Unknown Target [$(TARGET)], Should be [Debug|Release]$(CLR_CLEAR)"
$(error Unknown Target [$(TARGET)], Should be [Debug|Release])
endif
endif

ifneq ($(PLATFORM), x86)
ifneq ($(PLATFORM), x64)
@echo "$(CLR_RED)Unknown Platform [$(PLATFORM)], Should be [x86|x64]$(CLR_CLEAR)"
$(error Unknown Platform [$(PLATFORM)], Should be [x86|x64])
endif
endif

ifeq ($(PLATFORM), x64)
	OBJPREFEX	 = x64/
	CXXFLAGS	+= -march=native -m64
	NASMFLAGS	+= -f elf64 -D__x86_64__ -DELF
else
	OBJPREFEX	 = 
	CXXFLAGS	+= -march=native -m32
	NASMFLAGS	+= -f elf32 -DELF
endif

ifeq ($(TARGET), Release)
	CXXFLAGS	+= -DNDEBUG -flto
ifeq ($(XZMK_CLANG), 1)
	CXXFLAGS	+= -fuse-ld=gold
endif
ifeq ($(CXXOPT),)
	CXXFLAGS	+= -O2
endif
else
ifeq ($(CXXOPT),)
	CXXFLAGS	+= -O0
endif
endif

ifneq ($(CXXOPT),)
	CXXFLAGS	+= $(CXXOPT)
endif

ifneq ($(BOOST_PATH), )
	INCPATH += -I"$(BOOST_PATH)"
endif

EXCEPT_C	:= 
EXCEPT_CPP	:= 
EXCEPT_RC	:=
EXCEPT_ASM	:= 
EXCEPT_NASM	:= 
EXCEPT_ISPC	:=
PCH_HEADER	:=
ISPC_TARGETS	:= 

CSRCS		 = $(filter-out $(EXCEPT_C), $(wildcard *.c))
CPPSRCS		 = $(filter-out $(EXCEPT_CPP), $(wildcard *.cpp) $(wildcard *.cc) $(wildcard *.cxx))
RCSRCS		 = $(filter-out $(EXCEPT_RC), $(wildcard *.rc))
ASMSRCS		 = $(filter-out $(EXCEPT_ASM), $(wildcard *.S))
NASMSRCS	 = $(filter-out $(EXCEPT_NASM), $(wildcard *.asm))
ISPCSRCS	 = $(filter-out $(EXCEPT_ISPC), $(wildcard *.ispc))
ISPCFSRCS	 = $(foreach tar,$(ISPC_TARGETS),$(patsubst %.ispc, %_$(tar).ispc, $(ISPCSRCS)))
CXXOBJS		 = $(patsubst %, $(OBJPATH)%.o, $(CSRCS) $(CPPSRCS) $(RCSRCS))
OTHEROBJS	 = $(patsubst %, $(OBJPATH)%.o, $(ASMSRCS) $(NASMSRCS) $(ISPCSRCS))
DIRS		 = $(dir $(CXXOBJS) $(OTHEROBJS))
PCH_PCH		 = $(patsubst %, $(OBJPATH)%.gch, $(PCH_HEADER))
DEPS 		 = $(patsubst %.o, %.d, $(CXXOBJS)) $(patsubst %.gch, %.d, $(PCH_PCH))
NAME		?= 



ifeq ($(BUILD_TYPE), static)
APPS		 = $(APPPATH)lib$(NAME).a
else
ifeq ($(BUILD_TYPE), dynamic)
APPS		 = $(APPPATH)lib$(NAME).so
else
APPS		 = $(APPPATH)$(NAME)
endif
endif

all: postbuild
