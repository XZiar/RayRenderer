BUILD_TYPE	:= static

include $(PROJPATH)Makebase.mk

ALLASMS		 = $(wildcard simd/*.asm)
AMD64ASMS	 = $(wildcard simd/*-sse2-64.asm)
EXCEPT_C	 = jccolext.c jcsample.c jdcol565.c jdcolext.c jdmrg565.c jdmrgext.c jstdhuff.c
SUBDIRS		+= $(OBJPATH)simd
INCPATH		+= -I./simd/ -I./
ifeq ($(PLATFORM), x64)
	NASMSRCS	=  $(filter-out simd/jccolext-sse2-64.asm simd/jcgryext-sse2-64.asm simd/jdcolext-sse2-64.asm simd/jdmrgext-sse2-64.asm, $(AMD64ASMS))
	CSRCS	+= simd/jsimd_x86_64.c
else
	NASMSRCS	=  $(filter-out simd/jccolext-mmx.asm simd/jcgryext-mmx.asm simd/jdcolext-mmx.asm simd/jdmrgext-mmx.asm simd/jccolext-sse2.asm simd/jcgryext-sse2.asm simd/jdcolext-sse2.asm simd/jdmrgext-sse2.asm $(AMD64ASMS), $(ALLASMS))
	CSRCS	+= simd/jsimd_i386.c
endif
CXXFLAGS += -D_LARGEFILE64_SOURCE=1 -DNO_UNDERLINE -DASMV -DWITH_SIMD -DSIZEOF_SIZE_T
NAME	= jpeg-turbo

include $(PROJPATH)MakeTargets.mk
