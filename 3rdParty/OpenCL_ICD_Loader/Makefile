#@XZBUILD
#@XZBUILD@description@The OpenCL ICD Loader

BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	     = OpenCLICDLoader
CXXFLAGS    += -DOpenCL_EXPORTS -DCL_TARGET_OPENCL_VERSION=220
LIBRARYS	+= -ldl
#using c11 will lead to undeifned enum error
CVERSION	 =
EXCEPT_C    += icd_windows.c icd_windows_hkr.c

include $(PROJPATH)/MakeTargets.mk
