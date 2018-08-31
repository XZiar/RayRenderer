BUILD_TYPE	:= static

include $(PROJPATH)/Makebase.mk

CSRCS	 	+= $(wildcard x11/*.c)
CXXFLAGS	+= -DHAVE_CONFIG_H -Dfreeglut_EXPORTS -DFREEGLUT_STATIC -Wno-unused-const-variable -Wno-unused-function
INCPATH		+= -I.
CVERSION	 =
NAME	= freeglut

include $(PROJPATH)/MakeTargets.mk
