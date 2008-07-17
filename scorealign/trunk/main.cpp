/* main.cpp -- the command line interface for scorealign
 * 
 * 14-Jul-08 RBD
 */

#include "stdio.h"
#include "main.h"
#include <fstream>
#include "allegro.h"
#include "audioreader.h"
#include "scorealign.h"
#include "sautils.h"
#include "alignfiles.h"
#include "gen_chroma.h"

// a global object with score alignment parameters and data
Scorealign sa;

static void print_usage(char *progname) 
{
    printf("\nUsage: %s [-<flags> [<period> <windowsize> <path> <smooth> "
           "<trans> <midi>]] <file1> [<file2>]\n", progname);
    printf("   specifying only <file1> simply transcribes MIDI in <file1> "
           "to\n");
    printf("   transcription.txt. Otherwise, align <file1> and <file2>.\n");
    printf("   -h 0.25 indicates a frame period of 0.25 seconds\n");
    printf("   -w 0.25 indicates a window size of 0.25 seconds. \n");
    printf("   -r indicates filename to write raw alignment path to "
           "(default path.data)\n");
    printf("   -s is filename to write smoothed alignment path(default is "
           "smooth.data)\n");
    printf("   -t is filename to write the time aligned transcription "
           "(default is transcription.txt)\n");
    printf("   -m is filename to write the time aligned midi file "
           "(default is midi.mid)\n");
    printf("   -b is filename to write the time aligned beat times "
           "(default is beatmap.txt)\n");
    printf("   -o 2.0 indicates a smoothing window time of 2.0s\n");
    printf("   -p 3.0 indicates presmoothing with a 3s window\n");
    printf("   -x 6.0 indicates 6s line segment approximation\n");
#if (defined (_WIN32) || defined (WIN32))
    printf("   This is a Unix style command line application which\n"
           "   should be run in a MSDOS box or Command Shell window.\n\n");
    printf("   Type RETURN to exit.\n") ;
    getchar();
#endif
} /* print_usage */


/*				SAVE_SMOOTH_FILE 
	saves the smooth time map in SMOOTH_FILENAME

*/
void save_smooth_file(char *smooth_filename, Scorealign &sa) {
    FILE *smoothf = fopen(smooth_filename, "w");
    assert(smoothf);
    for (int i = 0; i < sa.file1_frames; i++) {
        fprintf(smoothf, "%g \t%g\n", i * sa.actual_frame_period_1,
                sa.smooth_time_map[i] * sa.actual_frame_period_2);
    }
    fclose(smoothf);
}


/*				PRINT_BEAT_MAP
   prints the allegro beat_map (for debugging) which contain
   the time, beat pair for a song 
*/
void print_beat_map(Alg_seq &seq, char *filename) {
    
    FILE *beatmap_print = fopen(filename, "w"); 
    
    Alg_beats &b = seq.get_time_map()->beats;
    long num_beats = seq.get_time_map()->length();
    
    for(int i=0 ; i < num_beats; i++) { 
        fprintf(beatmap_print," %f  %f \n", b[i].beat, b[i].time); 
    }	
    fclose(beatmap_print); 
    
}


/*			MIDI_TEMPO_ALIGN 
	Creates the time aligned midi file from SEQ and writes
	it to MIDINAME
*/

void midi_tempo_align(Alg_seq &seq, char *midiname, 
                      char *beatname, Scorealign &sa)
{
    //We create a new time map out of the alignment, and replace it with
    //the original time map in the song
    Alg_seq new_time_map_seq;
	
    /** align at all integer beats **/
    int totalbeats; 
    float dur_in_sec; 
    find_midi_duration(seq, &dur_in_sec); 
    // totalbeat = lastbeat + 1 and round up the beat
    totalbeats = (int) (seq.get_time_map()->time_to_beat(dur_in_sec) + 2);
    printf("midi duration = %f, totalbeats=%i \n", dur_in_sec, totalbeats); 	
    
    float *newtime_array = ALLOC(float, totalbeats);
    
    for (int i = 0; i < totalbeats; i++) {
        newtime_array[i] = sa.map_time(seq.get_time_map()->beat_to_time(i));
        if (newtime_array[i] > 0) 
            new_time_map_seq.insert_beat((double)newtime_array[i], (double)i);
    }

    free(newtime_array);
	
    seq.set_time_map(new_time_map_seq.get_time_map()); 
    print_beat_map(seq, beatname); 
    seq.smf_write(midiname);
}


/*				EDIT_TRANSCRIPTION
	edit the allegro time map structure according
	to the warping and output a midi file and transcription
	file 

*/
void edit_transcription(Alg_seq &seq , bool warp, FILE *outf, 
                        char *midi_filename, char *beat_filename) {
    int note_x = 1;
    seq.convert_to_seconds();
    seq.iteration_begin();

    Alg_event_ptr e = seq.iteration_next();

    while (e) {
        if (e->is_note()) {
            Alg_note_ptr n = (Alg_note_ptr) e;
            fprintf(outf, "%d %d %d %d ", 
                    note_x++, n->chan, ROUND(n->pitch), ROUND(n->loud));
            // now compute onset time mapped to audio time
            double start = n->time;
            double finish = n->time + n->dur;
            if (warp) {
                start = sa.map_time(start);
                finish = sa.map_time(finish);
            }
            fprintf(outf, "%.3f %.3f\n", start, finish-start);
        }
        e = seq.iteration_next();
    }
    seq.iteration_end();
    fclose(outf);
    if (warp) {
        // align the midi file and write out 	
        midi_tempo_align(seq, midi_filename, beat_filename, sa); 
    }
}


/*		SAVE_TRANSCRIPTION
write note data corresponding to audio file

assume audio file is file 1 and midi file is file 2
so pathx is index into audio, pathy is index into MIDI

If warp is false, simply write a transcription of the midi file.

Every note has 6 fields separated by a space character. The fields are:
<sequence number> <channel> <pitch> <velocity> <onset> <duration> 
Where
   <sequence number> is just an integer note number, e.g. 1, 2, 3, ...
   <channel> is MIDI channel from 0 to 15 
   <pitch> is MIDI key number (60 = middle C)
   <velocity> is MIDI key velocity (1 to 127)
   <onset> is time in seconds, rounded to 3 decimal places (milliseconds)
   <duration> is time in seconds, rounded to 3 decimal places

*/


void save_transcription(char *file1, char *file2, 
                        bool warp, char *filename, char *smooth_filename, 
                        char *midi_filename, char *beat_filename)
{
    
    char *midiname; //midi file to be read
    char *audioname; //audio file to be read
    
    if (warp) save_smooth_file(smooth_filename, sa); 
    
    //If either is a midifile
    if (is_midi_file(file1) || is_midi_file(file2)) {
	
        if (is_midi_file(file1)) {
            midiname=file1;
            audioname=file2;
        } else {
            midiname=file2;
            audioname=file1;
        }
	
        Alg_seq seq(midiname, true);
	
        FILE *outf = fopen(filename, "w");
        if (!outf) {
            printf("Error: could not open %s\n", filename);
            return;
        }
        fprintf(outf, "# transcription of %s\n", midiname);
        if (warp) {
            fprintf(outf, "# note times are aligned to %s\n", audioname);
        } else {
            fprintf(outf, "# times are unmodified from those in MIDI file\n");
        }
        fprintf(outf, "# transcription format : <sequence number> "
                "<channel> <pitch> <velocity> <onset> <duration>\n");
        
        edit_transcription(seq, warp, outf, midi_filename, beat_filename); 
    }
}


/*		SAVE_PATH
	write the alignment path to FILENAME
*/
void save_path(char *filename, int pathlen, short* pathx, short *pathy,
               float actual_frame_period_1, float actual_frame_period_2)
{
    // print the path to a (plot) file
    FILE *pathf = fopen(filename, "w");
    assert(pathf);
    int p;
    for (p = 0; p < pathlen; p++) {
        fprintf(pathf, "%g %g\n", pathx[p] * actual_frame_period_1, 
                pathy[p] * actual_frame_period_2);
    }
    fclose(pathf);
}


/*			
	Prints the chroma table (for debugging)
*/

void print_chroma_table(float *chrom_energy, int frames)
{
    int i, j;
    for (j = 0; j < frames; j++) {
        for (i = 0; i <= CHROMA_BIN_COUNT; i++) {
            printf("%g| ", AREF2(chrom_energy, j, i));
        }
        printf("\n");
    }
}


int main(int argc, char *argv []) 
{	
    char *progname, *infilename1, *infilename2;
    char *smooth_filename, *path_filename, *trans_filename;
    char *midi_filename, *beat_filename;
    
    //just transcribe if trasncribe == 1
    int transcribe = 0;
	
    // Default for the user definable parameters
    
    path_filename = "path.data";
    smooth_filename = "smooth.data";
    trans_filename = "transcription.txt";
    midi_filename = "midi.mid";
    beat_filename = "beatmap.txt";
	
    progname = strrchr(argv [0], '/'); 
    progname = progname ? progname + 1 : argv[0] ;

    // If no arguments, return usage 
    if (argc < 2) {
        print_usage(progname);
        return 1;
    }

	

    /*******PARSING CODE BEGINS*********/
    int i = 1; 
    while (i < argc) {
        //expected flagged argument
        if (argv[i][0] == '-') {
            char flag = argv[i][1];
            if (flag == 'h') {
                sa.frame_period = atof(argv[i+1]);	
            } else if (flag == 'w') {
                sa.window_size = atof(argv[i+1]); 
            } else if (flag == 'r') {
                path_filename=argv[i+1];
            } else if (flag == 's') {
                smooth_filename=argv[i+1];
            } else if (flag == 't') {
                trans_filename=argv[i+1]; 
            } else if (flag == 'm') {
                midi_filename=argv[i+1];
            } else if (flag == 'b') {
                beat_filename = argv[i+1];
            } else if (flag == 'o') {
                sa.smooth_time = atof(argv[i+1]);
            } else if (flag == 'p') {
                sa.presmooth_time = atof(argv[i+1]);
            } else if (flag == 'x') {
                sa.line_time = atof(argv[i+1]);
            }
            i++;
        }
        // When aligning audio to midi we must force file1 to be midi 
        else {			
            // file 1 is midi
            if (transcribe == 0) {
                infilename1 = argv[i];
                transcribe++;
            }
            // file 2 is audio or a second midi 
            else {
                infilename2 = argv[i];
                transcribe++;
            }	
        }
        i++;
    }
    /**********END PARSING ***********/
    if (sa.presmooth_time > 0 && sa.line_time > 0) {
        printf("WARNING: both -p and -x options selected.\n");
    }
#if DEBUG_LOG
    dbf = fopen("debug-log.txt", "w");
    assert(dbf);
#endif

    if (transcribe == 1) {
	// if only one midi file, just write transcription and exit, 
        // no alignment
        save_transcription(infilename1, "", false, trans_filename,NULL, NULL, NULL);
        printf("Wrote %s\n", trans_filename);
        goto finish;
    }


    // if midi only in infilename2, make it infilename1
    if (is_midi_file(infilename2) && !is_midi_file(infilename1)) {
        char *temp; 
        temp = infilename1; 
        infilename1 = infilename2;
        infilename2 = temp;
    }

    if (!align_files(infilename1, infilename2, sa, true /* verbose */)) {
        printf("An error occurred, not saving path and transcription data\n");
        goto finish;
    }
    if (sa.file1_frames <= 2 || sa.file2_frames <= 2) {
        printf("Error: file frame counts are low: %d (for input 1) and %d "
               "for input 2)\n...not saving path and transcription data\n",
               sa.file1_frames, sa.file2_frames);
    goto finish;
    }
    // save path
    save_path(path_filename, sa.pathlen, sa.pathx, sa.pathy, 
              sa.actual_frame_period_1, sa.actual_frame_period_2);
    // save smooth, midi, transcription
    save_transcription(infilename1, infilename2, true, trans_filename, 
                       smooth_filename, midi_filename, beat_filename);

    // print what the chroma matrix looks like
    /*
      printf("file1 chroma table: \n"); 
      print_chroma_table(chrom_energy1,file1_frames);
      printf("\nfile2 chroma table: \n"); 
      print_chroma_table(chrom_energy2, file2_frames); 
    */
	
    // only path and smooth are written when aligning two audio files
    if (is_midi_file(infilename1) || is_midi_file(infilename2))
        printf("Wrote %s, %s, %s, and %s.", path_filename, smooth_filename, 
               trans_filename, beat_filename);
    else
        printf("Wrote %s and %s.", path_filename, smooth_filename); 
    
finish:
#if DEBUG_LOG
    fclose(dbf);
#endif

    return 0 ;
} /* main */


/* print_path_range -- debugging output */
/**/
void print_path_range(short *pathx, short *pathy, int i, int j)
{
    while (i <= j) {
        printf("%d %d\n", pathx[i], pathy[i]);
        i++;
    }
}


