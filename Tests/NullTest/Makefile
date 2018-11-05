#@XZBUILD
#@XZBUILD@description@minimized test
#@XZBUILD@dependency@fmt, boost.stacktrace, StringCharset

BUILD_TYPE	:= executable

include $(PROJPATH)/Makebase.mk

NAME	= NullTest

DEPLIBS		+= -lfmt
LIBRARYS	+= -lboost.stacktrace -lStringCharset

include $(PROJPATH)/MakeTargets.mk

