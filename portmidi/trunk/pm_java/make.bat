javac pmdefaults/*.java
javac jportmidi/*.java
rem -- temporarily copy portmusic_logo.png here to add to jar file
copy pmdefaults\portmusic_logo.png .
mkdir win32
copy ..\debug\pmjni.dll win32\pmjni.dll
jar cmf pmdefaults\manifest.txt win32\pmdefaults.jar pmdefaults\*.class portmusic_logo.png jportmidi\*.class
rem -- clean up temporary image file now that it is in the jar file
del portmusic_logo.png
copy JavaExe.exe win32\pmdefaults.exe
UpdateRsrcJavaExe -run -exe=win32\pmdefaults.exe -ico=pmdefaults\pmdefaults.ico
copy pmdefaults\readme-win32.txt win32\README.txt
copy pmdefaults\pmdefaults-license.txt win32\license.txt
echo "You can run pmdefault.exe in win32"


