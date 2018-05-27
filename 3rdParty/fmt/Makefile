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


SRCS	= $(wildcard *.cc) $(wildcard *.cpp)
OBJS 	= $(patsubst %, $(OBJPATH)%.o, $(SRCS))
DEPS 	= $(patsubst %.o, %.d, $(OBJS))
NAME	= fmt
APPS	= $(APPPATH)lib$(NAME).a

$(NAME): $(OBJS)
	$(info building ${NAME} [${TARGET} version on ${PLATFORM}] to ${APPPATH})
	$(SCOMPILER) rcs $(APPS) $^

$(OBJPATH)%.o: %
	$(COMPILER) $(INCPATH) $(CPPFLAGS) -MMD -MP -fPIC -c $< -o $@

.PHONY: clean

clean:
	$(RM) $(OBJS) $(DEPS) $(APPS)

-include $(DEPS)
