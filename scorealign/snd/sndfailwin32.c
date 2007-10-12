/* sndfailwin32.c -- version of sndfail that puts up a message box */

/* this should not be compiled into an snd library! handling snd_fail
 * is application specific
 */

#include <windows.h>


void snd_fail(char *msg)
{
    MessageBox(0, msg, 0, 0);
    exit(1);
}


void snd_warn(char *msg)
{
    MessageBox(0, msg, 0, MB_OK);
}
