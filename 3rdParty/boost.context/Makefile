#@XZBUILD
#@XZBUILD@description@A foundational library provides multitasking on a single thread

BUILD_TYPE	:= static

include $(PROJPATH)/Makebase.mk

ifeq ($(PLATFORM), x64)
	ASMSRCS	= $(wildcard asm/*x86_64_*_elf_gas.S)
else
	ASMSRCS	= $(wildcard asm/*i386_*_elf_gas.S)
endif
CPPSRCS		+= posix/stack_traits.cpp
NAME	= boost.context

include $(PROJPATH)/MakeTargets.mk
