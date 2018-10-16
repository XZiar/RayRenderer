#@XZBUILD
#@XZBUILD@description@A C++ library for storing and printing backtraces

BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= boost.stacktrace

CXXFLAGS    += -DSTKTRACE_EXPORT
LIBRARYS	+= -lpthread -ldl -lbacktrace

include $(PROJPATH)/MakeTargets.mk
