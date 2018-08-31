BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= FreeGLUTView

LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger -lAsyncExecutor -lGL -lX11 -lXxf86vm -lXi
DEPLIBS		+= -lfreeglut -lfmt
CPPFLAGS	+= -DGLUTVIEW_EXPORT

include $(PROJPATH)/MakeTargets.mk
