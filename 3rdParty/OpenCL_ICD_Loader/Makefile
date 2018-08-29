BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	     = OpenCLICDLoader
CXXFLAGS    += -DOpenCL_EXPORTS
#using c11 will lead to undeifned enum error
CVERSION	 =
EXCEPT_C    += icd_windows.c icd_windows_hkr.c

include $(PROJPATH)/MakeTargets.mk
