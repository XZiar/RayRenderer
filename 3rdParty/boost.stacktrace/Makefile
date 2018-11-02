#@XZBUILD
#@XZBUILD@description@A C++ library for storing and printing backtraces
#@XZBUILD@dependency@StringCharset

BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= boost.stacktrace

CXXFLAGS    += -DSTKTRACE_EXPORT
LIBRARYS	+= -lpthread -ldl -lbacktrace -lStringCharset

include $(PROJPATH)/MakeTargets.mk
