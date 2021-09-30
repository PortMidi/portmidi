# PortMidi, PortSMF, scorealign

This is:
 - PortMidi, a cross-platform library for MIDI IO,
 - PortSMF, a Standard MIDI File reader/writer/data structure, 
 - scorealign, music alignment software (audio and/or symbolic), 
all from the orignal designer/developer.
Each of these is independent; if you 
just want PortMidi, you only need the portmidi/trunk subdirectory.

# PortMidi

## [Full C API documentation is here.](https://rbdannenberg.github.io/portmidi/)

## Compiling and Using PortMidi

Use CMake (or ccmake) to create a Makefile for Linux/BSD or a 
project file for Xcode or MS Visual Studio. Use make or an IDE to compile. 
libportmidi_s.* is a static library (recommended - fewer things can go wrong);
libportmidi.* is a dynamic library.

## What's New?

This repo is a copy of the PortMedia project at Sourceforge. 

As of September 2021, PortMidi has some changes:

 - added Pm_CreateVirtualInput() and Pm_CreateVirtualOutput() functions that allow
   applications to create named ports analogous to devices.
   
 - improvements for macOS CoreMIDI include higher data rates for devices, better
   handling of Unicode interface names in addition to virtual device creation.
   
 - the notion of default devices, Pm_GetDefaultInputDeviceID(), 
   Pm_GetDefaultOutputDeviceID and the PmDefaults program have fallen into disuse
   and are now deprecated.
   
 - Native Interfaces for Python, Java, Go, Rust, Lua and more seem best left
   to individual repos, so support within this repo has been dropped. A Java
   interface is still here and probably usable -- let me know if you need it
   and/or would like to help bring it up to date. I am happy to help with, 
   link to, or collaborate in supporting PortMidi for other languages. 
   
For up-to-date PortMidi for languages other than C/C++ found here, check with
developers. As of 27 Sep 2021, this (and SourceForge) is the only repo with
the features described above.

# PortSMF

PortSMF is a library for reading/writing/editing Standard MIDI Files. It is
actually much more, with a general representation of events and updates with
properties consisting of attributes and typed values. Familiar properties of
pitch, time, duration, and channel are built into events and updates to make
them faster to access and more compact.

To my knowledge, PortSMF has the most complete and useful handling of MIDI
tempo tracks. E.g., you can edit notes according to either beat or time, and
you can edit tempo tracks, for example, flattening the tempo while preserving
the beat alignment, preserving the real time while changing the tempo or 
stretching the tempo over some interval.

In addition to Standard MIDI Files, PortSMF supports an ASCII representation
called Allegro. PortSMF and Allegro are used for Audacity Note Tracks.

# scorealign

Another library and applicatioon in this repo is scorealign, which aligns
audio-to-audio, audio-to-MIDI or MIDI-to-MIDI using dynamic time warping (DTW)
of a computed chromagram representation. There are some added smoothing tricks
to improve performance. This library is written in C and runs substantially 
faster than most other implementations, especially those written in MATLAB,
due to the core DTW algorithm. Users should be warned that while chromagrams
are robust features for alignment, they achieve robustness by operating at 
fairly high granularity, e.g., durations of around 100ms, which limits 
time precision. Other more recent algorithms can doubtless do better, but
be cautious of claims, since it all depends on what assumptions you can 
make about the music.
