BUILD_TYPE	:= static

include $(PROJPATH)/Makebase.mk

#ISPC_TARGETS	+= sse4 avx2
NAME	= ispc_texcomp

include $(PROJPATH)/MakeTargets.mk

#prebuild: $(patsubst %, $(OBJPATH)%.o, $(ISPCSRCS))
	

#      <Command Condition="'$(Platform)'=='Win32'">ispc -g -O2 "%(Filename).ispc" -o "$(Configuration)\(Filename).obj" -h "$(ProjectDir)%(Filename)_ispc.h" --arch=x86 --target=sse4,avx2 --opt=fast-math</Command>
#      <Outputs Condition="'$(Platform)'=='Win32'">$(Configuration)\(Filename).obj;$(Configuration)\(Filename)_sse4.obj;$(Configuration)\(Filename)_avx2.obj;</Outputs>
#      <Command Condition="'$(Platform)'=='x64'">ispc -g -O2 "%(Filename).ispc" -o "$(Platform)\$(Configuration)\%(Filename).obj" -h "$(ProjectDir)%(Filename)_ispc.h" --arch=x86-64 --target=sse4,avx2 --opt=fast-math</Command>
#      <Outputs Condition="'$(Platform)'=='x64'">$(Platform)\$(Configuration)\%(Filename).obj;$(Platform)\$(Configuration)\%(Filename)_sse4.obj;$(Platform)\$(Configuration)\%(Filename)_avx2.obj;</Outputs>
