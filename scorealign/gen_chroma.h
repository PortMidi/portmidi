#define CHROMA_BIN_COUNT 12

/*				GEN_CHROMA
generates the chroma energy for a given file
with a low cutoff and high cutoff.  
The chroma energy is placed in the float** chrom_energy.
this 2D is an array of pointers.  the pointers point to an array 
of lenght 12, representing the 12 chroma bins
The function returns the number of frames (aka the length of the 1st dimention of chrom_energy
*/
int gen_chroma(char *filename, int hcutoff, int lcutoff, float **chrom_energy, 
               float *actual_frame_period);

bool is_midi_file(char *filename);

#define AREF2(chrom_energy, row, column) \
    (chrom_energy[row * (CHROMA_BIN_COUNT + 1) + column])
