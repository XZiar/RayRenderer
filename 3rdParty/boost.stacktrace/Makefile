BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= boost.stacktrace

CXXFLAGS    += -DSTKTRACE_EXPORT
LIBRARYS	+= -lpthread -ldl -lbacktrace

ifeq ($(XZMK_CLANG), 1)
CXXFLAGS	+= -DBOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE=</usr/lib/gcc/x86_64-linux-gnu/8/include/backtrace.h>
endif

include $(PROJPATH)/MakeTargets.mk
