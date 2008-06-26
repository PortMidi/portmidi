// allegrowr.cpp -- write sequence to an Allegro file (text)

#include "stdlib.h"
#include "stdio.h"
#include "assert.h"
#include "memory.h"
#include "string.h"
#include "strparse.h"
#include "allegro.h"
#include <errno.h>


void parameter_print(FILE *file, Alg_parameter_ptr p)
{
    char str[256];
    fprintf(file, " -%s:", p->attr_name());
    switch (p->attr_type()) {
    case 'a':
        fprintf(file, "'%s'", p->a);
        break;
    case 'i':
        fprintf(file, "%d", p->i);
        break;
    case 'l':
        fprintf(file, "%s", p->l ? "true" : "false");
        break;
    case 'r':
        fprintf(file, "%g", p->r);
        break;
    case 's':
        string_escape(str, p->s, "\"");
        fprintf(file, "%s", str);
        break;
    }
}


void Alg_seq::write(FILE *file, bool in_secs)
{
    int i, j;
    if (in_secs) convert_to_seconds();
    else convert_to_beats();
    // first write the tempo map
    fprintf(file, "#track 0\n");
    Alg_beats &beats = time_map->beats;
    for (i = 0; i < beats.len - 1; i++) {
        Alg_beat_ptr b = &(beats[i]);
        if (in_secs) {
            fprintf(file, "T%g", b->time);
        } else {
            fprintf(file, "TW%g", b->beat / 4);
        }
        double tempo = (beats[i + 1].beat - b->beat) /
                       (beats[i + 1].time - beats[i].time);
        fprintf(file, " -tempor:%g\n", tempo * 60);
    }
    if (time_map->last_tempo_flag) { // we have final tempo:
        Alg_beat_ptr b = &(beats[beats.len - 1]);
        if (in_secs) {
            fprintf(file, "T%g", b->time);
        } else {
            fprintf(file, "TW%g", b->beat / 4);
        }
        fprintf(file, " -tempor:%g\n", time_map->last_tempo * 60.0);
    }

    // write the time signatures
    for (i = 0; i < time_sig.length(); i++) {
        Alg_time_sig &ts = time_sig[i];
        double time = ts.beat;
        if (in_secs) {
            fprintf(file, "T%g V- -timesig_numr:%g\n", time, ts.num); 
            fprintf(file, "T%g V- -timesig_denr:%g\n", time, ts.den);
        } else {
            double wholes = ts.beat / 4;
            fprintf(file, "TW%g V- -timesig_numr:%g\n", time / 4, ts.num);
            fprintf(file, "TW%g V- -timesig_denr:%g\n", time / 4, ts.den);
        }
    }

    for (j = 0; j < track_list.length(); j++) {
        Alg_events &notes = track_list[j];
        if (j != 0) fprintf(file, "#track %d\n", j);
        // now write the notes at beat positions
        for (i = 0; i < notes.length(); i++) {
            Alg_event_ptr e = notes[i];
            double start = e->time;
            if (in_secs) {
                fprintf(file, "T%g ", start);
            } else {
                fprintf(file, "TW%g ", start / 4);
            }
            // write the channel as Vn or V-
            if (e->chan == -1) fprintf(file, "V-");
            else fprintf(file, "V%d", e->chan);
            // write the note or update data
            if (e->is_note()) {
                Alg_note_ptr n = (Alg_note_ptr) e;
                double dur = n->dur;
                fprintf(file, " K%d P%g ", n->get_identifier(), n->pitch);
                if (in_secs) {
                    fprintf(file, "U%g ", dur);
                } else {
                    fprintf(file, "Q%g ", dur);
                }
                fprintf(file, "L%g ", n->loud);
                Alg_parameters_ptr p = n->parameters;
                while (p) {
                    parameter_print(file, &(p->parm));
                    p = p->next;
                }
            } else { // an update
                assert(e->is_update());
                Alg_update_ptr u = (Alg_update_ptr) e;
                if (u->get_identifier() != -1) {
                    fprintf(file, " K%d", u->get_identifier());
                }
                parameter_print(file, &(u->parameter));
            }
            fprintf(file, "\n");
        }
    }
}

int Alg_seq::write(const char *filename)
{
     FILE *file = fopen(filename, "w");
     if (!file) return errno;
     write(file, units_are_seconds);
     fclose(file);
     return 0;
}
