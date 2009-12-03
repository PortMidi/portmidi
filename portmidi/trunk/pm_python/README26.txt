PyPortMidi v0.03 03/15/05
Python wrappings for PortMidi
John Harrison
harrison@media.mit.edu

Modified by Roger B. Dannenberg, Nov 2009

PyPortMidi details for Python 2.6
---------------------------------

Installing PyPortMidi from its Pyrex source code:
-------------------------------------------------

1. Linux only: install ASLA if it is not installed:
   http://www.alsa-project.org/

2. Install Pyrex if it is not installed
   http://nz.cosc.canterbury.ac.nz/~greg/python/Pyrex/
   (Tested using Pyrex-0.9.8.5)

3. Choose to rebuild the PortMidi C library...or not:
   compiled binaries of the PortMidi package are included for Win32,
   and OS X, so you might be able to skip this step. If you need
   to rebuild these:
        a. download and extract PortMidi from SourceForge
           http://sourceforge.net/projects/portmedia/files/

        b. Win32: - compile PortMidi with MS VC 2008 Express (free download)
                  - build the project, creating
                    portmidi/Release/portmidi.{lib,dll}

        c. OS X:  - change to portmedia/portmidi directory
                  - compile. Type: xcodebuild -project portmidi.xcodeproj -target libportmidi.dylib -configuration Release

        d. Linux: - follow directions in portmidi/pm_linux/README_LINUX.txt
                  - copy libportmidi.a
                         from portmidi's pm_linux directory
                         to PyPortMidi's linux directory

4. WINDOWS: in PyPortMidi's root directory, type:
   python setup.py install
   (make sure you have admin/superuser privileges)

   MAC OS X: you will need xcode. Even though this README says it is for
       version 2.6, OS X 1.5 has Python 2.5 installed, so I worked with it
       and have not tried installing 2.6 or testing it with PyPortMidi.
   open pm_python/macpypm/macpypm.xcodeproj
   build the pypm.so target. It should build on OS X 10.5 using 
       Python 2.5, but you may need to adjust some directories
   copy (by hand) pm_python/macpypm/build/Release/pypm.so to the
       appropriate python directory, e.g.
       /Volumes/Macintosh HD/Library/Python/2.5/site-packaages/pypm.so
   rename pm_python/pypm.py to pm_python/pypm.py-hidden (so that python
       will not try to load it instead of pypm.so -- pypm.py is for
       Python 3000 and not for Python 2.x)
   open a terminal and cd portmidi/pm_python
   test the installation with: python test.py


Distribution of PyPortMidi compiled code:
--------------------------------------------

John Harrison created a Win32 installer for Python 2.3.x. 
There is no installer for Python 2.6 and beyond. (Follow directions above.)

Using PyPortMidi
----------------
Running the test.py sample script and looking at the test.py code is the
easiest way to start. The classes and functions are mostly documented, or
seem self-explanatory. miniTest.py is another test program.

You can also look at the portmidi.h header, which heavily documents all
of PortMidi's functions.

Overview of Files and Architecture
----------------------------------

Pyrex is used to build an interface from Python 2.6 to the PortMidi
dynamic library. The interface has been changed in several ways, 
mainly providing some default behavior such as using the built-in
PortTime as a time reference.

Pyrex compiles pypm.pyx to pypm.c
pypm.c is compiled to a dynamic library pypm.pyd and installed so
    that Python can load it.
There is no wrapper or intermediate layer of Python code. Everything
is defined in pypm.pyx and implemented via C.

Bugs, suggestions etc.
----------------------
Pm_Channel(channel) in PortMidi numbers channels from 0 to 15. 
Pm_Channel(channel) in PyPortMidi for Python 2.6 numbers channels 
from 1 to 16. This was intended as a "bug fix", but now PyPortMidi 
and PortMidi behave differently. Perhaps PyPortMidi will be changed 
if it will not disrupt too many users and applications.

I welcome any bugs you have to report or any suggestions you have about
how to improve the code and the interface.

-John

CHANGELOG

18-nov-2009 Roger B. Dannenberg
   Update after porting to Python 3.1 and testing on Vista.



