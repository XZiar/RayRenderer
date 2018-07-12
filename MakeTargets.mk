
default: $(NAME)

ifeq ($(BUILD_TYPE), static)
BUILD_TYPE2	:= static library
$(NAME): $(OBJS) $(ASMOBJS)
	@echo "$(CLR_GREEN)linking $(CLR_MAGENTA)$(APPS)$(CLR_CLEAR)"
	$(STATICLINKER) rcs $(APPS) $(OBJS) $(ASMOBJS)
else
ifeq ($(BUILD_TYPE), dynamic)
BUILD_TYPE2	:= dynamic library
$(NAME): $(OBJS) $(ASMOBJS)
	@echo "$(CLR_GREEN)linking $(CLR_MAGENTA)$(APPS)$(CLR_CLEAR)"
	$(DYNAMICLINKER) $(INCPATH) $(LDPATH) $(CPPFLAGS) -shared $(OBJS) $(ASMOBJS) -Wl,--whole-archive $(DEPLIBS) -Wl,--no-whole-archive $(LIBRARYS) -o $(APPS)
else
BUILD_TYPE2	:= executable binary
$(NAME): $(OBJS) $(ASMOBJS)
	@echo "$(CLR_GREEN)linking $(CLR_MAGENTA)$(APPS)$(CLR_CLEAR)"
	$(APPLINKER) $(INCPATH) $(LDPATH) $(CPPFLAGS) $(OBJS) $(ASMOBJS) -Wl,-rpath=. -Wl,--whole-archive $(DEPLIBS) -Wl,--no-whole-archive $(LIBRARYS) -o $(APPS)
endif
endif

mkobjdir:
	mkdir -p $(OBJPATH) $(SUBDIRS)

dependencyinfo:
	@echo "$(CLR_GREEN)building $(CLR_MAGENTA)$(BUILD_TYPE2)$(CLR_CLEAR) [$(CLR_CYAN)${NAME}$(CLR_CLEAR)] [$(CLR_MAGENTA)${TARGET}$(CLR_CLEAR) version on $(CLR_MAGENTA)${PLATFORM}$(CLR_CLEAR)] to $(CLR_WHITE)${APPPATH}$(CLR_CLEAR)"
	@echo "$(CLR_MAGENTA)C++ Sources:\n$(CLR_WHITE)${CPPSRCS}$(CLR_CLEAR)"
	@echo "$(CLR_MAGENTA)C Sources:\n$(CLR_WHITE)${CSRCS}$(CLR_CLEAR)"
	@echo "$(CLR_MAGENTA)ASM Sources:\n$(CLR_WHITE)${ASMSRCS}$(CLR_CLEAR)"
	@echo "$(CLR_MAGENTA)NASM Sources:\n$(CLR_WHITE)${NASMSRCS}$(CLR_CLEAR)"

$(OBJPATH)%.cpp.o: %.cpp mkobjdir dependencyinfo
	$(CPPCOMPILER) $(INCPATH) $(CPPFLAGS) -MMD -MP -fPIC -c $< -o $@

$(OBJPATH)%.cc.o: %.cc mkobjdir dependencyinfo
	$(CPPCOMPILER) $(INCPATH) $(CPPFLAGS) -MMD -MP -fPIC -c $< -o $@

$(OBJPATH)%.c.o: %.c mkobjdir dependencyinfo
	$(CCOMPILER) $(INCPATH) $(CFLAGS) -MMD -MP -fPIC -c $< -o $@

$(OBJPATH)%.asm.o: %.asm mkobjdir dependencyinfo
	$(NASMCOMPILER) $(INCPATH) $(ASMFLAGS) $< -o $@

$(OBJPATH)%.S.o: %.S mkobjdir dependencyinfo
	$(ASMCOMPILER) $(INCPATH) $(CXXFLAGS) -MMD -MP -fPIC -c $< -o $@

.PHONY: clean

clean:
	@echo "$(CLR_GREEN)cleaning $(CLR_MAGENTA)$(BUILD_TYPE2)$(CLR_CLEAR) [$(CLR_CYAN)${NAME}$(CLR_CLEAR)]"
	$(RM) $(OBJS) $(ASMOBJS) $(DEPS) $(APPS)

-include $(DEPS)

