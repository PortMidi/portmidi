#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <snd.h>
#ifndef __MACH__
    #include <malloc.h>
#endif

#include "allegro.h"
#include "scorealign.h"
#include "gen_chroma.h"
#include "comp_chroma.h"
#include "allegro.h"
#include "mfmidi.h"


#if (defined (WIN32) || defined (_WIN32))
#define	snprintf	_snprintf
#endif

#define	LOW_CUTOFF  40
#define HIGH_CUTOFF 2000

// these are global parameters for the alignment

/*===========================================================================*/
float frame_period; // nominal time in seconds
float window_size;  //window size in seconds
float smooth_time = 1.75; // duration of smoothing window
int smooth; // number of points used to compute the smooth time map

//chromagrams and lengths, path data
float *chrom_energy1;
int file1_frames; //number of frames in file1
float *chrom_energy2;
int file2_frames; //number of frames in file2
short *pathx;  //for midi (when aligning midi and audio)
short *pathy; //for audio (when aligning midi and audio)
int pathlen = 0;
int path_count = 0; // for debug log formatting
float *time_map;
float *smooth_time_map;

// chroma vectors are calculated from an integer number of samples
// that approximates the nominal frame_period. Actual frame period
// is calculated and stored here:
float actual_frame_period_1; // time in seconds for midi (when aligning midi and audio)
float actual_frame_period_2; // time in seconds for audio (when aligning midi and audio)

// path is file1_frames by file2_frames array, so first index
// (rows) is in [0 .. file1_frames]. Array is sequence of rows.
// columns (j) ranges from [0 .. file2_frames]
#define PATH(i,j) (path[(i) * file2_frames + (j)])

/*===========================================================================*/

#if DEBUG_LOG
FILE *dbf = NULL;
#endif

static void print_usage(char *progname) 
{
    printf("\nUsage: %s [-<flags> [<period> <windowsize> <path> <smooth> <trans> <midi>]] <file1> [<file2>]\n", progname);
    printf("   specifying only <file1> simply transcribes MIDI in <file1> to\n");
    printf("   transcription.txt. Otherwise, align <file1> and <file2>.\n");
    printf("   -h 0.25 indicates a frame period of 0.25 seconds\n");
    printf("   -w 0.25 indicates a window size of 0.25 seconds. \n");
    printf("   -r indicates filename to write raw alignment path to (default path.data)\n");
    printf("   -s is filename to write smoothed alignment path(default is smooth.data)\n");
    printf("   -t is filename to write the time aligned transcription (default is transcription.txt)\n");
    printf("   -m is filename to write the time aligned midi file (default is midi.mid)\n");
    printf("   -o 2.0 indicates a smoothing window time of 2.0s\n");
#if (defined (_WIN32) || defined (WIN32))
    printf("   This is a Unix style command line application which\n"
           "   should be run in a MSDOS box or Command Shell window.\n\n");
    printf("   Type RETURN to exit.\n") ;
    getchar();
#endif
} /* print_usage */


/*			MAP_TIME  
	lookup time of file1 in smooth_time_map and interpolate
	to get time in file2 
*/

float map_time(float t1)
{
	t1 /= actual_frame_period_1; // convert from seconds to frames
	int i = (int) t1; // round down
	if (i < 0) i = 0;


	if (i >= file1_frames - 1) i = file1_frames - 2;

	// interpolate to get time
	return actual_frame_period_2 * 
		   (smooth_time_map[i] + (t1 - i) * 
		                         (smooth_time_map[i + 1] - smooth_time_map[i]));


}
/*				PRINT_BEAT_MAP
   prints the allegro beat_map (for debugging) which contain
   the time, beat pair for a song 
*/


void print_beat_map(Alg_seq_ptr seq, char *filename) {

	FILE *beatmap_print = fopen(filename, "w"); 

	Alg_beats &b = seq->get_time_map()->beats;
	long num_beats = seq->get_time_map()->length();

	for(int i=0 ; i < num_beats; i++) { 
			fprintf(beatmap_print," %f  %f \n", b[i].beat, b[i].time); 
	}	
	fclose(beatmap_print); 

}

/*				FIND_MIDI_DURATION 
	Finds the duration of a midi song where the end
	is defined by where the last note off occurs. Duration
	in seconds is given in DUR, and returns in int the number
	of notes in the song
*/

int find_midi_duration(Alg_seq_ptr seq, float *dur) {
	*dur = 0.0F;
	int nnotes = 0;
	int i, j;
	seq->convert_to_seconds();
    for (j = 0; j < seq->track_list.length(); j++) {
		Alg_events &notes = (seq->track_list[j]);
		
		for (i = 0; i < notes.length(); i++) {
			Alg_event_ptr e = notes[i];
			if (e->is_note()) {
				Alg_note_ptr n = (Alg_note_ptr) e;
				float note_end = n->time + n->dur;
				if (note_end > *dur) *dur = note_end;
				nnotes++;
			}

		}
	}
	
	return nnotes; 
}



/*			MIDI_TEMPO_ALIGN 
	Creates the time aligned midi file from SEQ and writes
	it to MIDINAME
*/

void midi_tempo_align(Alg_seq_ptr seq , char *midiname) {

    //We create a new time map out of the alignment, and replace it with
    //the original time map in the song
    Alg_seq new_time_map_seq;
	
    /** align at all integer beats **/
    int totalbeats; 
    float dur_in_sec; 
    find_midi_duration(seq, &dur_in_sec); 
    totalbeats= seq->get_time_map()->time_to_beat(dur_in_sec) +2; //totalbeat= lastbeat +1 and round up the beat
    printf("midi duration = %f, totalbeats=%i \n", dur_in_sec, totalbeats); 	
    
    float *newtime_array =(float *) malloc(sizeof(float) * totalbeats);

    for(int i=0; i<totalbeats; i++) {

        newtime_array[i]= map_time(seq->get_time_map()->beat_to_time(i));
        if (newtime_array[i]> 0) 
            new_time_map_seq.insert_beat((double)newtime_array[i], (double)i);
    }

    free(newtime_array);
	
    seq->set_time_map(new_time_map_seq.get_time_map()); 
    print_beat_map(seq, "beatmapnew.txt"); 
    FILE *tempo_aligned_midi = fopen(midiname, "wb");
	
    seq->smf_write(tempo_aligned_midi);
    fclose(tempo_aligned_midi);
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


/* Returns the minimum of three values */
double min3(double x, double y, double z)
{
    return (x < y ?
            (x < z ? x : z) :
            (y < z ? y : z));
}


void save_frames(char *name, int frames, float **chrom_energy)
{
    FILE *outf = fopen(name, "w");
    int i,j;
    for (j=0; j < frames; j++) {
        float *chrom_energy_frame = chrom_energy[j];
        for (i = 0;  i <= CHROMA_BIN_COUNT; i++) {
            fprintf(outf, "%g ", chrom_energy_frame[i]);
        }
        fprintf(outf, "\n");
    }
    fclose(outf);
}


/* steps through the dynamic programming path
*/
void path_step(int i, int j)
{
#if DEBUG_LOG
    fprintf(dbf, "(%i,%i) ", i, j);
	if (++path_count % 5 == 0 ||
		(i == 0 && j == 0)) 
		fprintf(dbf, "\n");
#endif
    pathx[pathlen] = i; 
    pathy[pathlen] = j;
    pathlen++;
}        


/* path_reverse -- path is computed from last to first, flip it */
/**/
void path_reverse()
{
	int i = 0;
	int j = pathlen - 1;
	while (i < j) {
	    short tempx = pathx[i]; short tempy = pathy[i];
	    pathx[i] = pathx[j]; pathy[i] = pathy[j];
	    pathx[j] = tempx; pathy[j] = tempy;
		i++; j--;
	}
}

/*
Sees if the chroma energy vector is silent (indicated by the 12th element being one)
Returns true if it is silent.  False if it is not silent 
*/
bool silent( int i, float *chrom_energy)
{
if (AREF2(chrom_energy, i,CHROMA_BIN_COUNT) == 1.0F)
	return true;
else 
	return false; 

}

/*
returns the first index in pathy where the element is bigger than sec 
*/
int sec_to_pathy_index(float sec) {

    for (int i=0 ; i<(file1_frames+file2_frames); i++) {
        if (smooth_time_map[i]*actual_frame_period_2>=sec) {
            return i; 
        }
        //rintf("%i\n" ,pathy[i]);
    }
    return -1; 
}


/*	
given a chrom_energy vector, sees how many 
of the inital frames are designated as silent 
*/

int frames_of_init_silence( float *chrom_energy, int frame_count)
{
	bool silence = true;
	int frames=0; 
	while(silence) {
		
		if ( silent(frames, chrom_energy)) 
			frames++; 
		else
			silence=false; 
		
	}

	return frames; 
}


/*		COMPARE_CHROMA
Perform Dynamic Programming to find optimal alignment
*/
void compare_chroma() 
{
    float *path;
    int x = 0;
    int y = 0;
	int start_frame_x=0;
	int start_frame_y=0;
    
    /* Allocate the distance matrix */
    path = (float *) calloc(file1_frames * file2_frames, sizeof(float));
    
    /* Initialize first row and column */

	/* allow free skip over initial silence in either signal, but not both */
	/* silence is indicated by a run of zeros along the first row and or 
	 * column, starting at the origin (0,0). After computing these runs, we
	 * put the proper value at (0,0)
	 */
	printf("Performing silent skip DP \n"); 
	PATH(0, 0) = (silent(0, chrom_energy1) ? 0 :
		          gen_dist(0, 0, chrom_energy1, chrom_energy2));
    for (int i = 1; i < file1_frames; i++)
		PATH(i, 0) = (PATH(i-1, 0) == 0 && silent(i, chrom_energy1) ? 0 :
                      gen_dist(i, 0, chrom_energy1, chrom_energy2) + 
					  PATH(i-1, 0));
	PATH(0, 0) = (silent(0, chrom_energy2) ? 0 :
		          gen_dist(0, 0, chrom_energy1, chrom_energy2));
    for (int j = 1; j < file2_frames; j++)
		PATH(0, j) = (PATH(0, j-1) == 0 && silent(j, chrom_energy2) ? 0 :
		              gen_dist(0, j, chrom_energy1, chrom_energy2) + 
                      PATH(0, j-1));
	/* first row and first column are done, put proper value at (0,0) */
	PATH(0, 0) = (!silent(0, chrom_energy1) || !silent(0, chrom_energy2) ?
		          gen_dist(0, 0, chrom_energy1, chrom_energy2) : 0);

    /* Perform DP for the rest of the matrix */
    for (int i = 1; i < file1_frames; i++)
        for (int j = 1; j < file2_frames; j++)
            PATH(i, j) = gen_dist(i, j, chrom_energy1, chrom_energy2) +
                         min3(PATH(i-1, j-1), PATH(i-1, j), PATH(i, j-1)); 
	   
    printf("Completed Dynamic Programming.\n");
	

    x = file1_frames - 1;
    y = file2_frames - 1;

	//x andy are the ending points, it can end at either the end of midi, or end of audio
	//but not both
    pathx = (short *) malloc(sizeof(short) * (x + y + 2));
    pathy = (short *) malloc(sizeof(short) * (x + y + 2));
	
	assert(pathx!=NULL);
	assert(pathy!=NULL);
	 
	// map from file1 time to file2 time
	time_map = (float *) malloc(sizeof(float) * file1_frames);
	smooth_time_map = (float *) malloc(sizeof(float) * file1_frames);
	
#if DEBUG_LOG
    fprintf(dbf, "\nOptimal Path: ");
	int point_count = 0;
#endif
    while (1) {
        /* Check for stopping */
	


		if (x ==  0 & y == 0) {

			path_step(0, 0);
			path_reverse();
            break;
        }
		
        /* Print the current coordinate in the path*/
        path_step(x, y);

        /* Check for the optimal path backwards*/
        if (x > 0 && y > 0 && PATH(x-1, y-1) <= PATH(x-1, y) &&
            PATH(x-1, y-1) <= PATH(x, y-1)) {
            x--;
            y--;
        } else if (x > 0 && y > 0 && PATH(x-1, y) <= PATH(x, y-1)) {
            x--;
        } else if (y > 0) {
            y--;
        } else if (x > 0) {
            x--;
        }
    }	
}
/*		SAVE_PATH
	write the alignment path to FILENAME
*/

void save_path(char *filename)
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




void linear_regression(int n, int width, float &a, float &b)
{
	int hw = (width - 1) / 2; // a more convenient form: 1/2 width
	// compute average of x = avg of time_map[i]
	float xsum = 0;
	float ysum = 0;
	float xavg, yavg;
	int i;
	for (i = n - hw; i <= n + hw; i++) {
		xsum += i;
		ysum += time_map[i];
	}
	xavg = xsum / width;
	yavg = ysum / width;
	float num = 0;
	float den = 0;
	for (i = n - hw; i <= n + hw; i++) {
		num += (i - xavg) * (time_map[i] - yavg);
		den += (i - xavg) * (i - xavg);
	}
	b = num / den;
	a = yavg - b * xavg;
}





/*			COMPUTE_SMOOTH_TIME_MAP 
	 compute regression line and estimate point at i
 
	 Number of points in regression is smooth (an odd number). First
	 index to compute is (smooth-1)/2. Use that line for the first
	 (smooth+1)/2 points. The last index to compute is 
	 (file1_frames - (smooth+1)/2). Use that line for the last 
	 (smooth+1)/2 points.
*/
void compute_smooth_time_map()
{
	// do the first points:
	float a, b;
	linear_regression((smooth - 1) / 2, smooth, a, b);
	int i;
	for (i = 0; i < (smooth + 1) / 2; i++) {
		smooth_time_map[i] = a + b*i;
	}

	// do the middle points:
	for (i = (smooth + 1) / 2; i < file1_frames - (smooth + 1) / 2; i++) {
		linear_regression(i, smooth, a, b);
		smooth_time_map[i] = a + b*i;

#if DEBUG_LOG
		fprintf(dbf, "time_map[%d] = %g, smooth_time_map[%d] = %g\n", 
			    i, time_map[i], i, a + b*i);
#endif

	}

	// do the last points
	linear_regression(file1_frames - (smooth + 1) / 2, smooth, a, b);
	for (i = file1_frames - (smooth + 1) / 2; i < file1_frames; i++) {
		smooth_time_map[i] = a + b*i;
	}


}

/*				COMPUTE_REGRESSION_LINES
	computes the smooth time map from the path computed
	by dynamic programming

*/
void compute_regression_lines()
{
	// first, compute the y value of the path at
	// each x value. If the path has multiple values
	// on x, take the average.
	int p = 0;
	int i;
	int upper, lower;
	for (i = 0; i < file1_frames; i++) {
	    lower = pathy[p];
		while (p < pathlen && pathx[p] == i) {
			upper = pathy[p];
			p = p + 1;
		}
		time_map[i] = ((float) lower + (float) upper) * 0.5;
	}
	// now fit a line to the nearest WINDOW points and record the 
	// line's y value for each x.
	compute_smooth_time_map();

}

/*				SAVE_SMOOTH_FILE 
	saves the smooth time map in SMOOTH_FILENAME

*/
void save_smooth_file(char *smooth_filename) {
    FILE *smoothf = fopen(smooth_filename, "w");
    assert(smoothf);
    for (int i = 0; i < file1_frames; i++) {
        fprintf(smoothf, "%g \t%g\n", i * actual_frame_period_1,
                smooth_time_map[i] * actual_frame_period_2);
    }
    fclose(smoothf);
}



/*				EDIT_TRANSCRIPTION
	edit the allegro time map structure according
	to the warping and output a midi file and transcription
	file 

*/
void edit_transcription(Alg_seq_ptr seq , bool warp, FILE *outf, 
                        char *midi_filename) {
    int note_x = 1;	
    seq->iteration_begin();

    Alg_event_ptr e = seq->iteration_next();

    while (e) {
        if (e->is_note()) {
            Alg_note_ptr n = (Alg_note_ptr) e;
            fprintf(outf, "%d %d %d %d ", 
                    note_x++, n->chan, n->pitch, (int) n->loud);
            // now compute onset time mapped to audio time
            double start = n->time;
            double finish = n->time + n->dur;
            if (warp) {
                start = map_time(start);
                finish = map_time(finish);
            }
            fprintf(outf, "%.3f %.3f\n", start, finish-start);
        }
        e = seq->iteration_next();
    }
    seq->iteration_end();
    fclose(outf);
    if (warp) {
        // align the midi file and write out 	
        midi_tempo_align(seq, midi_filename); 
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
                        char *midi_filename)
{
	
	char *midiname; //midi file to be read
	char *audioname; //audio file to be read
	
	if (warp) save_smooth_file(smooth_filename); 

	//If either is a midifile
	if (is_midi_file(file1) || is_midi_file(file2)) {
	
		if (is_midi_file(file1)) {
			midiname=file1;
			audioname=file2;
		}
		else{
		
			midiname=file2;
			audioname=file1;
		}	
		

		FILE *inf = fopen(midiname, "rb");
		
		if (!inf) {
			printf("internal error: could not reopen %s\n", midiname);
			return;
		}
		
		Alg_seq_ptr seq = alg_smf_read(inf, NULL);
		
		fclose(inf);
		
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
		fprintf(outf, "# transcription format : <sequence number> <channel> <pitch> <velocity> <onset> <duration>\n");
		
	
		edit_transcription(seq, warp, outf, midi_filename); 
		delete(seq); 
		
	}

}



int main(int argc, char *argv []) 
{	
    char *progname, *infilename1, *infilename2;
    char *smooth_filename, *path_filename, *trans_filename, *midi_filename;
    float Audio_Offset = 0; //this is the audio offset output
    
    //just transcribe if trasncribe == 1
    int transcribe = 0;
	
    // Default for the user definable parameters
    
    frame_period = 0.25;
    window_size = .25; 
    path_filename = "path.data";
    smooth_filename = "smooth.data";
    trans_filename = "transcription.txt";
    midi_filename = "midi.mid";
	
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
            if (argv[i][1] == 'h') {
                frame_period = atof(argv[i+1]);	
            } else if (argv[i][1] == 'w') {
                window_size = atof(argv[i+1]); 
            } else if (argv[i][1] == 'r') {
                path_filename=argv[i+1];
            } else if (argv[i][1] == 's') {
                smooth_filename=argv[i+1];
            } else if (argv[i][1] == 't') {
                trans_filename=argv[i+1]; 
            } else if (argv[i][1] == 'm') {
                midi_filename=argv[i+1];
            } else if (argv[i][1] == 'o') {
                smooth_time = atof(argv[i+1]);
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

#if DEBUG_LOG
    dbf = fopen("debug-log.txt", "w");
    assert(dbf);
#endif

    if (transcribe == 1) {
	// if only one midi file, just write transcription and exit, 
        // no alignment
        save_transcription(infilename1, "", false, trans_filename,NULL, NULL);
        printf("Wrote %s\n", trans_filename);
        goto finish;
    }


    // if midi is infilename2, make it infilename1
    if (is_midi_file(infilename2) & !is_midi_file(infilename1)) {
        char *temp; 
        temp = infilename1; 
        infilename1 = infilename2;
        infilename2 = temp; 
    }

     /* Generate the chroma for file 1 
      * This will always be the MIDI File when aligning midi with audio.
      */
    printf ("==============FILE 1====================\n");
    file1_frames = gen_chroma(infilename1, HIGH_CUTOFF, LOW_CUTOFF, 
                              &chrom_energy1, &actual_frame_period_1);
    if (file1_frames == -1) { // error opening file
        printf ("Error : Not able to open input file %s\n", infilename1);
        return 1;
    }
    printf("\nGenerated Chroma. file1_frames is %i\n", file1_frames);
    /* Generate the chroma for file 2 
     * This will always be the Audio File when aligning midi with audio. 
     */
    printf ("==============FILE 2====================\n");
    file2_frames = gen_chroma(infilename2, HIGH_CUTOFF, LOW_CUTOFF, 
                              &chrom_energy2, &actual_frame_period_2);
    if (file2_frames == -1) { // error opening file
        printf ("Error : Not able to open input file %s\n", infilename2);
        return 1;
    }
    printf("\nGenerated Chroma.\n");
    /* now that we have actual_frame_period_2, we can compute smooth */
    // smooth is an odd number of frames that spans about smooth_time
    smooth = int(0.5 + smooth_time / actual_frame_period_2);
    if (smooth < 3) smooth = 3;
    if (!(smooth & 1)) smooth++; // must be odd

    printf("smoothing time is %g\n", smooth_time);
    printf("smooth count is %d\n", smooth);

    /* Normalize the chroma frames */
    norm_chroma(file1_frames, chrom_energy1);
    // print_chroma_table(chrom_energy1, file1_frames);
    norm_chroma(file2_frames, chrom_energy2);
    // print_chroma_table(chrom_energy2, file2_frames);
    printf("Normalized Chroma.\n");

    /* Compare the chroma frames */
    compare_chroma();
    /* Compute the smooth time map*/	
    compute_regression_lines();
    // save path
    save_path(path_filename);	
    // save smooth, midi, transcription
    save_transcription(infilename1, infilename2, true, trans_filename, 
                       smooth_filename,midi_filename);

    // print what the chroma matrix looks like
    /*
      printf("file1 chroma table: \n"); 
      print_chroma_table(chrom_energy1,file1_frames);
      printf("\nfile2 chroma table: \n"); 
      print_chroma_table(chrom_energy2, file2_frames); 
    */
	
    free(pathx);
    free(pathy);		
    // only path and smooth are written when aligning two audio files
    if (is_midi_file(infilename1) || is_midi_file(infilename2))
        printf("Wrote %s %s and %s.", path_filename, smooth_filename, 
               trans_filename);
    else
        printf("Wrote %s and %s.",path_filename, smooth_filename); 
finish:
#if DEBUG_LOG
    fclose(dbf);
#endif

    return 0 ;
} /* main */

