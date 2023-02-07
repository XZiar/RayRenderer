
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

APPDIR		 = $(SOLDIR)/$(OBJPATH)
OBJDIR		 = $(SOLDIR)/$(BUILDPATH)/$(OBJPATH)
include $(APPDIR)/xzbuild.sol.mk # per solution xzbuild settings
include $(OBJDIR)/xzbuild.proj.mk # per project xzbuild settings

CPPCOMPILER		?= $(xz_cppcompiler)
CCOMPILER		?= $(xz_ccompiler)
ASMCOMPILER		?= $(xz_cppcompiler)
NASMCOMPILER	?= nasm
ISPCCOMPILER	?= ispc
CUDACOMPILER	?= nvcc
STATICLINKER	?= ar
DYNAMICLINKER	?= $(xz_cppcompiler)
APPLINKER		?= $(xz_cppcompiler)

VERBOSE			?= 0

LINKFLAGS	?= 
LINKLIBS	?= 
cpp_srcs	?=
cpp_pch		?=
c_srcs		?=
c_pch		?=
asm_srcs	?=
nasm_rcs	?=
cuda_rcs	?=
rc_srcs		?=
ispc_srcs	?=

INCPATH		 = $(patsubst %, -I"%", $(xz_incDir))
LDPATH		 = -L"$(APPDIR)" -L.  $(patsubst %, -L"%", $(xz_libDir))
# DYNLIBS		:= $(patsubst %, -l%, $(libDynamic))
# STALIBS		:= $(patsubst %, -l%, $(libStatic))
ALLDEFFLAGS := $(patsubst %, -D%, $(xz_define))
CDEFFLAGS	:= $(patsubst %, -D%, $(c_defs))
CPPDEFFLAGS	:= $(patsubst %, -D%, $(cpp_defs))
CUDADEFFLAGS	:= $(patsubst %, -D%, $(cuda_defs))
CINCPATHS	:= $(patsubst %, -I"%", $(c_incpaths)) $(INCPATH)
CPPINCPATHS	:= $(patsubst %, -I"%", $(cpp_incpaths)) $(INCPATH)
CUDAINCPATHS	:= $(patsubst %, -I"%", $(cuda_incpaths)) $(INCPATH)
ASMINCPATHS	:= $(patsubst %, -I"%", $(asm_incpaths)) $(INCPATH)
NASMINCPATHS	:= $(patsubst %, -I"%", $(nasm_incpaths)) $(INCPATH)

### section OBJs
CXXOBJS		 = $(patsubst %, $(OBJDIR)/%.o, $(c_srcs) $(cpp_srcs) $(rc_srcs) $(cuda_srcs))
ISPCOBJS	 = $(patsubst %, $(OBJDIR)/%.o, $(ispc_srcs))
ASMOBJS		 = $(patsubst %, $(OBJDIR)/%.o, $(asm_srcs))
NASMOBJS	 = $(patsubst %, $(OBJDIR)/%.o, $(nasm_srcs))
OTHEROBJS	 = $(ASMOBJS) $(NASMOBJS)


### section PCH
PCH_CPP		 = $(patsubst %, $(OBJDIR)/%.gch, $(cpp_pch))
PCH_C		 = $(patsubst %, $(OBJDIR)/%.gch, $(c_pch))
PCH_PCH		 = $(PCH_CPP) $(PCH_C)
ifeq ($(xz_compiler), clang)
	PCHFIX	:= -Wno-pragma-once-outside-header
endif
ifneq ($(PCH_CPP), )
ifeq ($(xz_compiler), gcc) # Has problem with pch on GCC
	CPPPCH 	:= -I"$(OBJDIR)"
else ifeq ($(xz_compiler), clang)
	CPPPCH	:= -include $(cpp_pch) -include-pch $(patsubst %, $(OBJDIR)/%.gch, $(cpp_pch))
endif
endif
ifneq ($(PCH_C), )
ifeq ($(xz_compiler), gcc) # Has problem with pch on GCC
	CPCH 	:= -I"$(OBJDIR)"
else ifeq ($(xz_compiler), clang)
	CPCH	:= -include $(c_pch) -include-pch $(patsubst %, $(OBJDIR)/%.gch, $(c_pch))
endif
endif


### section LD
# ifeq ($(xz_osname), Darwin)
# LD_STATIC	:= -Wl,-all_load $(STALIBS)
# LD_DYNAMIC	:= -Wl,-noall_load $(DYNLIBS) 
# else
# LD_STATIC	:= -Wl,--whole-archive $(STALIBS)
# LD_DYNAMIC	:= -Wl,--no-whole-archive $(DYNLIBS) 
# endif


###============================================================================
### create directory
DIRS		 = $(dir $(ISPCOBJS) $(CXXOBJS) $(OTHEROBJS) $(PCH_PCH))
$(shell mkdir -p $(OBJDIR) $(DIRS))


###============================================================================
### dependent includes
DEPS 		 = $(patsubst %.o, %.d, $(CXXOBJS) $(ASMOBJS)) $(patsubst %.gch, %.d, $(PCH_PCH))
DEP_MK	:= $(SOLDIR)/$(BUILDPATH)/xzbuild.proj.json


###============================================================================
### beautify print
ifeq ($(VERBOSE), 0)
define BuildProgress
    @printf "$(CLR_GREEN)$(1) $(CLR_BLUE)[$(2)] $(CLR_WHITE)$(3) $(CLR_CLEAR)\n"
    @$(4)
endef
else
define BuildProgress
	$(4)
endef
endif


###============================================================================
### stage targets
APP		:= $(APPDIR)/$(TARGET_NAME)

### clean
ifeq ($(CLEAN), 1)
$(info docleaning......)
CLEAN_RET	:=$(shell rm -f $(PCH_PCH) $(CXXOBJS) $(ISPCOBJS) $(OTHEROBJS) $(DEPS) $(APP))
endif

### speedup & hotfix
.PHONY: all clean

.PRECIOUS: $(PCH_PCH)

.SUFFIXES:

### exposed targets
all: $(APP)
	@printf "$(CLR_WHITE)make of $(CLR_CYAN)[$(NAME)]$(CLR_WHITE) finished$(CLR_CLEAR)\n"

clean: 
	@printf "$(CLR_WHITE)clean finished$(CLR_CLEAR)\n"

### main targets
ifeq ($(BUILD_TYPE), static)
$(APP): $(CXXOBJS) $(ISPCOBJS) $(OTHEROBJS)
#	@printf "$(CLR_GREEN)linking $(CLR_MAGENTA)$(APP)$(CLR_CLEAR)\n"
	$(eval $@_bcmd := $(STATICLINKER) rcs $(APP) $(CXXOBJS) $(ISPCOBJS) $(OTHEROBJS))
	$(call BuildProgress,link   ,  lib, $(APP), $($@_bcmd))
else ifeq ($(BUILD_TYPE), dynamic)
$(APP): $(CXXOBJS) $(ISPCOBJS) $(OTHEROBJS)
#	@printf "$(CLR_GREEN)linking $(CLR_MAGENTA)$(APP)$(CLR_CLEAR)\n"
	$(eval $@_bcmd := $(DYNAMICLINKER) $(INCPATH) $(LDPATH) $(cpp_flags) -fvisibility=hidden $(LINKFLAGS) $(CXXOBJS) $(ISPCOBJS) $(OTHEROBJS) $(LINKLIBS) -o $(APP))
	$(call BuildProgress,link   ,  dll, $(APP), $($@_bcmd))
else
$(APP): $(CXXOBJS) $(ISPCOBJS) $(OTHEROBJS)
#	@printf "$(CLR_GREEN)linking $(CLR_MAGENTA)$(APP)$(CLR_CLEAR)\n"
	$(eval $@_bcmd := $(APPLINKER) $(INCPATH) $(LDPATH) $(cpp_flags) -fvisibility=hidden $(LINKFLAGS) $(CXXOBJS) $(ISPCOBJS) $(OTHEROBJS) $(LINKLIBS) -o $(APP))
	$(call BuildProgress,link   ,  exe, $(APP), $($@_bcmd))
endif

### dependent includes
-include $(DEPS)


###============================================================================
### pch targets
$(OBJDIR)/%.gch: % $(DEP_MK)
ifeq ($(xz_compiler), gcc) # Has problem with pch on GCC
	@echo "" > $(basename $@)
endif
ifneq (($(filter $<,$(cpp_pch))), ) # cpp pch
	$(eval $@_bcmd := $(CPPCOMPILER) $(CPPINCPATHS) $(cpp_flags) $(ALLDEFFLAGS) $(CPPDEFFLAGS) -x c++-header -MMD -MP -fPIC $(PCHFIX) -c $< -o $@)
else ifneq (($(filter $<,$(c_pch))), ) # c pch
	$(eval $@_bcmd := $(CPPCOMPILER) $(CINCPATHS) $(c_flags) $(ALLDEFFLAGS) $(CDEFFLAGS) -x c-header -MMD -MP -fPIC $(PCHFIX) -c $< -o $@)
else
$(error unknown pch file target)
endif
	$(call BuildProgress,compile,  pch, $<, $($@_bcmd))


###============================================================================
### cxx targets
$(OBJDIR)/%.cpp.o: %.cpp $(PCH_CPP) $(ISPCOBJS) $(DEP_MK)
	$(eval $@_bcmd := $(CPPCOMPILER) $(CPPPCH) $(CPPINCPATHS) $(cpp_flags) $(ALLDEFFLAGS) $(CPPDEFFLAGS) -Winvalid-pch -MMD -MP -fPIC -c $< -o $@)
	$(call BuildProgress,compile,  cpp, $<, $($@_bcmd))

$(OBJDIR)/%.cc.o: %.cc $(PCH_CPP) $(ISPCOBJS) $(DEP_MK)
	$(eval $@_bcmd := $(CPPCOMPILER) $(CPPPCH) $(CPPINCPATHS) $(cpp_flags) $(ALLDEFFLAGS) $(CPPDEFFLAGS) -Winvalid-pch -MMD -MP -fPIC -c $< -o $@)
	$(call BuildProgress,compile,  cpp, $<, $($@_bcmd))

$(OBJDIR)/%.c.o: %.c $(PCH_C) $(ISPCOBJS) $(DEP_MK)
	$(eval $@_bcmd := $(CCOMPILER) $(CPCH) $(CINCPATHS) $(c_flags) $(ALLDEFFLAGS) $(CDEFFLAGS) -Winvalid-pch -MMD -MP -fPIC -c $< -o $@)
	$(call BuildProgress,compile,    c, $<, $($@_bcmd))


###============================================================================
### asm targets
$(OBJDIR)/%.asm.o: %.asm $(DEP_MK)
	$(eval $@_bcmd := $(NASMCOMPILER) $(NASMINCPATHS) $(nasm_flags) $< -o $@)
	$(call BuildProgress,compile, nasm, $<, $($@_bcmd))

$(OBJDIR)/%.S.o: %.S $(DEP_MK)
	$(eval $@_bcmd := $(ASMCOMPILER) $(ASMINCPATHS) $(asm_flags) -MMD -MP -fPIC -c $< -o $@)
	$(call BuildProgress,compile,  asm, $<, $($@_bcmd))


###============================================================================
### rc targets
$(OBJDIR)/%.rc.o: %.rc $(DEP_MK)
	python3 $(SOLDIR)/$(xz_xzbuildPath)/ResourceCompiler.py $< $(xz_platform) $(OBJDIR)


###============================================================================
### ispc targets
# ispc's dependency file is still buggy, so only generate it but not use it
$(OBJDIR)/%.ispc.o: %.ispc $(DEP_MK)
	$(eval $@_bcmd := $(ISPCCOMPILER) $< -M -MF $(patsubst %.ispc.o, %.ispc.d, $@) -o $(OBJDIR)/$*.o -h $(SOLDIR)/$(BUILDPATH)/$*_ispc.h $(ispc_flags) -MT $@)
	$(call BuildProgress,compile, ispc, $<, $($@_bcmd))
#	sed -i '1s/^(null):/$@:/g' $(patsubst %.ispc.o, %.ispc.d, $@)
	$(eval $@_bcmd := ld -r $(patsubst %, $(OBJDIR)/$*_%.o, $(ispc_targets)) $(patsubst %.ispc.o, %.o, $@) -o $@)
	$(call BuildProgress,merge  , ispc, $<, $($@_bcmd))


###============================================================================
### cuda targets
$(OBJDIR)/%.cu.o: %.cu $(PCH_CPP) $(DEP_MK)
	$(eval $@_bcmd := $(CUDACOMPILER) $(CUDAINCPATHS) $(cuda_flags) $(CUDADEFFLAGS) -c $< -MM -MF $(patsubst %.cu.o, %.cu.d, $@))
	$(call BuildProgress,depend , cuda, $<, $($@_bcmd))
	$(eval $@_bcmd := $(CUDACOMPILER) $(CUDAINCPATHS) $(cuda_flags) $(CUDADEFFLAGS) -c $< -o $@)
	$(call BuildProgress,compile, cuda, $<, $($@_bcmd))
