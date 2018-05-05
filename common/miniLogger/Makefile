COMPILER	?= g++
TARGET		?= Debug
OBJPATH 	:= ./x64/$(TARGET)
INCPATH		:= -I../../ -I../../3rdParty
CPPFLAGS	:= -g3 -Wall -pedantic -std=c++17 -pthread
LIBRARYS	:= -lstdc++fs
ifeq ($(TARGET), Release)
	CPPFLAGS	+= -DNDEBUG -O2 -flto
else
endif

ifneq ($(BOOST_PATH), )
	INCPATH += -I"$(BOOST_PATH)"
endif


SRCS	= $(wildcard *.cpp)
OBJS 	= $(patsubst %.cpp, $(OBJPATH)/%.o, $(SRCS))
DEPS 	= $(patsubst %.cpp, $(OBJPATH)/%.d, $(SRCS))

miniLogger: $(OBJS)
	$(COMPILER) $(INCPATH) $(CPPFLAGS) $(LIBRARYS) -shared $^ -o $@.so

$(OBJPATH)/%.o: %.cpp
	$(COMPILER) $(INCPATH) $(CPPFLAGS) -MMD -MP -MF$(OBJPATH)/%.d -fPIC -c $< -o $@

.PHONY: clean

clean:
	$(RM) $(OBJS) $(DEPS) miniLogger.so

-include $(DEPS)
