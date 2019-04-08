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


include $(SOLPATH)/$(OBJPATH)/xzbuild.sol.mk # per solution xzbuild settings
include ./$(OBJPATH)/xzbuild.proj.mk # per project xzbuild settings

LINKFLAGS	?= 
cpp_srcs	?=
cpp_pch		?=
c_srcs		?=
c_pch		?=
asm_srcs	?=
nasm_rcs	?=
rc_srcs		?=
ispc_srcs	?=

APPPATH		 = $(SOLPATH)/$(OBJPATH)/
INCPATH		 = -I"$(SOLPATH)" -I"$(SOLPATH)/3rdParty" $(patsubst %, -I"%", $(xz_incDir))
LDPATH		 = -L"$(APPPATH)" -L.
DYNLIBS		:= $(patsubst %, -l%, $(libDynamic))
STALIBS		:= $(patsubst %, -l%, $(libStatic))
CDEFFLAGS	:= $(patsubst %, -D%, $(c_defs))
CPPDEFFLAGS	:= $(patsubst %, -D%, $(cpp_defs))
CINCPATHS	:= $(patsubst %, -I"%", $(c_incpaths)) $(INCPATH)
CPPINCPATHS	:= $(patsubst %, -I"%", $(cpp_incpaths)) $(INCPATH)
ASMINCPATHS	:= $(patsubst %, -I"%", $(asm_incpaths)) $(INCPATH)
NASMINCPATHS	:= $(patsubst %, -I"%", $(nasm_incpaths)) $(INCPATH)


### section OBJs
CXXOBJS		 = $(patsubst %, $(OBJPATH)/%.o, $(c_srcs) $(cpp_srcs) $(rc_srcs))
ISPCOBJECTS	 = $(patsubst %, $(OBJPATH)/%.o, $(ispc_srcs))
OTHEROBJS	 = $(patsubst %, $(OBJPATH)/%.o, $(asm_srcs) $(nasm_srcs))


### section PCH
PCH_CPP		 = $(patsubst %, $(OBJPATH)/%.gch, $(cpp_pch))
PCH_C		 = $(patsubst %, $(OBJPATH)/%.gch, $(c_pch))
PCH_PCH		 = $(PCH_CPP) $(PCH_C)
ifeq ($(xz_compiler), clang)
	PCHFIX	:= -Wno-pragma-once-outside-header
endif
ifneq ($(PCH_CPP), )
ifeq ($(xz_compiler), gcc) # Has problem with pch on GCC
	CPPPCH 	:= -I"$(OBJPATH)"
else ifeq ($(xz_compiler), clang)
	CPPPCH	:= -include $(cpp_pch) -include-pch $(patsubst %, $(OBJPATH)/%.gch, $(cpp_pch))
endif
endif
ifneq ($(PCH_C), )
ifeq ($(xz_compiler), gcc) # Has problem with pch on GCC
	CPCH 	:= -I"$(OBJPATH)"
else ifeq ($(xz_compiler), clang)
	CPCH	:= -include $(c_pch) -include-pch $(patsubst %, $(OBJPATH)/%.gch, $(c_pch))
endif
endif


###============================================================================
### create directory
DIRS		 = $(dir $(ISPCOBJECTS) $(CXXOBJS) $(OTHEROBJS) $(PCH_PCH))
$(shell mkdir -p $(OBJPATH) $(DIRS))


###============================================================================
### dependent includes
DEPS 		 = $(patsubst %.o, %.d, $(CXXOBJS)) $(patsubst %.gch, %.d, $(PCH_PCH))
DEP_MK	:= xzbuild.proj.json


###============================================================================
### stage targets
ifeq ($(BUILD_TYPE), static)
APP		:= $(APPPATH)lib$(NAME).a
else ifeq ($(BUILD_TYPE), dynamic)
APP		:= $(APPPATH)lib$(NAME).so
else ifeq ($(BUILD_TYPE), executable)
APP		:= $(APPPATH)$(NAME)
else
$(error unknown build type)
endif

### clean
ifeq ($(CLEAN), 1)
$(info docleaning......)
CLEAN_RET	:=$(shell rm -f $(PCH_PCH) $(CXXOBJS) $(ISPCOBJECTS) $(OTHEROBJS) $(DEPS) $(APP))
endif

### exposed targets
all: $(APP)
	@echo "$(CLR_WHITE)make of $(CLR_CYAN)[$(NAME)]$(CLR_WHITE) finished$(CLR_CLEAR)"

clean: 
	@echo "$(CLR_WHITE)clean finished$(CLR_CLEAR)"

### main targets
ifeq ($(BUILD_TYPE), static)
$(APP): $(CXXOBJS) $(OTHEROBJS)
	@echo "$(CLR_GREEN)linking $(CLR_MAGENTA)$(APP)$(CLR_CLEAR)"
	$(STATICLINKER) rcs $(APP) $(CXXOBJS) $(ISPCOBJECTS) $(OTHEROBJS)
else ifeq ($(BUILD_TYPE), dynamic)
$(APP): $(CXXOBJS) $(OTHEROBJS)
	@echo "$(CLR_GREEN)linking $(CLR_MAGENTA)$(APP)$(CLR_CLEAR)"
	$(DYNAMICLINKER) $(INCPATH) $(LDPATH) $(cpp_flags) $(LINKFLAGS) -fvisibility=hidden -shared $(CXXOBJS) $(ISPCOBJECTS) $(OTHEROBJS) -Wl,-rpath='$$ORIGIN' -Wl,-rpath-link,. -Wl,--whole-archive $(STALIBS) -Wl,--no-whole-archive $(DYNLIBS) -o $(APP)
else
$(APP): $(CXXOBJS) $(OTHEROBJS)
	@echo "$(CLR_GREEN)linking $(CLR_MAGENTA)$(APP)$(CLR_CLEAR)"
	$(APPLINKER) $(INCPATH) $(LDPATH) $(cpp_flags) $(LINKFLAGS) $(CXXOBJS) $(ISPCOBJECTS) $(OTHEROBJS) -Wl,-rpath='$$ORIGIN' -Wl,-rpath-link,. -Wl,--whole-archive $(STALIBS) -Wl,--no-whole-archive $(DYNLIBS) -o $(APP)
endif

### dependent includes
-include $(DEPS)


###============================================================================
### pch targets
$(OBJPATH)/%.gch: % $(DEP_MK)
ifeq ($(xz_compiler), gcc) # Has problem with pch on GCC
	@echo "#pragma once" > $(basename $@)
endif
ifneq (($(filter $<,$(cpp_pch))), ) # cpp pch
	$(CPPCOMPILER) $(CPPINCPATHS) $(cpp_flags) $(CPPDEFFLAGS) -x c++-header -MMD -MP -fPIC $(PCHFIX) -c $< -o $@
else ifneq (($(filter $<,$(c_pch))), ) # c pch
	$(CPPCOMPILER) $(CINCPATHS) $(c_flags) $(CDEFFLAGS) -x c-header -MMD -MP -fPIC $(PCHFIX) -c $< -o $@
else
$(error unknown pch file target)
endif


###============================================================================
### cxx targets
$(OBJPATH)/%.cpp.o: %.cpp $(PCH_CPP) $(ISPCOBJECTS) $(DEP_MK)
	$(CPPCOMPILER) $(CPPPCH) $(CPPINCPATHS) $(cpp_flags) $(CPPDEFFLAGS) -Winvalid-pch -MMD -MP -fPIC -c $< -o $@

$(OBJPATH)/%.cc.o: %.cc $(PCH_CPP) $(ISPCOBJECTS) $(DEP_MK)
	$(CPPCOMPILER) $(CPPPCH) $(CPPINCPATHS) $(cpp_flags) $(CPPDEFFLAGS) -Winvalid-pch -MMD -MP -fPIC -c $< -o $@

$(OBJPATH)/%.c.o: %.c $(PCH_C) $(ISPCOBJECTS) $(DEP_MK)
	$(CCOMPILER) $(CPCH) $(CINCPATHS) $(c_flags) $(CDEFFLAGS) -Winvalid-pch -MMD -MP -fPIC -c $< -o $@


###============================================================================
### asm targets
$(OBJPATH)/%.asm.o: %.asm $(DEP_MK)
	$(NASMCOMPILER) $(NASMINCPATHS) $(nasm_flags) $< -o $@

$(OBJPATH)/%.S.o: %.S $(DEP_MK)
	$(ASMCOMPILER) $(ASMINCPATHS) $(asm_flags) -MMD -MP -fPIC -c $< -o $@


###============================================================================
### rc targets
$(OBJPATH)/%.rc.o: %.rc $(DEP_MK)
	python3 $(SOLPATH)/ResourceCompiler.py $< $(xz_platform) $(OBJPATH)


###============================================================================
### ispc targets
$(OBJPATH)/%.ispc.o: %.ispc $(DEP_MK)
	$(ISPCCOMPILER) $< -o $(OBJPATH)/$*.o -h $*_ispc.h $(ispc_flags) 
	ld -r $(patsubst %, $(OBJPATH)/$*_%.o, $(ispc_targets)) $(patsubst %.ispc.o, %.o, $@) -o $@


