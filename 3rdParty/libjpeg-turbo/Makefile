COMPILER	?= g++
CCOMPILER	?= gcc
SCOMPILER	?= ar
PLATFORM	?= x64
ifeq ($(PLATFORM), x64)
	OBJPREFEX = x64/
else
	OBJPREFEX = 
endif
TARGET		?= Debug
PROJPATH	?= ./
OBJPATH 	:= ./$(OBJPREFEX)$(TARGET)/
APPPATH		:= $(PROJPATH)$(OBJPREFEX)$(TARGET)/
INCPATH		:= -I$(PROJPATH) -I$(PROJPATH)3rdParty
LDPATH		:= -L$(APPPATH)
CPPFLAGS	:= -g3 -Wall -pedantic -std=c++17 -pthread
CFLAGS		:= -g3 -Wall -pedantic -std=c11 -pthread
ASMFLAGS	:= 
LIBRARYS	:= 
DEPLIBS		:= 

ifeq ($(TARGET), Release)
	CPPFLAGS	+= -DNDEBUG -O2 -flto
	CFLAGS		+= -DNDEBUG -O2 -flto
else
endif

ifneq ($(BOOST_PATH), )
	INCPATH += -I"$(BOOST_PATH)"
endif


ALLASMS		= $(wildcard simd/*.asm)
AMD64ASMS	= $(wildcard simd/*-sse2-64.asm)
EXCEPT	= jccolext.c jcsample.c jdcol565.c jdcolext.c jdmrg565.c jdmrgext.c jstdhuff.c
SRCS	= $(filter-out $(EXCEPT), $(wildcard *.c))
ifeq ($(PLATFORM), x64)
	ASMS	=  $(filter-out simd/jccolext-sse2-64.asm simd/jcgryext-sse2-64.asm simd/jdcolext-sse2-64.asm simd/jdmrgext-sse2-64.asm, $(AMD64ASMS))
	SRCS	+= simd/jsimd_x86_64.c
	ASMFLAGS	+= -f elf64 -D__x86_64__
else
	ASMS	=  $(filter-out simd/jccolext-mmx.asm simd/jcgryext-mmx.asm simd/jdcolext-mmx.asm simd/jdmrgext-mmx.asm simd/jccolext-sse2.asm simd/jcgryext-sse2.asm simd/jdcolext-sse2.asm simd/jdmrgext-sse2.asm $(AMD64ASMS), $(ALLASMS))
	SRCS	+= simd/jsimd_i386.c
	ASMFLAGS	+= -f elf
endif
OBJS 	= $(patsubst %, $(OBJPATH)%.o, $(SRCS))
ASMOBJS	= $(patsubst %, $(OBJPATH)%.o, $(ASMS))
DEPS 	= $(patsubst %.o, %.d, $(OBJS))
NAME	= jpeg-turbo
APPS	= $(APPPATH)lib$(NAME).a

CFLAGS	+= -D_LARGEFILE64_SOURCE=1 -DNO_UNDERLINE -DASMV -DWITH_SIMD -DSIZEOF_SIZE_T
INCPATH += -I./


$(NAME): $(OBJS) $(ASMOBJS) mkdir
	$(info building ${NAME} [${TARGET} version on ${PLATFORM}] to ${APPPATH})
	$(SCOMPILER) rcs $(APPS) $(OBJS) $(ASMOBJS)

$(OBJPATH)%.o: % mkdir
	$(CCOMPILER) $(INCPATH) $(CFLAGS) -MMD -MP -fPIC -c $< -o $@

$(OBJPATH)%.asm.o: %.asm mkdir
	nasm -g $(ASMFLAGS) -i'simd/' $< -o $@

.PHONY: clean mkdir

mkdir: 
	mkdir -p $(OBJPATH)simd

clean:
	$(RM) $(OBJS) $(DEPS) $(APPS)

-include $(DEPS)
