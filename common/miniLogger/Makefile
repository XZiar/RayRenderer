#@XZBUILD
#@XZBUILD@description@A mini logger library
#@XZBUILD@dependency@fmt

BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= miniLogger

LIBRARYS	+= -lpthread -lstdc++fs
DEPLIBS		+= -lfmt
CPPFLAGS	+= -DMINILOG_EXPORT

include $(PROJPATH)/MakeTargets.mk
