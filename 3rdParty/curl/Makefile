#@XZBUILD
#@XZBUILD@description@A library for transferring data with URL syntax
#@XZBUILD@version@7.63.0
#@XZBUILD@dependency@zlib-ng, libressl

BUILD_TYPE	:= static

include $(PROJPATH)/Makebase.mk

INCPATH		+= -I./include/ -I./ -I"../zlib-ng" -I"../libressl/include"

EXCEPT_C	+= http2.c openldap.c ssh-libssh.c krb5.c ldap.c ssh.c curl_des.c amigaos.c
CSRCS		+= $(wildcard vauth/*.c) vtls/vtls.c vtls/openssl.c
CSRCS		:= $(filter-out %_sspi.c %_gssapi.c %_win32.c, $(CSRCS))

CVERSION	 =
CXXFLAGS	+= -Wpointer-arith -Wwrite-strings -Wunused -Wshadow -Winline -Wnested-externs -Wmissing-declarations -Wmissing-prototypes -Wno-long-long -Wfloat-equal -Wno-multichar -Wsign-compare -Wundef -Wno-format-nonliteral -Wendif-labels -Wstrict-prototypes -Wdeclaration-after-statement -Wstrict-aliasing=3 -Wcast-align -Wtype-limits -Wold-style-declaration -Wmissing-parameter-type -Wempty-body -Wclobbered -Wignored-qualifiers -Wconversion -Wno-sign-conversion -Wvla -Wdouble-promotion -Wno-system-headers
CXXFLAGS	+= -DHAVE_CONFIG_H -DCURL_STATICLIB -DBUILDING_LIBCURL -DCURL_HIDDEN_SYMBOLS


ifeq ($(TARGET), Debug)
	CXXFLAGS	+= -DDEBUGBUILD
endif

NAME	= curl

include $(PROJPATH)/MakeTargets.mk
