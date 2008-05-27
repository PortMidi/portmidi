
#ifdef _WIN32
    #include "malloc.h"
#endif
#include "stdlib.h" // for OSX compatibility, malloc.h -> stdlib.h
#include "stdio.h"
#include "assert.h"
#include "string.h"
#include "math.h"
#include "snd.h"

#include "allegro.h"
#include "scorealign.h"
#include "fft3/FFT.h"
#include "gen_chroma.h"
#include "comp_chroma.h"
#include "mfmidi.h"
#include "sautils.h"

long end_offset_store;

//if 1, causes printing internally
#define PRINT_BIN_ENERGY 1

#define p1 0.0577622650466621
#define p2 2.1011784386926213

// each row is one chroma vector, 
// data is stored as an array of chroma vectors:
// vector 1, vector 2, ...
#define CHROM(row, column) AREF2((*chrom_energy), row, column)

float hz_to_step(float hz)
{
    return float((log(hz) - p2) / p1);
}

/*				GEN_MAGNITUDE
   given the real and imaginary portions of a complex FFT function, compute 
   the magnitude of the fft bin.
   given input of 2 arrays (inR and inI) of length n, takes the ith element
   from each, squares them, sums them, takes the square root of the sum and
   puts the output into the ith position in the array out.
   
   NOTE: out should be length n
*/
void gen_Magnitude(float* inR,float* inI, int low, int hi, float* out)
{
    int i;
    for (i = low; i < hi; i++) {
      float magVal = sqrt(inR[i] * inR[i] + inI[i] * inI[i]);
      //printf("   %d: sqrt(%g^2+%g^2)=%g\n",i,inR[i],inI[i+1],magVal);
      out[i]= magVal;
#ifdef VERBOSE
      if (i == 1000) printf("gen_Magnitude: %d %g\n", i, magVal);
#endif
    }
}


/*				PRINT_BINS
    This function is intended for debugging purposes.
    pass in an array representing the "mid point"
    of each bin, and the number of bins.  The
    function will print out:
    i value
    index falue
    low range of the bin
    middle of the bin
    high range of the bin
*/
void print_Bins(float* bins, int numBins){
    printf("BINS: \n");
    int i;
    for (i=0; i<numBins; i++) {
      int index = i % numBins;
      int indexNext = (index + 1) % numBins;
      int indexPrev = (index - 1) % numBins;
      int halfNext = int((bins[index]+bins[indexNext])/2);
      int halfPrev = int((bins[index]+bins[indexPrev])/2);     
      
      float maxValue =(bins[index]+bins[indexNext])/2;
      float minValue=(bins[index]+bins[indexPrev])/2;
      
      if(index == 1)
        maxValue =bins[index]+(bins[index]-((bins[index]+bins[indexPrev])/2));
      if(index == 2)
        minValue =bins[index]-(((bins[index]+bins[indexNext])/2)-bins[index]);
      
      printf("%d (%d) %g||%g||%g\n",i,index,minValue,bins[i],maxValue);
    }		
}

/*				MIN_BIN_NUM
    Returns the index in the array of bins
    of the "smallest" bin.  aka, the bin
    whose midpoint is the smallest.
*/
int min_Bin_Num(float* bins, int numBins){
    
    int i;
    int minIndex=0;
    float minValue=bins[0];
    for (i = 0; i < numBins; i++) {   
      if (minValue > bins[i]) {
        minValue = bins[i];
        minIndex = i;
      }
    }
    return minIndex;
}


/*				GEN_HAMMING
    given data from reading in a section of a sound file
    applies the hamming function to each sample.
    n specifies the length of in and out.
*/
void gen_Hamming(float* in, int n, float* out)
{
    int k = 0;
    for(k = 0; k < n; k++) {
      float internalValue = 2.0 * M_PI * k * (1.0 / (n - 1));
      float cosValue = cos(internalValue);
      float hammingValue = 0.54F + (-0.46F * cosValue);
#ifdef VERBOSE
      if (k == 1000) printf("Hamming %g\n", hammingValue);
#endif
      out[k] = hammingValue * in[k];
    }
}

/*				NEXTPOWEROF2
    given an int n, finds the next power of 2 larger than
    or equal to n.
*/
int nextPowerOf2(int n)
{
    int result = 1;
    while (result < n) result = (result << 1);
    return result;
}


int get_frames(snd_type snd_file)
{
    return (snd_file->u.file.end_offset - snd_file->u.file.byte_offset) / 
		   snd_bytes_per_frame(snd_file);
}
static bool reading_first_window = false;
static bool reading_last_window = false;

void read_window_init()
// call this function before a sequence of calls to read_next_window()
{
	reading_first_window = true;
}


bool read_next_window(snd_type file, float *data , float *temp_data, int win_samples,
					  int hop_samples)
// reads the next window of samples
//   the first time, fill half the window with zeros and the second half with data 
//     from the file
//   after that, shift the window by hop_size and fill the end of the window with
//     hop_size new samples
// the window is actually constructed in temp_data, then copied to data. That way, the
//   caller can apply a smoothing function to data and we'll still have a copy.
// the function returns false on the next call when detecting that there is no more samples, 
// data -- the window to be returned
// temp_data -- since we destroy data by windowing, temp_data saves overlapping samples
//          so we don't have to read them again
// win_samples -- must be even, note that first window is padded half-full with zeros
// hop_samples -- additional samples read each time after the first window
{
	snd_node float_sound; // descriptor: mono floats
	int frames_read;    // how many frames did we read?

	char *input_data = (char *) alloca(snd_bytes_per_frame(file) * win_samples);
	assert(input_data!=NULL) ;
	
	if (reading_first_window) {
		hop_samples = win_samples / 2; // first time we read more data	
		// zero end of temp_data, which will shift to beginning
		memset(temp_data + hop_samples, 0, sizeof(float) *(win_samples - hop_samples));
		reading_first_window = false;
	}
	
	
	// before reading in new sounds, shift temp_data by hop_size
	memmove(temp_data, temp_data + hop_samples, (win_samples-hop_samples) * sizeof(float));


	frames_read = snd_read(file, input_data, hop_samples);
	// zero any leftovers (happens at last frame):
		//printf("check fr %i  hs %i ws %i ",frames_read,hop_size,window_size); 
	memset(input_data + snd_bytes_per_frame(file) * frames_read, 0, 
		   snd_bytes_per_frame(file) * (win_samples - frames_read));
	assert(win_samples -frames_read >= 0);

	// now we have a full input_data buffer of samples to convert
	float_sound.format = file->format;
	float_sound.format.channels = 1; // make sure we convert to mono
	float_sound.format.mode = SND_MODE_FLOAT; // and convert to float
	float_sound.format.bits = 32;
	// note: snd_convert takes a frame count; divide samples by channels
	
	int converted = snd_convert(&float_sound, temp_data + win_samples - hop_samples,
	                file, input_data, hop_samples);
	if(frames_read == hop_samples)
		assert(converted == hop_samples);
	// now copy temp_data to data	
	memcpy(data, temp_data, sizeof(float) *win_samples);
	
	if(frames_read != hop_samples & reading_last_window==false){
		reading_last_window=true;
		return true; 
	}
	else if(reading_last_window==true){
		return false; 
		
	}

	else{		
		return true; 
	}
	

}



void snd_print_file_info(snd_type file)
{
    printf("file name = %s\n", file->u.file.filename);
    double sample_rate = file->format.srate;
    printf("   sample rate = %g\n", sample_rate);
    printf("   channels = %d\n", file->format.channels);
    /*=============================================================================*/
    long frames =(file->u.file.end_offset - file->u.file.byte_offset) / 
            snd_bytes_per_frame(file);
    end_offset_store=file->u.file.end_offset;
    printf("   End_offset is = %d\n",file->u.file.end_offset);
    printf("   Total frames number is = %d\n", frames);
    printf("   Bits per sample is  %d\n", file->format.bits);
    printf("   Audio Duration = %g seconds\n", (frames) / sample_rate);
    /*=============================================================================*/
}


/* GEN_CHROMA_AUDIO -- compute chroma for an audio file 
 */
int gen_chroma_audio(char *filename, int hcutoff, int lcutoff, 
                     float **chrom_energy, float *actual_frame_period)
{
    int i;
    int sequence=1;
    snd_node file;
    file.device = SND_DEVICE_FILE;
    file.write_flag = SND_READ;
    strcpy(file.u.file.filename, filename);
    long flags;
    float reg11[CHROMA_BIN_COUNT]; // temp storage1;
    float reg12[CHROMA_BIN_COUNT]; // temp storage2;

    for (i=0; i<CHROMA_BIN_COUNT; i++){
        reg11[i]=-999;
      }
    for (i=0; i<CHROMA_BIN_COUNT; i++){
        reg12[i]=0;
      }

    int infile = snd_open(&file, &flags);

    if (infile != SND_SUCCESS) {
	//file failed to open
		return -1;
    }

    snd_print_file_info(&file);

    double sample_rate = file.format.srate;
    long pcm_frames = get_frames(&file);    
    printf("   Total sample number is %ld\n", pcm_frames);
	//we want to make sure samples_per_frame is even, to keep things consistent
	//we'll change hopsize_samples the same way
    int samples_per_frame = (int) (window_size * sample_rate + 0.5);
	if(samples_per_frame%2 == 1) 
		samples_per_frame= samples_per_frame+1;
    printf("   samples per frame is %d \n", samples_per_frame);

   
   /*=============================================================*/
	
    int hopsize_samples= (int)(frame_period*sample_rate+.5);
    if (hopsize_samples%2 == 1) 
        hopsize_samples= hopsize_samples+1;
    *actual_frame_period = (hopsize_samples/sample_rate);

    int frame_count= (int)ceil(((float)pcm_frames/ hopsize_samples + 1)); 	

    printf("   Total chroma frames %d\n", frame_count); 
    printf("   Window size  %g second \n", window_size);
    printf("   Hopsize in samples %d \n", hopsize_samples);
   /*=============================================================*/

    //set up the buffer for reading in data
    int readcount = 0;
	
    // allocate some buffers for use in the loop
    int full_data_size = nextPowerOf2(samples_per_frame);
	//int hop_data_size = nextPowerOf2(samples_per_frame*hopsize_ratio);
    float *full_data = ALLOC(float, full_data_size);
    float *fft_dataR = ALLOC(float, full_data_size);
    float *fft_dataI = ALLOC(float, full_data_size);	
    float *temporary_data = ALLOC(float, full_data_size);
    //set to zero
    memset(full_data, 0, full_data_size*sizeof(float));
    memset(fft_dataR, 0, full_data_size*sizeof(float));	
    memset(fft_dataI, 0, full_data_size*sizeof(float));
    memset(temporary_data, 0, full_data_size*sizeof(float)); 
    //check to see if memory has been allocated
    assert(full_data!=NULL);
    assert(fft_dataR!=NULL);
    assert(fft_dataI!=NULL);
    assert(temporary_data!=NULL); 
   
    int *bin_map = ALLOC(int, full_data_size);
	
    //set up the chrom_energy array;
    (*chrom_energy) = ALLOC(float, frame_count * (CHROMA_BIN_COUNT + 1));
    int cv_index = 0;

    // set up mapping from spectral bins to chroma bins
    // ordinarily, we would add 0.5 to round to nearest bin, but we also
    // want to subtract 0.5 because the bin has a width of +/- 0.5. These
    // two cancel out, so we can just round down and get the right answer.
    int num_bins_to_use = (int) (hcutoff * full_data_size / sample_rate);
    // But then we want to add 1 because the loops will only go to 
    // high_bin - 1:
    int high_bin = min(num_bins_to_use + 1, full_data_size);
    //printf("center freq of high bin is %g\n", (high_bin - 1) * sample_rate / 
    //    full_data_size);
    //printf("high freq of high bin is %g\n", 
    //     (high_bin - 1 + 0.5) * sample_rate / full_data_size);
    // If we add 0.5, we'll round to nearest bin center frequency, but
    // bin covers a frequency range that goes 0.5 bin width lower, so we
    // add 1 before rounding.
    int low_bin = (int) (lcutoff * full_data_size / sample_rate) + 1;
    //printf("center freq of low bin is %g\n", low_bin * sample_rate / 
    //    full_data_size);
    //printf("low freq of low bin is %g\n", (low_bin - 0.5) * sample_rate / 
    //    full_data_size);
    //printf("frequency spacing of bins is %g\n", 
    //     sample_rate / full_data_size);
    double freq = low_bin * sample_rate / full_data_size;
    for(i = low_bin; i < high_bin; i++) {
      float raw_bin = hz_to_step(freq);
      int round_bin = (int) (raw_bin + 0.5F);
      int mod_bin = round_bin % 12;
      bin_map[i] = mod_bin;
      freq += sample_rate / full_data_size;
    }
    // printf("BIN_COUNT is !!!!!!!!!!!!!   %d\n",CHROMA_BIN_COUNT);

    read_window_init();
	
    while (read_next_window(&file, full_data, temporary_data,
			samples_per_frame, hopsize_samples)) {
      //fill out array with 0's till next power of 2
#ifdef VERBOSE
      printf("readcount %d sample %g\n", readcount, full_data[0]);
#endif
      for (i = samples_per_frame; i < full_data_size; i++) full_data[i] = 0;

#ifdef VERBOSE
      printf("preFFT: full_data[1000] %g\n", full_data[1000]);
#endif

      //the data from the wave file, each point mult by a hamming value
      gen_Hamming(full_data, full_data_size, full_data);


#ifdef VERBOSE
      printf("preFFT: hammingData[1000] %g\n", full_data[1000]);
#endif
      FFT(full_data_size, 0, full_data, NULL, fft_dataR, fft_dataI); //fft3
      
      //given the fft, compute the energy of each point
      gen_Magnitude(fft_dataR, fft_dataI, low_bin, high_bin, full_data);
      
      /*-------------------------------------
            GENERATE BINS AND PUT
            THE CORRECT ENERGY IN
            EACH BIN, CORRESPONDING
            TO THE CORRECT PITCH
      -------------------------------------*/

      float binEnergy[CHROMA_BIN_COUNT];
      int binCount[CHROMA_BIN_COUNT];

      for(i = 0; i < CHROMA_BIN_COUNT; i++) {
        binCount[i] = 0; 
        binEnergy[i] = 0.0;
      }
      
      for(i = low_bin; i < high_bin; i++) {
        int mod_bin = bin_map[i];
        binEnergy[mod_bin] += full_data[i];
        binCount[mod_bin]++;
      }

      /*-------------------------------------
        END OF BIN GENERATION
      -------------------------------------*/
      /* THE FOLLOWING LOOKS LIKE SOME OLD CODE TO COMPUTE
       * CHROMA FLUX, BUT IT IS NOT IN USE NOW 

      if (PRINT_BIN_ENERGY) {
        float mao1;
        float sum=0.;

        for (i = 0; i < CHROMA_BIN_COUNT; i++) {
            reg12[i]=binEnergy[i] / binCount[i];
        }
       
        if (reg11[0]==-999){
            printf("Chroma Flux \n\n");
        } else {
            for (i = 0; i < CHROMA_BIN_COUNT; i++) {
            }
            for (int k = 0; k < CHROMA_BIN_COUNT; k++) {
              float x = reg11[k];
              float y = reg12[k];
              float diff = x - y;
              sum += diff * diff;
            }
            mao1=sqrt( sum );         
            sequence++;          
            sum=0.;
            mao1=0.;
        }
        for (i = 0; i < CHROMA_BIN_COUNT; i++) {
            reg11[i]=reg12[i];
        }
        //fclose(Pointer);
      }
*/
      //put chrom energy into the returned array

#ifdef VERBOSE
      printf("cv_index %d\n", cv_index);
#endif
      for (i = 0;  i < CHROMA_BIN_COUNT; i++)
        CHROM(cv_index, i) = binEnergy[i] / binCount[i];
      cv_index++;
    } // end of while ((readcount = read_mono_floats...

    free(fft_dataI);
    free(fft_dataR);
    free(full_data);
    free(temporary_data);
    return frame_count;
}


class Event_list {
public:
	Alg_note_ptr note;
	Event_list *next;

	Event_list(Alg_event_ptr event_, Event_list *next_) {
		note = (Alg_note_ptr) event_;
		next = next_;
	}

	~Event_list() {
	}
};
typedef Event_list *Event_list_ptr;


/* gen_chroma_midi -- generate chroma vectors for midi file */
/*
  Notes: keep a list of notes that are sounding.
  For each frame, 
    zero the vector
    while next note starts before end of frame, insert note in list
	  for each note in list, compute weight and add to vector. Remove
	  if note ends before frame start time.	 
  How many frames? 
 */

int gen_chroma_midi(char *filename, int hcutoff, int lcutoff, 
		        float **chrom_energy, float *actual_frame_period)
{
	
	/*=============================================================*/

	*actual_frame_period = (frame_period) ; // since we don't quantize to samples
	
	/*=============================================================*/
    
	FILE *inf = fopen(filename, "rb");
    if (!inf) return -1;
    Alg_seq_ptr seq = alg_smf_read(inf, NULL);

    fclose(inf);
    seq->convert_to_seconds();
    /* find duration */
    float dur = 0.0F;
    int nnotes = 0;
    nnotes= find_midi_duration(seq,&dur); 

    /*================================================================*/
	
    int frame_count= (int)ceil(((float)dur/ frame_period + 1)); 	
	
    /*================================================================*/
	
    printf("file name = %s\n", filename);
    printf("note count = %d\n", nnotes);
    printf("duration in sec =%f\n", dur); 
    printf("chroma frames %d\n", frame_count);

    //set up the chrom_energy array;
    (*chrom_energy) = ALLOC(float, frame_count * (CHROMA_BIN_COUNT + 1));
    Event_list_ptr list = NULL;
    seq->iteration_begin();
    Alg_event_ptr event = seq->iteration_next();
    int cv_index;
    for (cv_index = 0; cv_index < frame_count; cv_index++) {
		
		/*====================================================*/

      float frame_begin = max((cv_index * (frame_period)) - window_size/2 , 0); //chooses zero if negative
			
		float frame_end= frame_begin +(window_size/2); 	
	/*============================================================*/
		/* zero the vector */
		for (int i = 0; i < CHROMA_BIN_COUNT; i++) CHROM(cv_index, i) = 0;
		/* add new notes that are in the frame */
		while (event && event->time < frame_end) {
			if (event->is_note()) {
     			list = new Event_list(event, list);
			}
    		event = seq->iteration_next();
		}
		/* remove notes that are no longer sounding */
		Event_list_ptr *ptr = &list;
		while (*ptr) {
			while ((*ptr) && 
				   (*ptr)->note->time + (*ptr)->note->dur < frame_begin) {
				Event_list_ptr temp = *ptr;
				*ptr = (*ptr)->next;
				delete temp;
			}
			if (*ptr) ptr = &((*ptr)->next);
		}
		for (Event_list_ptr item = list; item; item = item->next) {
			/* compute duration of overlap */
			float overlap = 
				    min(frame_end, item->note->time + item->note->dur) - 
				    max(frame_begin, item->note->time);
			float velocity = item->note->loud;
			float weight = overlap * velocity;
#if DEBUG_LOG
			fprintf(dbf, "%3d note: %d overlap %g velocity %g\n", 
					cv_index, item->note->pitch, overlap, velocity);
#endif
			CHROM(cv_index, (int)item->note->pitch % 12) += weight;
		}
#if DEBUG_LOG
		for (int i = 0; i < CHROMA_BIN_COUNT; i++) {
			fprintf(dbf, "%d:%g ", i, CHROM(cv_index, i));
		}
		fprintf(dbf, "\n\n");
#endif
	}
	while (list) {
		Event_list_ptr temp = list;
		list = list->next;
		delete temp;
	}
	seq->iteration_end();
    delete seq;
	return frame_count;
}


/* is_midi_file -- see if file name ends in .mid */
/**/
bool is_midi_file(char *filename)
{
	size_t len = strlen(filename);
	return (len > 4 && strcmp(filename + len - 4, ".mid") == 0);
}


/*				GEN_CHROMA
    generates the chroma energy for a given file
    with a low cutoff and high cutoff.  
    The chroma energy is placed in the float *chrom_energy.
    this 2D is an array of pointers.
    The function returns the number of frames 
    (aka the length of the 1st dimention of chrom_energy)
*/
int gen_chroma(char *filename, int hcutoff, int lcutoff, float **chrom_energy, 
               float *actual_frame_period)
{
	size_t len = strlen(filename);
	if (is_midi_file(filename)) {
		return gen_chroma_midi(filename, hcutoff, lcutoff, chrom_energy, 
							   actual_frame_period);
	} else {
		return gen_chroma_audio(filename, hcutoff, lcutoff, chrom_energy, 
							    actual_frame_period);
	}
}
