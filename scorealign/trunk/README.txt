scorealign -- a program for audio-to-audio and audio-to-midi alignment

Contributors include: 
             Ning Hu
             Roger B. Dannenberg
             Joshua Hailpern
             Umpei Kurokawa
             Greg Wakefield
             Mark Bartsch
 
scorealign works by computing chromagrams of the two sources. Midi chromagrams
are estimated directly from pitch data without synthesis. A similarity matrix
is constructed and dynamic programming finds the lowest-cost path through the
matrix.

(some more details should be added here about handling boundaries)

Output includes a map from one version to the other. If one file is MIDI, 
output also includes an estimated transcript in ASCII format with time, 
pitch, MIDI channel, and duration of each notes in the audio file.

For Macintosh OS X, use "make -f Makefile.osx"
For Linux, use "make"
For Windows, open score-align.vcproj

Command line parameters:

scorealign [-<flags> [<period><windowsize><path> <smooth><trans> <midi>]] 
                 <file1> [<file2>]
   specifying only <file1> simply transcribes MIDI in <file1> to  
   transcription.txt. Otherwise, align <file1> and <file2>.
   -h 0.25 indicates a frame period of 0.25 seconds
   -w 0.25 indicates a window size of 0.25 seconds. 
   -r indicates filename to write raw alignment path to (default path.data)
   -s is filename to write smoothed alignment path(default is smooth.data)
   -t is filename to write the time aligned transcription 
      (default is transcription.txt)
   -m is filename to write the time aligned midi file (default is midi.mid)

