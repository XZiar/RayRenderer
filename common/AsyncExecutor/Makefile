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


SRCS	= $(wildcard *.cpp)
OBJS 	= $(patsubst %, $(OBJPATH)%.o, $(SRCS))
DEPS 	= $(patsubst %.o, %.d, $(OBJS))
NAME	= AsyncExecutor
APPS	= $(APPPATH)lib$(NAME).so
LIBRARYS	+= -lminiLogger
DEPLIBS		+= -lboost.context
CPPFLAGS	+= -DASYEXE_EXPORT

$(NAME): $(OBJS)
	$(info building ${NAME} [${TARGET} version on ${PLATFORM}] to ${APPPATH})
	$(COMPILER) $(INCPATH) $(LDPATH) $(CPPFLAGS) $(LIBRARYS) -Wl,--whole-archive $(DEPLIBS) -Wl,--no-whole-archive -shared $^ -o $(APPPATH)lib$@.so

$(OBJPATH)%.o: %
	$(COMPILER) $(INCPATH) $(CPPFLAGS) -MMD -MP -fPIC -c $< -o $@

.PHONY: clean

clean:
	$(RM) $(OBJS) $(DEPS) $(APPS)

-include $(DEPS)
