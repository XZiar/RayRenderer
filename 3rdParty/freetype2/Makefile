BUILD_TYPE	:= static

include $(PROJPATH)/Makebase.mk

INCPATH		+= -I"." -I"../zlib-ng" -I"../libpng"
DEPLIBS		+= -lzlib-ng -lpng
CXXFLAGS	+= -DFT2_BUILD_LIBRARY
ifeq ($(TARGET), Debug)
	CXXFLAGS	+= -DFT_DEBUG_LEVEL_ERROR -DFT_DEBUG_LEVEL_TRACE
endif
CSRCS		 = autofit/autofit.c base/ftbase.c base/ftbbox.c base/ftbdf.c base/ftbitmap.c base/ftcid.c base/ftdebug.c base/ftfstype.c\
base/ftgasp.c base/ftglyph.c base/ftgxval.c base/ftinit.c base/ftmm.c base/ftotval.c base/ftpatent.c base/ftpfr.c\
base/ftstroke.c base/ftsynth.c base/ftsystem.c base/fttype1.c base/ftwinfnt.c bdf/bdf.c cache/ftcache.c cff/cff.c\
cid/type1cid.c gzip/ftgzip.c lzw/ftlzw.c pcf/pcf.c pfr/pfr.c psaux/psaux.c pshinter/pshinter.c psnames/psnames.c\
raster/raster.c sfnt/sfnt.c smooth/smooth.c truetype/truetype.c type1/type1.c type42/type42.c winfonts/winfnt.c


NAME	= freetype2

include $(PROJPATH)/MakeTargets.mk

