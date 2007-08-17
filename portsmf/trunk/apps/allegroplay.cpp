
#include "stdlib.h"
#include "stdio.h"
#include "memory.h"
#include "assert.h"
#include "allegro.h"
#include "mfmidi.h"
#include "portmidi.h"
#include "seq2midi.h"
#include "string.h"
#include "strparse.h"

#ifdef WIN32
#include "crtdbg.h" // for memory allocation debugging
#endif

void midi_fail(char *msg)
{
    printf("Failure: %s\n", msg);
    exit(1);
}


void *midi_alloc(size_t s) { return malloc(s); }
void midi_free(void *a) { free(a); }


Alg_seq_ptr read_allegro_file(char *name)
{
    FILE *inf = fopen(name, "r");
    if (!inf) {
        printf("could not open allegro file\n");
        exit(-1);
    }
    Alg_seq_ptr seq = new Alg_seq(inf, false);
    fclose(inf);
    return seq;
}


Alg_seq_ptr read_file(char *name)
{
    FILE *inf = fopen(name, "rb");
    if (!inf) {
        printf("could not open midi file\n");
        exit(1);
    }
    Alg_seq_ptr seq = new Alg_seq(inf, true);
    fclose(inf);
    return seq;
}


void print_help()
{
    printf("Usage: allegroplay [-m] [-a] <filename> [-n]\n");
    printf("    use -m for midifile, -a for allegro file.\n");
    printf("    .mid extension implies -m, .gro implies -a\n");
    printf("    use -n for non-interactive use (no prompts)\n");
    exit(-1);
}


int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_help();
    }
    char *filename = NULL;
    bool midifile = false;
    bool allegrofile = false;
    bool interactive = true;
    int i = 1; // scan the command line
    while (i < argc) {
        if (argv[i][0] == '-') {
            if (argv[1][1] == 'm') midifile = true;
            else if (argv[i][1] == 'a') allegrofile = true;
            else if (argv[i][1] == 'n') interactive = false;
        } else {
            filename = argv[i];
        }
        i++;
    }
    if (!filename) {
        print_help();
    }

    if (!midifile && !allegrofile) {
        int len = strlen(filename);
        if (len < 4) print_help();    // no extension, need -m or -a
        char *ext = filename + len - 4;
        if (strcmp(ext, ".mid") == 0) midifile = true;
        else if (strcmp(ext, ".gro") == 0) allegrofile = true;
        else print_help();
    }
    Alg_seq_ptr seq;
    if (midifile) {
        seq = read_file(filename);
    } else if (allegrofile) {
        seq = read_allegro_file(filename);
    }

    int events = 0;
    for (i = 0; i < seq->tracks(); i++) {
        events += seq->track(i)->length();
    }
    if (interactive) {
        printf("%d tracks, %d events\n", seq->tracks(), events);
    }
    /* PLAY THE FILE VIA MIDI: */
    if (interactive) {
        printf("type return to play midi file: ");
        char input[80];
        fgets(input, 80, stdin);
    }
    seq_play(seq);
    /**/

    /* DELETE THE DATA */
    delete seq;

    return 0;
}
