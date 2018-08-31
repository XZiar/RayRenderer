
$(shell mkdir -p $(OBJPATH) $(DIRS))

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
	$(DYNAMICLINKER) $(INCPATH) $(LDPATH) $(CPPFLAGS) -fvisibility=hidden -shared $(CXXOBJS) $(OTHEROBJS) -Wl,-rpath='$$ORIGIN' -Wl,-rpath-link,. -Wl,--whole-archive $(DEPLIBS) -Wl,--no-whole-archive $(LIBRARYS) -o $(APPS)
else
BUILD_TYPE2	:= executable binary
$(APPS): $(CXXOBJS) $(OTHEROBJS)
	@echo "$(CLR_GREEN)linking $(CLR_MAGENTA)$(APPS)$(CLR_CLEAR)"
	$(APPLINKER) $(INCPATH) $(LDPATH) $(CPPFLAGS) $(CXXOBJS) $(OTHEROBJS) -Wl,-rpath='$$ORIGIN' -Wl,-rpath-link,. -Wl,--whole-archive $(DEPLIBS) -Wl,--no-whole-archive $(LIBRARYS) -o $(APPS)
endif
endif

buildinfo:
	@echo "$(CLR_GREEN)building $(CLR_MAGENTA)$(BUILD_TYPE2)$(CLR_CLEAR) [$(CLR_CYAN)${NAME}$(CLR_CLEAR)] [$(CLR_MAGENTA)${TARGET}$(CLR_CLEAR) version on $(CLR_MAGENTA)${PLATFORM}$(CLR_CLEAR)] to $(CLR_WHITE)${APPPATH}$(CLR_CLEAR)"
ifneq ($(CPPSRCS), )
	@echo "$(CLR_MAGENTA)C++ Sources:\n$(CLR_WHITE)${CPPSRCS}$(CLR_CLEAR)"
endif
ifneq ($(CSRCS), )
	@echo "$(CLR_MAGENTA)C Sources:\n$(CLR_WHITE)${CSRCS}$(CLR_CLEAR)"
endif
ifneq ($(ASMSRCS), )
	@echo "$(CLR_MAGENTA)ASM Sources:\n$(CLR_WHITE)${ASMSRCS}$(CLR_CLEAR)"
endif
ifneq ($(NASMSRCS), )
	@echo "$(CLR_MAGENTA)NASM Sources:\n$(CLR_WHITE)${NASMSRCS}$(CLR_CLEAR)"
endif
ifneq ($(ISPCSRCS), )
	@echo "$(CLR_MAGENTA)ISPC Sources:\n$(CLR_WHITE)${ISPCSRCS}$(CLR_CLEAR)"
endif

prebuild: buildinfo
	@echo "$(CLR_CYAN)pre build$(CLR_CLEAR)"

mainbuild: prebuild
	@echo "$(CLR_CYAN)main build$(CLR_CLEAR)"
	+@$(MAKE) BOOST_PATH=$(BOOST_PATH) PLATFORM=$(PLATFORM) TARGET=$(TARGET) PROJPATH=$(PROJPATH) --no-print-directory $(APPS)

postbuild: mainbuild
	@echo "$(CLR_CYAN)post build$(CLR_CLEAR)"

-include $(DEPS)

$(OBJPATH)%.cpp.o: %.cpp
	$(CPPCOMPILER) $(INCPATH) $(CPPFLAGS) -MMD -MP -fPIC -c $< -o $@

$(OBJPATH)%.cc.o: %.cc
	$(CPPCOMPILER) $(INCPATH) $(CPPFLAGS) -MMD -MP -fPIC -c $< -o $@

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
	$(ISPCCOMPILER) -g -O2 $< -o $(patsubst %.ispc.o, %.o, $@) -h $(patsubst %.ispc, %_ispc.h, $<) --arch=x86-64 --target=sse4,avx2 --opt=fast-math
else
	$(ISPCCOMPILER) -g -O2 $< -o $(patsubst %.ispc.o, %.o, $@) -h $(patsubst %.ispc, %_ispc.h, $<) --arch=x86 --target=sse4,avx2 --opt=fast-math
endif
	ld -r $(patsubst %.ispc.o, %_sse4.o, $@) $(patsubst %.ispc.o, %_avx2.o, $@) -o $@

.PHONY: clean

clean:
	@echo "$(CLR_GREEN)cleaning $(CLR_MAGENTA)$(BUILD_TYPE2)$(CLR_CLEAR) [$(CLR_CYAN)${NAME}$(CLR_CLEAR)]"
	$(RM) $(CXXOBJS) $(OTHEROBJS) $(DEPS) $(APPS)


