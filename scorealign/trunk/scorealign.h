extern float frame_period; // time in seconds
extern float window_size;
extern float actual_frame_period_1; // time in seconds
extern float actual_frame_period_2; // time in seconds

#define DEBUG_LOG 1
#if DEBUG_LOG
extern FILE *dbf;
#endif

int find_midi_duration(Alg_seq &seq, float *dur);
