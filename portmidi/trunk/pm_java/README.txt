README.txt
Roger B. Dannenberg
1 Jan 2009

This directory was created to implement PmDefaults, a program to
set default input and output devices for PortMidi applications.

There are three main sub-projects here:
  1) pmjni -- a JNI (Java Native Interface) to access PortMidi
  2) jportmidi -- a Java class to access PortMidi (uses pmjni)
  3) pmdefaults -- the PmDefaults application (uses jportmidi)

For Mac OS X, you should build the PmDefaults application in Xcode.

Fow Win32, you should first build everything including the portmidi dll
(that will be used by the Java application) using Visual C++ and a
provided .sln file in the portmidi home directory. Then, run make.bat 
in this directory. The subdirectory win32 will be created with the 
application pmdefaults.exe. You can run this application in the 
normal way. To move the application, you need to copy *everything* in 
win32. To build setup/pmdefaults-setup.exe, I use Setup Generator from
Gentee software. The configuration file and Setup Generator are not
included in the PortMidi distribution, but there is no magic involved.

---- implementation notes ----

For windows, we use the free software JavaExe.exe. The copy here was
downloaded from 

http://software.techrepublic.com.com/abstract.aspx?kw=javaexe&docid=767485

I found this page by visiting http://software.techrepublic.com.com and
searching in the "Software" category for "JavaExe"

JavaExe works by placing the JavaExe.exe file in the directory with the
Java application jar file and then *renaming* JavaExe.exe to the name
of the jar file, but keeping the .exe extension. (See make.bat for this 
step.) Documentation for JavaExe can be obtained by downloading the
whole program from the URL(s) above.






