BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= miniLogger

LIBRARYS	+= -lpthread -lstdc++fs
DEPLIBS		+= -lfmt
CPPFLAGS	+= -DMINILOG_EXPORT

include $(PROJPATH)/MakeTargets.mk
