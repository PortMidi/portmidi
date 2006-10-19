README_MAC.txt for PortMidi
Roger Dannenberg
29 Aug 2006

There are two ways to build PortMidi for Mac OSX:

=== METHOD 1 (make) ===

To make PortMidi and PortTime, go back up to the portmidi
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

This code has not been carefully tested; however, 
all test programs in pm_test seem to run properly.

=== METHOD 2 (xcodebuild) ===
From this directory (pm_mac) run this command:

sudo xcodebuild -project pm_mac.xcodeproj -configuration Deployment install DSTROOT=/

[pm_mac.xcodeproj courtesy of Leigh Smith]

CHANGELOG

07-Oct-2006 Roger B. Dannenberg
    Added directions for xcodebuild
29-aug-2006 Roger B. Dannenberg
    Updated this documentation.
 
