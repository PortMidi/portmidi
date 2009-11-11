PyPortMidi v0.03 03/15/05
Python wrappings for PortMidi
John Harrison
harrison@media.mit.edu

Modified by Roger B. Dannenberg, Nov 2009

PyPortMidi
----------

PyPortMidi is a Python wrapper for PortMidi. PortMidi is a cross-platform
C library for realtime MIDI control. Using PyPortMidi, you can send and
receive MIDI data in realtime from Python.

Besides using PyPortMidi to communicate to synthesizers and the
like, it is possible to use PyPortMidi as a way to send MIDI messages
between software packages on the same computer. For example, Using
PyPortMidi and MIDI-YOKE on a Windows machine, it is possible to send
realtime MIDI messages between programs on the same computer using
loopback virtual MIDI ports.

PyPortMidi is cross-platform, but it will require some small
changes in the setup.py file for it to install correctly on Linux
machines. The changes should be pretty straightforward, and I am
anxious to work with a Linux user on the port.

PyPortMidi was written in Pyrex, a language for writing Python extension
modules.

At present, PyPortMidi only supports Python 2.6.

Installing PyPortMidi from its Pyrex source code:
-------------------------------------------------

1. Linux only: install ASLA if it is not installed:
   http://www.alsa-project.org/

2. Install Pyrex if it is not installed
   http://nz.cosc.canterbury.ac.nz/~greg/python/Pyrex/
   (Tested using Pyrex-0.9.8.5)

3. Choose to rebuild the PortMidi C library...or not:
   compiled binaries of the PortMidi package are included for Win32,
    OS X, and Linux, so you can skip this step. However, If you need
    to rebuild these:
        a. download and extract PortMidi from SourceForge
           http://sourceforge.net/projects/portmedia/files/

        b. Win32: - compile PortMidi with MS VC 2008 Express (free download)
                  - build the project, creating
                    portmidi/Release/portmidi.{lib,dll}

        c. OS X:  - change to PortMidi subdirectory pm_mac
                  - compile. Type: xcodebuild -project pm_mac.pbproj
                  - copy newly created libportmidi.a to
                    PyPortMidi's OSX subdirectory

        d. Linux: - type <make> from PortMidi's root directory
                  - copy libportmidi.a
                         from portmidi's pm_linux directory
                         to PyPortMidi's linux directory
                  - copy libporttime.a
                         from portmidi's porttime directory
                         to PyPortMidi's linux directory

4. in PyPortMidi's root directory, type:
   python setup.py install
   (make sure you have admin/superuser privileges)

Distribution of PyPortMidi compiled code:
--------------------------------------------

John Harrison created a Win32 installer for Python 2.3.x. 
There is no installer for Python 2.6 and beyond.

Using PyPortMidi
----------------
Running the test.py sample script and looking at the test.py code is the
easiest way to start. The classes and functions are mostly documented, or
seem self-explanatory. miniTest.py is another test program.

You can also look at the portmidi.h header, which heavily documents all
of PortMidi's functions.

Bugs, suggestions etc.
----------------------
Pm_Channel(channel) in PortMidi numbers channels from 0 to 15. 
Pm_Channel(channel) in PyPortMidi numbers channels from 1 to 16.
This was intended as a "bug fix", but now PyPortMidi and PortMidi
behave differently. Perhaps PyPortMidi will be changed if it will
not disrupt too many users and applications.

I welcome any bugs you have to report or any suggestions you have about
how to improve the code and the interface.

-John



