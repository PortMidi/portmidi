\swigwin-1.3.40\swig.exe -python -module pypmbase -I../pm_common -o pypmbase.c pypmbase.i

mkdir build
mkdir build\temp.win32-3.1
mkdir build\temp.win32-3.1\Release
mkdir build\lib.win32-3.1

rem vcvarsall.bat should only run once because it appends to the path
path | findstr "VCPackages"
if errorlevel 0 if not errorlevel 1 goto found
rem not found, set environment for visual c
call "C:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
@echo on

:found
"C:\Program Files\Microsoft Visual Studio 9.0\VC\BIN\cl.exe" /c /nologo /Ox /MD /W3 /GS- /DNDEBUG -I../porttime -IC:\Python31\include -IC:\Python31\PC -I..\pm_common /Tcpypmbase.c /Fobuild\temp.win32-3.1\Release\pypmbase.obj /DWIN32

"C:\Program Files\Microsoft Visual Studio 9.0\VC\BIN\link.exe" /DLL /nologo /INCREMENTAL:NO /LIBPATH:../Release /LIBPATH:C:\Python31\libs /LIBPATH:C:\Python31\PCbuild portmidi.lib winmm.lib /EXPORT:PyInit__pypmbase build\temp.win32-3.1\Release\pypmbase.obj /OUT:build\lib.win32-3.1\_pypmbase.pyd /IMPLIB:build\temp.win32-3.1\Release\_pypmbase.lib /MANIFESTFILE:build\temp.win32-3.1\Release\_pypmbase.pyd.manifest

"C:\Program Files\Microsoft SDKs\Windows\v6.0A\bin\mt.exe" -nologo -manifest build\temp.win32-3.1\Release\_pypmbase.pyd.manifest -outputresource:build\lib.win32-3.1\_pypmbase.pyd;2
