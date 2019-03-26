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
CPPPCH		:= 
LIBRARYS	:= $(patsubst %, -l%, $(libDynamic))
DEPLIBS		:= $(patsubst %, -l%, $(libStatic))


ISPCFSRCS	 = $(foreach tar,$(ISPC_TARGETS),$(patsubst %.ispc, %_$(tar).ispc, $(ISPCSRCS)))
CXXOBJS		 = $(patsubst %, $(OBJPATH)%.o, $(c_srcs) $(cpp_srcs) $(rc_srcs))
OTHEROBJS	 = $(patsubst %, $(OBJPATH)%.o, $(asm_srcs) $(nasm_srcs) $(ispc_srcs))
DIRS		 = $(dir $(CXXOBJS) $(OTHEROBJS))
ifeq ($(XZMK_CLANG), 1) # Has problem with pch on GCC
PCH_PCH		 = $(patsubst %, $(OBJPATH)%.gch, $(PCH_HEADER))
else
PCH_PCH		 = 
endif
DEPS 		 = $(patsubst %.o, %.d, $(CXXOBJS)) $(patsubst %.gch, %.d, $(PCH_PCH))



ifeq ($(BUILD_TYPE), static)
APPS		 = $(APPPATH)lib$(NAME).a
else
ifeq ($(BUILD_TYPE), dynamic)
APPS		 = $(APPPATH)lib$(NAME).so
else
APPS		 = $(APPPATH)$(NAME)
endif
endif

all: postbuild




$(shell mkdir -p $(OBJPATH) $(DIRS))
ifneq ($(PCH_HEADER), )
    $(shell touch -a $(OBJPATH)/$(PCH_HEADER))
ifneq ($(XZMK_CLANG), 1)
	CPPPCH += -I"$(OBJPATH)"
endif
	CPPPCH += -include $(PCH_HEADER)
ifeq ($(XZMK_CLANG), 1)
	CPPPCH += -include-pch $(PCH_PCH)
endif
endif

ifeq ($(BUILD_TYPE), static)
BUILD_TYPE2	:= static library
$(APPS): $(CXXOBJS) $(OTHEROBJS)
	@echo "$(CLR_GREEN)linking $(CLR_MAGENTA)$(APPS)$(CLR_CLEAR)"
	$(STATICLINKER) rcs $(APPS) $(CXXOBJS) $(OTHEROBJS)
else
ifeq ($(BUILD_TYPE), dynamic)
BUILD_TYPE2	:= dynamic library
$(APPS): $(CXXOBJS) $(OTHEROBJS)
	@echo "$(CLR_GREEN)linking $(CLR_MAGENTA)$(APPS)$(CLR_CLEAR)"
	$(DYNAMICLINKER) $(INCPATH) $(LDPATH) $(CPPFLAGS) $(LINKFLAGS) -fvisibility=hidden -shared $(CXXOBJS) $(OTHEROBJS) -Wl,-rpath='$$ORIGIN' -Wl,-rpath-link,. -Wl,--whole-archive $(DEPLIBS) -Wl,--no-whole-archive $(LIBRARYS) -o $(APPS)
else
BUILD_TYPE2	:= executable binary
$(APPS): $(CXXOBJS) $(OTHEROBJS)
	@echo "$(CLR_GREEN)linking $(CLR_MAGENTA)$(APPS)$(CLR_CLEAR)"
	$(APPLINKER) $(INCPATH) $(LDPATH) $(CPPFLAGS) $(LINKFLAGS) $(CXXOBJS) $(OTHEROBJS) -Wl,-rpath='$$ORIGIN' -Wl,-rpath-link,. -Wl,--whole-archive $(DEPLIBS) -Wl,--no-whole-archive $(LIBRARYS) -o $(APPS)
endif
endif

buildinfo:
	@echo "$(CLR_GREEN)building $(CLR_MAGENTA)$(BUILD_TYPE2)$(CLR_CLEAR) [$(CLR_CYAN)${NAME}$(CLR_CLEAR)] [$(CLR_MAGENTA)${TARGET}$(CLR_CLEAR) version on $(CLR_MAGENTA)${PLATFORM}$(CLR_CLEAR)] to $(CLR_WHITE)${APPPATH}$(CLR_CLEAR)"
ifneq ($(CPPSRCS), )
	@echo "$(CLR_MAGENTA)C++ Sources:\n$(CLR_WHITE)${cpp_srcs}$(CLR_CLEAR)"
endif
ifneq ($(CSRCS), )
	@echo "$(CLR_MAGENTA)C Sources:\n$(CLR_WHITE)${c_srcs}$(CLR_CLEAR)"
endif
ifneq ($(ASMSRCS), )
	@echo "$(CLR_MAGENTA)ASM Sources:\n$(CLR_WHITE)${asm_srcs}$(CLR_CLEAR)"
endif
ifneq ($(NASMSRCS), )
	@echo "$(CLR_MAGENTA)NASM Sources:\n$(CLR_WHITE)${nasm_srcs}$(CLR_CLEAR)"
endif
ifneq ($(ISPCSRCS), )
	@echo "$(CLR_MAGENTA)ISPC Sources:\n$(CLR_WHITE)${ispc_srcs}$(CLR_CLEAR)"
endif

prebuild: buildinfo
	@echo "$(CLR_CYAN)pre build$(CLR_CLEAR)"

buildpch: prebuild
ifneq ($(PCH_PCH), )
	@echo "$(CLR_CYAN)preparing precompiled header$(CLR_CLEAR)"
	+@$(MAKE) BOOST_PATH="$(BOOST_PATH)" PLATFORM=$(PLATFORM) TARGET=$(TARGET) PROJPATH="$(PROJPATH)" --no-print-directory $(PCH_PCH)
endif

mainbuild: buildpch
	@echo "$(CLR_CYAN)main build$(CLR_CLEAR)"
	+@$(MAKE) BOOST_PATH="$(BOOST_PATH)" PLATFORM=$(PLATFORM) TARGET=$(TARGET) PROJPATH="$(PROJPATH)" --no-print-directory $(APPS)

postbuild: mainbuild
	@echo "$(CLR_CYAN)post build$(CLR_CLEAR)"

-include $(DEPS)

$(OBJPATH)%.h.gch: %.h
	$(CPPCOMPILER) $(INCPATH) $(CPPFLAGS) -x c++-header -MMD -MP -fPIC -c $< -o $@

$(OBJPATH)%.cpp.o: %.cpp
	$(CPPCOMPILER) $(CPPPCH) $(INCPATH) $(CPPFLAGS) -MMD -MP -fPIC -c $< -o $@

$(OBJPATH)%.cc.o: %.cc
	$(CPPCOMPILER) $(CPPPCH) $(INCPATH) $(CPPFLAGS) -MMD -MP -fPIC -c $< -o $@

$(OBJPATH)%.c.o: %.c
	$(CCOMPILER) $(INCPATH) $(CFLAGS) -MMD -MP -fPIC -c $< -o $@

$(OBJPATH)%.asm.o: %.asm
	$(NASMCOMPILER) $(INCPATH) $(NASMFLAGS) $< -o $@

$(OBJPATH)%.S.o: %.S
	$(ASMCOMPILER) $(INCPATH) $(CXXFLAGS) -MMD -MP -fPIC -c $< -o $@

$(OBJPATH)%.rc.o: %.rc
	python3 $(PROJPATH)/ResourceCompiler.py $< $(PLATFORM) $(OBJPATH)

$(OBJPATH)%.ispc.o: %.ispc
ifeq ($(PLATFORM), x64)
	$(ISPCCOMPILER) -g -O2 $< -o $(patsubst %.ispc.o, %.o, $@) -h $(patsubst %.ispc, %_ispc.h, $<) --arch=x86-64 --target=sse4,avx2 --opt=fast-math --pic
else
	$(ISPCCOMPILER) -g -O2 $< -o $(patsubst %.ispc.o, %.o, $@) -h $(patsubst %.ispc, %_ispc.h, $<) --arch=x86 --target=sse4,avx2 --opt=fast-math --pic
endif
	ld -r $(patsubst %.ispc.o, %_sse4.o, $@) $(patsubst %.ispc.o, %_avx2.o, $@) $(patsubst %.ispc.o, %.o, $@) -o $@

.PHONY: clean

clean:
	@echo "$(CLR_GREEN)cleaning $(CLR_MAGENTA)$(BUILD_TYPE2)$(CLR_CLEAR) [$(CLR_CYAN)${NAME}$(CLR_CLEAR)]"
	$(RM) $(CXXOBJS) $(OTHEROBJS) $(DEPS) $(APPS)


