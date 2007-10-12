/* sndwrite.h -- header to write sounds to files */

double sound_save(LVAL snd_expr, long n,
                  unsigned char *filename, long format,
                  long mode, long bits, double *sr, long *nchans,
                  double *duration);
/* LISP: (SND-SAVE ANY FIXNUM STRING FIXNUM FIXNUM FIXNUM ANYNUM^ FIXNUM^ ANYNUM^) */

double sound_overwrite(LVAL snd_expr, long n,
                       unsigned char *filename, long byte_offset,
                       long mode, long bits, double sr, long nchans,
                       double *duration);
/* LISP: (SND-OVERWRITE ANY FIXNUM STRING FIXNUM FIXNUM FIXNUM ANYNUM FIXNUM ANYNUM^) */


