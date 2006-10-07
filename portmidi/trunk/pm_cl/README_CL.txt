README_CL.txt for PortMidi
Roger Dannenberg
07 Oct 2006

This is a Common Lisp interface to PortMidi.

On Mac OSX, you need to build PortMidi as a dynamic link library
before you can use PortMidi from Common Lisp.

You can build PortMidi as a dynamic link library by running this:

cd portmidi
cd pm_mac
sudo xcodebuild -project pm_mac.pbproj install DSTROOT=/

(Thanks to Leigh Smith and Rick Taube)
