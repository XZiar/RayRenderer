BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= boost.stacktrace

CXXFLAGS    += -DSTKTRACE_EXPORT
LIBRARYS	+= -lpthread -ldl

include $(PROJPATH)/MakeTargets.mk
