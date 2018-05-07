COMPILER	?= g++
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
LIBRARYS	:= 
DEPLIBS		:= 

ifeq ($(TARGET), Release)
	CPPFLAGS	+= -DNDEBUG -O2 -flto
else
endif

ifneq ($(BOOST_PATH), )
	INCPATH += -I"$(BOOST_PATH)"
endif


ifeq ($(PLATFORM), x64)
	ASMS	= $(wildcard asm/*x86_64_*_elf_gas.S)
else
	ASMS	= $(wildcard asm/*i386_*_elf_gas.S)
endif
SRCS	= posix/stack_traits.cpp
OBJS 	= $(patsubst %, $(OBJPATH)%.o, $(SRCS) $(ASMS))
DEPS 	= $(patsubst %.o, %.d, $(OBJS))
NAME	= boost.context
APPS	= $(APPPATH)lib$(NAME).a

$(NAME): $(OBJS) mkdir
	$(info building ${NAME} [${TARGET} version on ${PLATFORM}] to ${APPPATH})
	$(SCOMPILER) rcs $(APPPATH)lib$@.a $(OBJS)

$(OBJPATH)%.o: % mkdir
	$(COMPILER) $(INCPATH) $(CPPFLAGS) -MMD -MP -fPIC -c $< -o $@

.PHONY: clean mkdir

mkdir: 
	mkdir -p $(OBJPATH)posix
	mkdir -p $(OBJPATH)asm

clean:
	$(RM) $(OBJS) $(DEPS) $(APPS)

-include $(DEPS)
