BUILD_TYPE	:= static

include $(PROJPATH)/Makebase.mk

NAME	= uchardet
CPPSRCS		+= $(wildcard LangModels/*.cpp)
CXXFLAGS += -mfpmath=sse -DVERSION=\"0.0.6\"

include $(PROJPATH)/MakeTargets.mk
