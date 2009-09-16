README_MAC.txt for PortMidi
Roger Dannenberg
17 jan 2007

To build PortMidi for Mac OS X:

==== USING MAKE ====

go back up to the portmidi
directory and type 

make -f pm_mac/Makefile.osx

(You can also copy pm_mac/Makefile.osx to Makfile in the 
portmidi directory and just type "make".)

The Makefile.osx will build all test programs and the portmidi
library. You may want to modify the Makefile.osx to remove the
PM_CHECK_ERRORS definition. For experimental software,
especially programs running from the command line, we 
recommend using PM_CHECK_ERRORS -- it will terminate your
program and print a helpful message if any PortMidi 
function returns an error code.

If you do not compile with PM_CHECK_ERRORS, you should 
check for errors yourself.

The make file will also build an OS X Universal (ppc and i386)
dynamic link library using xcode. For instructions about this
and other options, type

make -f pm_mac/Makefile.osx help

==== USING XCODE ====

(1) Open portmidi/portmidi.xcodeproj with Xcode and 
build what you need. The simplest thing is to build the
ALL_BUILD target. The default will be to build the Debug
version, but you may want to change this to Release. 

The Debug version is compiled with PM_CHECK_ERRORS, and the
Release version is not. PM_CHECK_ERRORS will print an error
message and exit your program if any error is returned from
a call into PortMidi.

CMake (currently) also creates MinSizRel and RelWithDebInfo
versions, but only because I cannot figure out how to disable
them.

You will probably want the application PmDefaults, which sets
default MIDI In and Out devices for PortMidi. You may also
want to build a Java application using PortMidi. Since I have
not figured out how to use CMake to make an OS X Java application,
use pm_mac/pm_mac.xcodeproj.

(2) open pm_mac/pm_mac.xcodeproj

(3) For completeness, the JPortMidiHeaders project makes
pm_java/pmjni/portmidi_JportmidiApi.h, a header that is needed
by libpmjni.jnilib, the Java native interface library. Since
portmidi_JportmidiApi.h is included with PortMidi, you can skip
this step.

(4) If you did not build libpmjni.dylib using portmidi.xcodeproj,
do it now. (It depends on portmidi_JportmidiApi.h, and the 
PmDefaults project depends on libpmjni.dylib.

(5) Returning to pm_mac.xcodeproj, build the PmDefaults program.

(6) If you wish, copy pm_mac/build/Deployment/PmDefaults.app to
your applications folder.

CHANGELOG

14-Sep-2009 Roger B. Dannenberg
    Modifications for using CMake
17-Jan-2007 Roger B. Dannenberg
    Explicit instructions for Xcode
15-Jan-2007 Roger B. Dannenberg
    Changed instructions because of changes to Makefile.osx
07-Oct-2006 Roger B. Dannenberg
    Added directions for xcodebuild
29-aug-2006 Roger B. Dannenberg
    Updated this documentation.
 
