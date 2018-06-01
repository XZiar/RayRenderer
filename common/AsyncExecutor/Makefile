BUILD_TYPE	:= dynamic

include $(PROJPATH)Makebase.mk

NAME	= AsyncExecutor

LIBRARYS	+= -lpthread -lminiLogger
DEPLIBS		+= -lboost.context
CPPFLAGS	+= -DASYEXE_EXPORT

include $(PROJPATH)MakeTargets.mk
