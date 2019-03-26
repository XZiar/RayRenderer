CPPCOMPILER		?= g++
CCOMPILER		?= gcc
ASMCOMPILER		?= g++
NASMCOMPILER	?= nasm
ISPCCOMPILER	?= ispc
STATICLINKER	?= ar
DYNAMICLINKER	?= g++
APPLINKER		?= g++

#ANSI Colors
CLR_BLACK	:= \033[90m
CLR_RED		:= \033[91m
CLR_GREEN	:= \033[92m
CLR_YELLOW	:= \033[93m
CLR_BLUE	:= \033[94m
CLR_MAGENTA	:= \033[95m
CLR_CYAN	:= \033[96m
CLR_WHITE	:= \033[97m
CLR_CLEAR	:= \033[39m
define CLR_TEXT
$(1)$(2)$(CLR_CLEAR)
endef

include ./$(OBJPATH)/xzbuild.mk
LINKFLAGS	?= 
cpp_srcs	?=
c_srcs		?=
asm_srcs	?=
nasm_rcs	?=
rc_srcs		?=
ispc_srcs	?=

APPPATH		 = $(PROJPATH)$(OBJPATH)/
INCPATH		 = -I"$(PROJPATH)" -I"$(PROJPATH)3rdParty"
LDPATH		 = -L"$(APPPATH)" -L.

ISPCOBJECTS	 = $(patsubst %, $(OBJPATH)%.o, $(ispc_srcs))

DIRS		 = $(dir $(CXXOBJS) $(OTHEROBJS))
ifeq ($(XZMK_CLANG), 1) # Has problem with pch on GCC
PCH_PCH		 = $(patsubst %, $(OBJPATH)%.gch, $(PCH_HEADER))
else
PCH_PCH		 = 
endif
DEPS 		 = $(patsubst %.gch, %.d, $(PCH_PCH))


$(shell mkdir -p $(OBJPATH) $(DIRS))

ifneq ($(PCH_HEADER), )
    $(shell touch -a $(OBJPATH)/$(PCH_HEADER))
endif


all: prebuild

buildispc: $(ISPCOBJECTS)
ifneq ($(ispc_srcs), )
	@echo "$(CLR_CYAN)ispc target compiled$(CLR_CLEAR)"
endif

buildpch: buildispc $(PCH_PCH)
ifneq ($(PCH_PCH), )
	@echo "$(CLR_CYAN)precompiled header generated$(CLR_CLEAR)"
endif

prebuild: buildpch
	@echo "$(CLR_CYAN)pre build finished$(CLR_CLEAR)"


-include $(DEPS)

$(OBJPATH)%.h.gch: %.h
	$(CPPCOMPILER) $(INCPATH) $(CPPFLAGS) -x c++-header -MMD -MP -fPIC -c $< -o $@

$(OBJPATH)%.ispc.o: %.ispc
ifeq ($(PLATFORM), x64)
	$(ISPCCOMPILER) -g -O2 $< -o $(patsubst %.ispc.o, %.o, $@) -h $(patsubst %.ispc, %_ispc.h, $<) --arch=x86-64 --target=sse4,avx2 --opt=fast-math --pic
else
	$(ISPCCOMPILER) -g -O2 $< -o $(patsubst %.ispc.o, %.o, $@) -h $(patsubst %.ispc, %_ispc.h, $<) --arch=x86 --target=sse4,avx2 --opt=fast-math --pic
endif
	ld -r $(patsubst %.ispc.o, %_sse4.o, $@) $(patsubst %.ispc.o, %_avx2.o, $@) $(patsubst %.ispc.o, %.o, $@) -o $@


