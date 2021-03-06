%.o : PATH=/c/media/common/tools/msvc8/VC/bin\:/c/media/common/tools/msvc8/Common7/IDE:/usr/bin
make.exe : PATH=/c/media/common/tools/msvc8/VC/bin\:/c/media/common/tools/msvc8/Common7/IDE:/usr/bin

CPPFLAGS:=$(filter debug, $(BUILD_OPTIONS))
CXXFLAGS:=$(filter debug, $(BUILD_OPTIONS))

CPPFLAGS:=$(subst debug,/DYYDEBUG /DDEBUG,$(CPPFLAGS))
CXXFLAGS:=$(subst debug,/DDEBUG,$(CXXFLAGS))

%.o : %.cc
	cl.exe /nologo /MTd /Zi /EHsc /Gm /W3 /wd4244 /wd4018 /RTC1 $(CXXFLAGS) -c $< /Fo$@ /Ic:\\media\\common\\tools\\msvc8\\VC\\include /Ic:\\media\\common\\tools\\msvc8\\VC\\PlatformSDK\\Include /Fdmake.pdb

%.o : %.c
	cl.exe /nologo /MTd /Zi /EHsc /Gm /W3 /wd4244 /wd4018 /RTC1 $(CPPFLAGS) -c $< /Fo$@ /Ic:\\media\\common\\tools\\msvc8\\VC\\include /Ic:\\media\\common\\tools\\msvc8\\VC\\PlatformSDK\\Include /Fdmake.pdb

make.exe : $(OBJS)
	/c/media/common/tools/msvc8/VC/bin/link /DEBUG /NOLOGO /LIBPATH:c:\\media\\common\\tools\\msvc8\\VC\\lib /SUBSYSTEM:CONSOLE /DYNAMICBASE:NO /MACHINE:X86 /LIBPATH:c:\\media\\common\\tools\\msvc8\\VC\\PlatformSDK\\Lib /PDB:make.pdb /out:$@ $^
