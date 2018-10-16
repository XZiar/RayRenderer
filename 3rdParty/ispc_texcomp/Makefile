#@XZBUILD
#@XZBUILD@description@ISPC Texture Compressor

BUILD_TYPE	:= static

include $(PROJPATH)/Makebase.mk

ISPC_TARGETS	+= sse4 avx2
NAME	= ispc_texcomp

ispcbuild: $(patsubst %, $(OBJPATH)%.o, $(ISPCSRCS))

mainbuild: ispcbuild

include $(PROJPATH)/MakeTargets.mk

