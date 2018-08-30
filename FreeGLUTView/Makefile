BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= FreeGLUTView

LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger -lAsyncExecutor -lGL -lX11
DEPLIBS		+= -lfreeglut -lfmt
CPPFLAGS	+= -DGLUTVIEW_EXPORT

include $(PROJPATH)/MakeTargets.mk
