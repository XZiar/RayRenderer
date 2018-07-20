CPPCOMPILER		?= g++
CCOMPILER		?= gcc
ASMCOMPILER		?= gcc
NASMCOMPILER	?= nasm
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

TARGET		?= Debug
PLATFORM	?= x64
PROJPATH	?= ./
OBJPATH 	 = ./$(OBJPREFEX)$(TARGET)/
APPPATH		 = $(PROJPATH)$(OBJPREFEX)$(TARGET)/
INCPATH		 = -I"$(PROJPATH)" -I"$(PROJPATH)3rdParty"
LDPATH		 = -L"$(APPPATH)"
SUBDIRS		:=
CXXFLAGS	:= -g3 -Wall -pedantic -pthread -Wno-unknown-pragmas
CPPFLAGS	 = $(CXXFLAGS) -std=c++17
CFLAGS		 = $(CXXFLAGS) -std=c11
NASMFLAGS	:= -g 
LIBRARYS	:= 
DEPLIBS		:= 

ifeq ($(PLATFORM), x64)
	OBJPREFEX	 = x64/
	CXXFLAGS	+= -mavx2 -m64
	NASMFLAGS	+= -f elf64 -D__x86_64__ -DELF
else
	OBJPREFEX	 = 
	CXXFLAGS	+= -msse2 -m32
	NASMFLAGS	+= -f elf32 -DELF
endif

ifeq ($(TARGET), Release)
	CXXFLAGS	+= -DNDEBUG -O2 -flto
else
endif

ifneq ($(BOOST_PATH), )
	INCPATH += -I"$(BOOST_PATH)"
endif

EXCEPT_C	:= 
EXCEPT_CPP	:= 
EXCEPT_ASM	:= 
EXCEPT_NASM	:= 

CSRCS		 = $(filter-out $(EXCEPT_C), $(wildcard *.c))
CPPSRCS		 = $(filter-out $(EXCEPT_CPP), $(wildcard *.cpp) $(wildcard *.cc))
ASMSRCS		 = $(filter-out $(EXCEPT_ASM), $(wildcard *.S))
NASMSRCS	 = $(filter-out $(EXCEPT_NASM), $(wildcard *.asm))
OBJS 		 = $(patsubst %, $(OBJPATH)%.o, $(CSRCS) $(CPPSRCS))
ASMOBJS		 = $(patsubst %, $(OBJPATH)%.o, $(ASMSRCS) $(NASMSRCS))
DEPS 		 = $(patsubst %.o, %.d, $(OBJS))
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

