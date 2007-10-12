/* sndfileio.h -- defines system-independent file IO */

#define SND_SEEK_CUR    1
#define SND_SEEK_END    2
#define SND_SEEK_SET    0

#define SND_RDONLY 0
#define SND_RDWR   2

int snd_file_open(char *fname, int mode);
int snd_file_creat(char *fname);
int snd_file_lseek(int file, long offset, int param);
long snd_file_read(int fp, char *data, long len);
long snd_file_write(int fp, char *data, long len);
int snd_file_close(int fp);
long snd_file_len(int fp);
