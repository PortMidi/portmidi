// allegrosmfwr.cpp -- Allegro Standard Midi File Write

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include "allegro.h"

// event_queue is a list element that keeps track of pending
// things to write to a track, including note-ons, note-offs,
// updates, tempo changes, and time signatures
//
class event_queue{
public:
  char type;//'n' for note, 'o' for off, 's' for time signature,
            // 'c' for tempo changes
  double time;
  long index; //of the event in mSeq->notes
  class event_queue *next;
  event_queue(char t, double when, long x, class event_queue *n) {
        type = t; time = when; index = x; next = n; }
};


class Alg_smf_write {
public:
	Alg_smf_write(Alg_seq_ptr seq);
	~Alg_smf_write();
	long channels_per_track; // used to encode track number into chan field
		// chan is actual_channel + channels_per_track * track_number
		// default is 100, set this to 0 to merge all tracks to 16 channels

	void write(FILE *fp /* , midiFileFormat = 1 */);

private:
	long previous_divs; // time in ticks of most recently written event

	void write_track(int i);
	void write_tempo(int divs, int tempo);
	void write_tempo_change(int i);
	void write_time_signature(int i);
	void write_note(Alg_note_ptr note, bool on);
	void write_update(Alg_update_ptr update);
	void write_text(char type, char length, char *s, double event_time);
	void write_data(int data);
	int to_midi_channel(int channel);
	int to_track(int channel);

	FILE *out_file;

	Alg_seq_ptr seq;

	int num_tracks; // number of tracks not counting tempo track
	int division; // divisions per quarter note, default = 120
	int initial_tempo;

	int timesig_num; // numerator of time signature
	int timesig_den; // denominator of time signature
	double timesig_when; // time of time signature

	int keysig;          // number of sharps (+) or flats (-), -99 for undefined
	char keysig_mode; // 'M' or 'm' for major/minor
	double keysig_when;	// time of key signature

	void write_delta(double event_time);
	void write_varinum(int num);
	void write_16bit(int num);
	void write_24bit(int num);
	void write_32bit(int num);
};

#define ROUND(x) (int) ((x)+0.5)

Alg_smf_write::Alg_smf_write(Alg_seq_ptr a_seq)
{
    out_file = NULL;

    division = 120;          // divisions per quarter note
	timesig_num = timesig_den = 0; // initially undefined
    keysig = -99;
	keysig_mode = 0;
    initial_tempo = 500000;

    seq = a_seq;

	previous_divs = 0; // used to compute deltas for midifile
}


Alg_smf_write::~Alg_smf_write()
{
}


event_queue* push(event_queue *queue, event_queue *event)
{
    if (queue == NULL) {
		event->next = NULL;
		return event;
	}
   
	event_queue *marker1 = NULL;
	event_queue *marker2 = queue;
	while (marker2 != NULL && marker2->time <= event->time) {
		marker1 = marker2;
		marker2 = marker2->next;
	}
    event->next = marker2;
	if (marker1 != NULL) {
		marker1->next=event;
		return queue;
	} else return event;
}


void print_queue(event_queue *q)
{
    printf("Printing queue. . .\n");
    event_queue *q2=q;
    while (q2) {
        printf("%c at %f ;", q2->type, q2->time);
        q2 = q2->next;
	}
    printf("\nDone printing.\n");
}


void Alg_smf_write::write_note(Alg_note_ptr note, bool on)
{
	double event_time = (on ? note->time : note->time + note->dur);
    write_delta(event_time);

    //printf("deltaDivisions: %d, beats elapsed: %g, on? %c\n", deltaDivisions, note->time, on);

    char chan = (note->chan & 15);
    int pitch = int(note->pitch + 0.5);
    if (pitch < 0) {
  	    pitch = pitch % 12;
    } else if (pitch > 127) {
	    pitch = (pitch % 12) + 120; // put pitch in 10th octave
	    if (pitch > 127) pitch -= 12; // or 9th octave
    }
	putc(0x90 + chan, out_file);
    putc(pitch, out_file);
	if (on) {
        int vel = (int) note->loud;
		if (vel <= 0) vel = 1;
		write_data(vel);
	} else putc(0, out_file); // note-off indicated by velocty zero
}


void Alg_smf_write::write_text(char type, char length, char *s, double event_time)
{
    write_delta(event_time);
    putc(0xFF, out_file);
    putc(type, out_file);
    putc(length, out_file);
    fprintf(out_file, s);
    //printf("Inserted text %s, of length %X, of type: %x.\n", s, length, type);
}


void Alg_smf_write::write_data(int data)
{
	if (data < 0) data = 0;
	else if (data > 0xFF) data = 0xFF;

	putc(data, out_file);
}


int Alg_smf_write::to_midi_channel(int channel)
{
	// allegro track number is stored as multiple of 100
	// also mask off all but 4 channel bits just in case
	if (channels_per_track > 0) channel %= channels_per_track;
	return channel & 0xF;
}


int Alg_smf_write::to_track(int channel)
{
	if (channel == -1) return 0;
	return channel / channels_per_track;
}


void Alg_smf_write::write_update(Alg_update_ptr update)
{
    char *name = update->parameter.attr_name();

    /****Non-Meta Events****/
    if (!strcmp(name, "pressurer")) {
	    write_delta(update->time);
		if (update->get_identifier() < 0) { // channel pressure message
		    write_data(0xD0 + to_midi_channel(update->chan));
		    write_data((int)(update->parameter.r * 127));
		} else { // just 1 key -- poly pressure
		    write_data(0xA0 + to_midi_channel(update->chan));
		    write_data(update->get_identifier());
		    write_data((int)(update->parameter.r * 127));
		}
	} else if (!strcmp(name, "programi")) {
		write_delta(update->time);
		write_data(0xC0 + to_midi_channel(update->chan));
		write_data(update->parameter.i);
	} else if (!strcmp(name, "bendr")) {
		int temp = ROUND(8192.0 * (update->parameter.r + 1));
		if (temp > 8191) temp = 8191; // 14 bits maximum
		if (temp < 0) temp = 0;
		int c2 = temp & 0x7F; // low 7 bits
		int c1 = temp >> 7;   // high 7 bits
		write_delta(update->time);
		write_data(0xE0 + to_midi_channel(update->chan));
		write_data(c1);
		write_data(c2);
	} else if (!strncmp(name, "control", 7) && 
		       update->parameter.attr_type() == 'r') {
	  int ctrlnum = atoi(name + 7);
	  int val = ROUND(update->parameter.r * 127);
	  write_delta(update->time);
	  write_data(0xB0 + to_midi_channel(update->chan));
	  write_data(ctrlnum);
	  write_data(val);

	/****Text Events****/
	} else if (!strcmp(name, "texts")) {
		write_text(0x01, (char) strlen(update->parameter.s),
				  update->parameter.s, update->time);
	} else if (!strcmp(name, "copyrights")) {
		write_text(0x02, (char) strlen(update->parameter.s),
				  update->parameter.s, update->time);
	} else if (!strcmp(name, "names")) {
		write_text(0x03, (char) strlen(update->parameter.s),
				  update->parameter.s, update->time);
	} else if (!strcmp(name, "instruments")) {
		write_text(0x04, (char) strlen(update->parameter.s),
				  update->parameter.s, update->time);
	} else if (!strcmp(name, "lyrics")) {
		write_text(0x05, (char) strlen(update->parameter.s),
				  update->parameter.s, update->time);
	} else if (!strcmp(name, "markers")) {
		write_text(0x06, (char) strlen(update->parameter.s),
				  update->parameter.s, update->time);
	} else if (!strcmp(name, "cues")) {
		write_text(0x07, (char) strlen(update->parameter.s),
				  update->parameter.s, update->time);
	} else if (!strcmp(name, "misc")) {
		write_text(0x08, (char) strlen(update->parameter.s),
				  update->parameter.s, update->time);

	/****Other Events****/
	// key signature is special because it takes two events in the Alg_seq
    // structure to make one midi file event. When we encounter one or 
    // the other event, we'll just record it in the Alg_smf_write object.
	// After both events are seen, we write the data. (See below.)
	} else if (!strcmp(name, "keysigi")) {
		keysig = update->parameter.i;
		keysig_when = update->time;
	} else if (!strcmp(name, "modea")) {
	    if (!strcmp(update->parameter.attr_name(), "majora"))
		    keysig_mode = 'M';
		else keysig_mode = 'm';
		keysig_when = update->time;
	}
	if (keysig != -99 && keysig_mode) { // write when both are defined
		write_delta(keysig_when);
		write_data(0xFF);
		write_data(0x59);
		write_data(2);
		write_data(keysig);
		write_data(keysig_mode == 'm');
		keysig = -99;
		keysig_mode = false;
	}
    //printf("Update: %s, key: %g\n", update->parameter.attr_name(), update->key);
}


void Alg_smf_write::write_track(int i)
{
	int j = 0; // note index
	Alg_events &notes = seq->track_list[i];
	event_queue *pending = NULL;
	if (notes.length() > 0) {
		pending = new event_queue('n', notes[j]->time, 0, NULL);
	}
	if (i == 0) { // track 0 may have tempo and timesig info
		if (seq->get_time_map()->last_tempo_flag || seq->get_time_map()->beats.len > 0) {
			pending = push(pending, new event_queue('c', 0.0, 0, NULL));
		}
		if (seq->time_sig.length() > 0) {
			pending = push(pending, new event_queue('s', seq->time_sig[0].beat,
												    0, NULL));
		}
	}
	while (pending) {
	    event_queue *current = pending;
		pending = pending->next;
		if (current->type == 'n') {
			Alg_note_ptr n = (Alg_note_ptr) notes[current->index];
		    if (n->is_note()) {
			    write_note(n, true);
				pending = push(pending, new event_queue('o', n->time + n->dur,
							   current->index, NULL));
			} else if (n->is_update()) {
				Alg_update_ptr u = (Alg_update_ptr) n;
				write_update(u);
			}
			int next = current->index + 1;
			if (next < notes.length()) {
				current->time = notes[next]->time;
				current->index = next;
				pending = push(pending, current);
			}
		} else if (current->type == 'o') { //note-off
			Alg_note_ptr n = (Alg_note_ptr) notes[current->index];
			write_note(n, false);
			delete current;
		} else if (current->type == 'c') { // tempo change
			write_tempo_change(current->index);
			current->index++; // -R
			if (current->index < seq->get_time_map()->beats.len) {
				current->time = seq->get_time_map()->beats[current->index].beat;
 				pending = push(pending, current);
			} else {
				delete current;
			}
		} else if (current->type == 's') { // time sig
			write_time_signature(current->index);
			current->index++;
			if (current->index < seq->time_sig.length()) {
				current->time = seq->time_sig[current->index].beat;
				pending = push(pending, current);
			} else {
				delete current;
			}
		}
	}
}


void Alg_smf_write::write_tempo(int divs, int tempo)
{
	//	printf("Inserting tempo %f after %f clocks.\n", tempo, delta);
	write_varinum(divs - previous_divs);
	previous_divs = divs;
	putc(0xFF, out_file);
	putc(0x51, out_file);
	putc(0x03, out_file);
	write_24bit((int)tempo);
}


void Alg_smf_write::write_tempo_change(int i)
	//  i is index of tempo map
{
    // extract tempo map
    Alg_beats &b = seq->get_time_map()->beats;
    double tempo;
    long divs;
	if (i < seq->get_time_map()->beats.len - 1) {
		tempo = 1000000 * ((b[i+1].time - b[i].time) / 
						   (b[i+1].beat - b[i].beat));
		divs = ROUND(b[i].beat * division);
		write_tempo(divs, ROUND(tempo));
	} else if (seq->get_time_map()->last_tempo_flag) { // write the final tempo
		divs = ROUND(division * b[i].beat);
		tempo = (1000000.0 / seq->get_time_map()->last_tempo);
		write_tempo(divs, ROUND(tempo));
	}    
}


void Alg_smf_write::write_time_signature(int i)
{
	Alg_time_sigs &ts = seq->time_sig;
	// write the time signature
	long divs = ROUND(ts[i].beat * division);
	write_varinum(divs - previous_divs);
	putc(0xFF, out_file);
	putc(0x58, out_file); // time signature
	putc(4, out_file);    // length of message
	putc(ROUND(ts[i].num), out_file);
	int den = ROUND(ts[i].den);
	int den_byte = 0;
	while (den > 1) { // compute the log2 of denominator
		den_byte++;
		den >>= 1;
	}
	putc(den_byte, out_file);
	putc(24, out_file); // clocks per quarter
	putc(8, out_file);  // 32nd notes per 24 clocks
}



void Alg_smf_write::write(FILE *fp)
{
    int track_len_offset;
    int track_end_offset;
    int track_len;

    fprintf(stderr, "Writing MIDI file...\n");

    out_file = fp;

    // Header
    fprintf(out_file, "MThd");
    write_32bit(6); // chunk length

    write_16bit(1); // format 1 MIDI file

    write_16bit(seq->tracks()); // number of tracks
    write_16bit(division); // divisions per quarter note


    // write_ all tracks
	seq->convert_to_beats();
	int i;
	for (i = 0; i < seq->tracks(); i++) {
		previous_divs = 0;
	    fprintf(out_file, "MTrk");
		track_len_offset = ftell(out_file);
		write_32bit(0); // track len placeholder

		write_track(i);

		// End of track event
		write_varinum(0);           // delta time
		putc(0xFF, out_file);
		putc(0x2F, out_file);
		putc(0x00, out_file);

		// Go back and write in the length of the track
		track_end_offset = ftell(out_file);
		track_len = track_end_offset - track_len_offset - 4;
		fseek(out_file, track_len_offset, SEEK_SET);
		write_32bit(track_len);
		printf("track_len %d\n", track_len);
		fseek(out_file, track_end_offset, SEEK_SET);
	}
    fprintf(stderr, "Done\n");
}


void Alg_smf_write::write_16bit(int num)
{
    putc((num & 0xFF00) >> 8, out_file);
    putc((num & 0xFF), out_file);
}

void Alg_smf_write::write_24bit(int num)
{
    putc((num & 0xFF0000) >> 16, out_file);
    putc((num & 0xFF00) >> 8, out_file);
    putc((num & 0xFF), out_file);
}

void Alg_smf_write::write_32bit(int num)
{
    putc((num & 0xFF000000) >> 24, out_file);
    putc((num & 0xFF0000) >> 16, out_file);
    putc((num & 0xFF00) >> 8, out_file);
    putc((num & 0xFF), out_file);
}


void Alg_smf_write::write_delta(double event_time)
{
    // divisions is ideal absolute time in divisions
    long divisions = ROUND(division * event_time);
    long delta_divs = divisions - previous_divs;
    write_varinum(delta_divs);
    previous_divs = divisions;    
}


void Alg_smf_write::write_varinum(int value)
{
  if(value<0) value=0;//this line should not have to be here!
  int buffer;

  buffer = value & 0x7f;
  while ((value >>= 7) > 0) {
    buffer <<= 8;
    buffer |= 0x80;
    buffer += (value & 0x7f);
  }

  for(;;) {
    putc(buffer, out_file);
    if (buffer & 0x80)
      buffer >>= 8;
    else
      break;
  }
}


// Note: caller must open file in binary mode,
// caller should close file.
//
void alg_smf_write(Alg_seq_ptr seq, FILE *file)
{
    Alg_smf_write writer(seq);
    writer.write(file);
}

void Alg_seq::smf_write(FILE *file)
{
	Alg_smf_write writer(this);
	writer.write(file);
}

int Alg_seq::smf_write(const char *filename)
{
	FILE *outf = fopen(filename, "wb");
	if (!outf) {
		return errno;
	}
	smf_write(outf);
	fclose(outf);
	return 0;
}

