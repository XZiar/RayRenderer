BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= OpenGLUtil

LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger -lImageUtil -lGL
DEPLIBS		+= -lglew -lfmt -lcpuid
CPPFLAGS	+= -DOGLU_EXPORT

include $(PROJPATH)/MakeTargets.mk
