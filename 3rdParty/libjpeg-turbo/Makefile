BUILD_TYPE	:= static

include $(PROJPATH)/Makebase.mk

I386ASMS	 = $(wildcard simd/i386/*.asm)
AMD64ASMS	 = $(wildcard simd/x86-64/*.asm)
EXCEPT_C	 = jccolext.c jdcol565.c jdcolext.c jdmrg565.c jdmrgext.c jstdhuff.c
INCPATH		+= -I./simd/ -I./
ifeq ($(PLATFORM), x64)
	NASMSRCS	 =  $(filter-out simd/x86-64/jccolext-avx2.asm simd/x86-64/jccolext-sse2.asm simd/x86-64/jcgryext-avx2.asm simd/x86-64/jcgryext-sse2.asm simd/x86-64/jdcolext-avx2.asm simd/x86-64/jdcolext-sse2.asm simd/x86-64/jdmrgext-avx2.asm simd/x86-64/jdmrgext-sse2.asm, $(AMD64ASMS))
	CSRCS		+= simd/x86-64/jsimd-x86-64.c
	INCPATH		+= -I"./simd/x86-64/"
else
	NASMSRCS	 =  $(filter-out simd/i386/jccolext-avx2.asm simd/i386/jccolext-sse2.asm simd/i386/jccolext-mmx.asm simd/i386/jcgryext-avx2.asm simd/i386/jcgryext-sse2.asm simd/i386/jcgryext-mmx.asm simd/i386/jdcolext-avx2.asm simd/i386/jdcolext-sse2.asm simd/i386/jdmrgext-avx2.asm simd/i386/jdmrgext-sse2.asm simd/i386/jdmrgext-mmx.asm, $(I386ASMS))
	CSRCS		+= simd/i386/jsimd-i386.c
	INCPATH		+= -I"./simd/i386/"
endif
CXXFLAGS += -D_LARGEFILE64_SOURCE=1 -DNO_PUTENV -DBMP_SUPPORTED -DPPM_SUPPORTED
NAME	= jpeg-turbo

include $(PROJPATH)/MakeTargets.mk
