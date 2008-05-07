scorealign -- a program for audio-to-audio and audio-to-midi alignment

Last updated May 7, 2008 by RBD

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
output also includes (1) an estimated transcript in ASCII format with time, 
pitch, MIDI channel, and duration of each notes in the audio file, (2) a
time-aligned midi file, and (3) a text file with beat times.

For Macintosh OS X, use Xcode to open scorealign.xcodeproj
For Linux, use "make -f Makefile.linux"
For Windows, open score-align.vcproj (probably out of date now -- please
    update the project following Makefile.linux, or contact rbd at cs.cmu.edu)

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
   -b is filename to write the time aligned beat times (default is beatmap.txt)
   -o 2.0 indicates a smoothing window of 2.0s
   -p 3.0 means pre-smooth with a 3s window
   -x 6.0 indicates 6s line segment approximation
   
A bit more detail:

The -o flag (smoothing) controls a post-process on the path. Since the
path is discrete, it will have small jumps ahead or pauses whenever it
differs from the diagonal. A linear regression is performed at each frame
using a set of points whose size is determined by the -o parameter, and the
discrete time indicated by the path is replaced by a continuous time estimated
from neighboring points. This smooths out local irregularities in the time
map.

The -p flag (presmoothing) operates on the discrete path. It tries to fit a 
straight line segment (length is set by -p) to the path. If the path fits
well to the first half of the path and the second half of the path, the 
entire path is replaced with a straight line approximation. To "fit well",
half of the path points must fall very close to the straight line (currently,
within 1.5 frames). For example, if the line segment spans 40 frames, then 10
path points must be close to the first 20 frames and 10 path points must be 
close to the last 20 frames. The step is repeated on overlapping windows
through the whole piece. This presmoothing step is designed to detect
places where dynamic programming "wanders off" from the true path and then
realigns to the true path. The off-track points are replaced, so they do not
adversely affect the smoothing step. This approach does not seem to be 
robust, but sometimes works well.

The -x flag is another approach to deal with dynamic programming errors. It
divides the entire piece into segments whose lengths are about equal and about
the length specified by the -x parameter. The line segments are fit to the
path by linear regression, and their endpoints are joined by averaging their
linear regression values. Next, a hill-climbing search is performed to 
minimize the total distance along the path. This is like dynamic programming
except that each line spans many frames, so the resulting path is forced to 
be fairly straight. Linear interpolation is used to estimate chroma distance
since the lines do always pass through integer frame locations. This approach
is probably good when the audio is known to have a steady tempo or be 
performed with tempo changes that match those in the midi file.
