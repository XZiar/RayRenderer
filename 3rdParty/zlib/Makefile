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


ifeq ($(PLATFORM), x64)
	ASMS	= $(wildcard masmx64/*.S)
else
	ASMS	= $(wildcard masmx86/*.S)
endif
SRCS	= $(wildcard *.c)
OBJS 	= $(patsubst %, $(OBJPATH)%.o, $(SRCS) $(ASMS))
DEPS 	= $(patsubst %.o, %.d, $(OBJS))
NAME	= zlibstatic
APPS	= $(APPPATH)lib$(NAME).a

CFLAGS += -D_LARGEFILE64_SOURCE=1 -DNO_UNDERLINE -DASMV

$(NAME): $(OBJS) mkdir
	$(info building ${NAME} [${TARGET} version on ${PLATFORM}] to ${APPPATH})
	$(SCOMPILER) rcs $(APPS) $(OBJS)

$(OBJPATH)%.o: % mkdir
	$(CCOMPILER) $(INCPATH) $(CFLAGS) -MMD -MP -fPIC -c $< -o $@

.PHONY: clean mkdir

mkdir: 
	mkdir -p $(OBJPATH)masmx64
	mkdir -p $(OBJPATH)masmx86

clean:
	$(RM) $(OBJS) $(DEPS) $(APPS)

-include $(DEPS)
