#include "assert.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "ctype.h"
//#include "memory.h"
#include "trace.h"
#include "strparse.h"
#include "allegro.h"

#define field_max 80

class Alg_reader {
public:
    FILE *file;
    int line_no;
    String_parse line_parser;
    bool line_parser_flag;
    char field[field_max];
    bool error_flag;
    Alg_seq_ptr seq;
    double tsnum;
    double tsden;

    Alg_reader(FILE *a_file, Alg_seq_ptr new_seq);
    void readline();
    Alg_parameters_ptr process_attributes(Alg_parameters_ptr attributes, double time);
    bool parse();
	long parse_chan(char *field);
    long parse_int(char *field);
    int find_real_in(char *field, int n);
    double parse_real(char *field);
    void parse_error(char *field, long offset, char *message);
    double parse_dur(char *field, double base);
    double parse_after_dur(double dur, char *field, int n, double base);
    double parse_loud(char *field);
    long parse_key(char *field);
    double parse_pitch(char *field);
    long parse_after_key(int key, char *field, int n);
    long find_int_in(char *field, int n);
    bool parse_attribute(char *field, Alg_parameter_ptr parm);
    bool parse_val(Alg_parameter_ptr param, char *s, int i);
    bool check_type(char type_char, Alg_parameter_ptr param);
};


#define streql(s1, s2) (strcmp(s1, s2) == 0)


void subseq(char *result, char *source, int from, int to)
{
    memcpy(result, source + from, to - from);
    result[to - from] = 0;
}


double Alg_reader::parse_pitch(char *field)
{
    if (isdigit(field[1])) {
        char real_string[80];
        int last = find_real_in(field, 1);
        subseq(real_string, field, 1, last);
        return atof(real_string);
    } else {
        return (double) parse_key(field);
    }
}


// it is the responsibility of the caller to delete
// the seq
Alg_reader::Alg_reader(FILE *a_file, Alg_seq_ptr new_seq)
{
    file = a_file; // save the file
    line_parser_flag = false;
    line_no = 0;
    tsnum = 4; // default time signature
    tsden = 4;
	seq = new_seq;
}


Alg_seq_ptr alg_read(FILE *file, Alg_seq_ptr new_seq)
	// read a sequence from allegro file
{
	if (!new_seq) new_seq = new Alg_seq();
	Alg_reader alg_reader(file, new_seq);
	alg_reader.parse();
	return alg_reader.seq;
}


void Alg_reader::readline()
{
    char line[256];
    char *line_flag = fgets(line, 256, file);
    line_parser_flag = false;
    if (line_flag) {
        line_parser.init(line);
        line_parser_flag = true;
        error_flag = false;
    }
}


Alg_parameters_ptr Alg_reader::process_attributes(
	    Alg_parameters_ptr attributes, double time)
{
    // print "process_attributes:", attributes
    bool ts_flag = false;
    if (attributes) {
        Alg_parameters_ptr a;
        if (a = Alg_parameters::remove_key(&attributes, "tempor")) {
            double tempo = a->parm.r;
            seq->insert_tempo(tempo, seq->get_time_map()->time_to_beat(time));
        }
        if (a = Alg_parameters::remove_key(&attributes, "beatr")) {
            double beat = a->parm.r;
            seq->insert_beat(time, beat);
        }
        if (a = Alg_parameters::remove_key(&attributes, "timesig_numr")) {
            tsnum = a->parm.r;
            ts_flag = true;
        }
        if (a = Alg_parameters::remove_key(&attributes, "timesig_denr")) {
            tsden = a->parm.r;
            ts_flag = true;
        }
        if (ts_flag) {
            seq->set_time_sig(seq->get_time_map()->time_to_beat(time), 
				              tsnum, tsden);
        }
    }
	return attributes; // in case it was modified
}


bool Alg_reader::parse()
{
    int voice = 0;
    int key = 60;
    double loud = 100.0;
    double pitch = 60.0;
    double dur = 1.0;
    double time = 0.0;
	int track_num = 0;
	seq->convert_to_seconds();
    readline();
    bool valid = false; // ignore blank lines
    while (line_parser_flag) {
        bool time_flag = false;
        bool next_flag = false;
        double next;
        bool voice_flag = false;
        bool loud_flag = false;
        bool dur_flag = false;
        bool new_pitch_flag = false; // "P" syntax
        double new_pitch = 0.0;
        bool new_key_flag = false;   // "K" syntax
        int new_key = 0;
        bool new_note_flag = false;  // "A"-"G" syntax
        int new_note = 0;
        Alg_parameters_ptr attributes = NULL;
		if (line_parser.peek() == '#') {
			// look for #track
			line_parser.get_nonspace_quoted(field);
			if (streql(field, "#track")) {
				line_parser.get_nonspace_quoted(field); // number
				track_num = parse_int(field - 1);
				seq->add_track(track_num);
			}
			// maybe we have a comment
		} else {
			// we must have a track to insert into
			if (seq->tracks() == 0) seq->add_track(0);
			line_parser.get_nonspace_quoted(field);
			char pk = line_parser.peek();
			// attributes are parsed as two adjacent nonspace_quoted tokens
			// so we have to conditionally call get_nonspace_quoted() again
			if (pk && !isspace(pk)) {
				line_parser.get_nonspace_quoted(field + strlen(field));
			}
			while (field[0]) {
				char first = toupper(field[0]);
				if (strchr("ABCDEFGKLPUSIQHW-", first)) {
					valid = true; // it's a note or event
				}
				if (first == 'V') {
					if (voice_flag) {
						parse_error(field, 0, "Voice specified twice");
					} else {
						voice = parse_chan(field);
					}
					voice_flag = true;
				} else if (first == 'T') {
					if (time_flag) {
						parse_error(field, 0, "Time specified twice");
					} else {
						time = parse_dur(field, 0.0);
					}
					time_flag = true;
				} else if (first == 'N') {
					if (next_flag) {
						parse_error(field, 0, "Next specified twice");
					} else {
						next = parse_dur(field, time);
					}
					next_flag = true;
				} else if (first == 'K') {
					if (new_key_flag) {
						parse_error(field, 0, "Key specified twice");
					} else {
						new_key = parse_key(field);
						new_key_flag = true;
					}
				} else if (first == 'L') {
					if (loud_flag) {
						parse_error(field, 0, "Loudness specified twice");
					} else {
						loud = parse_loud(field);
					}
					loud_flag = true;
				} else if (first == 'P') {
					if (new_note_flag || new_pitch_flag) {
						parse_error(field, 0, "Pitch specified twice");
					} else {
						new_pitch = parse_pitch(field);
						new_pitch_flag = true;
					}
				} else if (first == 'U') {
					if (dur_flag) {
						parse_error(field, 0, "Dur specified twice");
					} else {
						dur = parse_dur(field, time);
						dur_flag = true;
					}
				} else if (strchr("SIQHW", first)) {
					if (dur_flag) {
						parse_error(field, 0, "Dur specified twice");
					} else {
						// prepend 'U' to field, copy EOS too
						memmove(field + 1, field, strlen(field) + 1);
						field[0] = 'U';
						dur = parse_dur(field, time);
						dur_flag = true;
					}
				} else if (strchr("ABCDEFG", first)) {
					if (new_note_flag || new_pitch_flag) {
						parse_error(field, 0, "Pitch specified twice");
					} else {
						// prepend 'K' to field, copy EOS too
						memmove(field + 1, field, strlen(field) + 1);
						field[0] = 'K';
						new_note = parse_key(field);
						new_note_flag = true;
					}
				} else if (first == '-') {
					Alg_parameter parm;
					if (parse_attribute(field, &parm)) { // enter attribute-value pair
						attributes = new Alg_parameters(attributes);
						attributes->parm = parm;
						parm.s = NULL; // protect string from deletion by destructor
					}
				} else {
					parse_error(field, 0, "Unknown field");
				}

				if (error_flag) {
					field[0] = 0; // exit the loop
				} else {
					line_parser.get_nonspace_quoted(field);
					pk = line_parser.peek();
					// attributes are parsed as two adjacent nonspace_quoted 
					// tokens so we have to conditionally call 
					// get_nonspace_quoted() again
					if (pk && !isspace(pk)) {
						line_parser.get_nonspace_quoted(field + strlen(field));
					}
				}
			}
			// a case analysis:
			// Key < 128 counts as both key and pitch
			// A-G implies pitch AND key unless key given too
			//   K60 P60 -- both are specified, use 'em
			//   K60 P60 C4 -- overconstrained, an error
			//   K60 C4 -- overconstrained
			//   K60 -- OK, pitch is 60
			//   C4 P60 -- over constrained
			//   P60 -- OK, key is from before, pitch is 60
			//   C4 -- OK, key is 60, pitch is 60
			//   <nothing> -- OK, key and pitch from before
			//   K200 with P60 ok, pitch is 60
			//   K200 with neither P60 nor C4 uses 
			//       pitch from before
			// figure out what the key/instance is:
			if (new_key_flag) { // it was directly specified
				key = new_key;
				if (key < 128 && new_note_flag) {
					parse_error("", 0, "Pitch specified twice");
				}
			} else if (new_note_flag) { // "A"-"G" used
				key = new_note;
			}
			if (new_pitch_flag) {
				pitch = new_pitch;
			} else if (key < 128) {
				pitch = key;
			}
			// now we've acquired new parameters
			// if (it is a note, then enter the note
			if (valid) {
				// change tempo or beat
				attributes = process_attributes(attributes, time);
				// if there's a duration or pitch, make a note:
				if (new_pitch_flag || dur_flag || new_note_flag) {
					new_key_flag = false;
					new_pitch_flag = false;
					Alg_note_ptr note_ptr = new Alg_note;
					note_ptr->chan = voice;
					note_ptr->time = time;
					note_ptr->dur = dur;
					note_ptr->set_identifier(key);
					note_ptr->pitch = pitch;
					note_ptr->loud = loud;
					note_ptr->parameters = attributes;
					seq->add_event(note_ptr, track_num); // sort later
				} else {
					int update_key = -1;
					// key or pitch must appear explicitly; otherwise
					//    update applies to channel
					if (new_key_flag || new_pitch_flag) {
						update_key = key;
					}
					if (loud_flag) {
						Alg_update_ptr new_upd = new Alg_update;
						new_upd->chan = voice;
						new_upd->time = time;
						new_upd->set_identifier(update_key);
						new_upd->parameter.set_attr(symbol_table.insert_string("loudr"));
						new_upd->parameter.r = pitch;
						seq->add_event(new_upd, track_num);
					}
					if (attributes) {
						while (attributes) {
							Alg_update_ptr new_upd = new Alg_update;
							new_upd->chan = voice;
							new_upd->time = time;
							new_upd->set_identifier(update_key);
							new_upd->parameter = attributes->parm;
							seq->add_event(new_upd, track_num);
							Alg_parameters_ptr p = attributes;
							attributes = attributes->next;
							p->parm.s = NULL; // so we don't delete the string
							delete p;
						}
					}
				}
				if (next_flag) {
					time = time + next;
				} else if (dur_flag) {
					time = time + dur;
				}
			}
		}
        readline();
    }
    //print "Finished reading score"
    if (!error_flag) {
        seq->convert_to_seconds(); // make sure format is correct
        // seq->notes.sort('event_greater_than');
    }
    // print "parse returns error_flag", error_flag
    return error_flag;
}


long Alg_reader::parse_chan(char *field)
{
	char *int_string = field + 1;
	char *msg = "Integer or - expected";
	char *p = int_string;
	char c;
	// check that all chars in int_string are digits or '-':
	while (c = *p++) {
		if (!isdigit(c) && c != '-') {
			parse_error(field, p - field - 1, msg);
			return 0;
		}
	}
	p--; // p now points to end-of-string character
	if (p - int_string == 0) {
		// bad: string length is zero
		parse_error(field, 1, msg);
		return 0;
	}
	if (p - int_string == 1 && int_string[0] == '-') {
		// special case: entire string is "-", interpret as -1
		return -1;
	}
	return atoi(int_string);
}


long Alg_reader::parse_int(char *field)
{
    char *int_string = field + 1;
    char *msg = "Integer expected";
    char *p = int_string;
    char c;
	// check that all chars in int_string are digits:
    while (c = *p++) {
        if (!isdigit(c)) {
            parse_error(field, p - field - 1, msg);
            return 0;
        }
    }
	p--; // p now points to end-of-string character
	if (p - int_string == 0) {
		// bad: string length is zero
        parse_error(field, 1, msg);
        return 0;
    }
    return atoi(int_string);
}


int Alg_reader::find_real_in(char *field, int n)
{
    // scans from offset n to the end of a real constant
    bool decimal = false;
    int len = strlen(field);
    for (int i = n; i < len; i++) {
        char c = field[i];
        if (!isdigit(c)) {
            if (c == '.' && !decimal) {
                decimal = true;
            } else {
                return i;
            }
        }
    }
    return strlen(field);
}


double Alg_reader::parse_real(char *field)
{
    char real_string[80];
    char *msg = "Real expected";
    bool decimal = false;
    int last = find_real_in(field, 1);
    subseq(real_string, field, 1, last);
    if (last <= 1 || last < (int) strlen(field)) {
       parse_error(field, 1, msg);
       return 0;
    }
    return atof(real_string);
}


void Alg_reader::parse_error(char *field, long offset, char *message)
{
    int position = line_parser.pos - strlen(field) + offset;
    error_flag = true;
    puts(line_parser.string);
    for (int i = 0; i < position; i++) {
        putc(' ', stdout);
    }
    putc('^', stdout);
    printf("    %s\n", message);
}


double duration_lookup[] = { 0.25, 0.5, 1.0, 2.0, 4.0 };


double Alg_reader::parse_dur(char *field, double base)
{
    char *msg = "Duration expected";
    char real_string[80];
    char *durs = "SIQHW";
    char *p;
    int last;
    double dur;
    if (strlen(field) < 2) {
        // fall through to error message
        return -1;
    } else if (isdigit(field[1])) {
        last = find_real_in(field, 1);
        subseq(real_string, field, 1, last);
        dur = atof(real_string);
        // convert dur from seconds to beats
        dur = seq->get_time_map()->time_to_beat(base + dur) - 
			  seq->get_time_map()->time_to_beat(base);
    } else if (p = strchr(durs, field[1])) {
        dur = duration_lookup[p - durs];
        last = 2;
    } else {
        parse_error(field, 1, msg);
        return 0;
    }
    dur = parse_after_dur(dur, field, last, base);
    dur = seq->get_time_map()->beat_to_time(
		      seq->get_time_map()->time_to_beat(base) + dur) - base;
    return dur;
}


double Alg_reader::parse_after_dur(double dur, char *field, int n, double base)
{
    char a_string[80];
    if ((int) strlen(field) == n) {
        return dur;
    }
    if (field[n] == 'T') {
        return parse_after_dur(dur * 2/3, field, n + 1, base);
    }
    if (field[n] == '.') {
        return parse_after_dur(dur * 1.5, field, n + 1, base);
    }
    if (isdigit(field[n])) {
        int last = find_real_in(field, n);
        subseq(a_string, field, n, last);
        double f = atof(a_string);
        return parse_after_dur(dur * f, field, last, base);
    }
    if (field[n] == '+') {
        subseq(a_string, field, n + 1, -1);
        return dur + parse_dur(
			    a_string, seq->get_time_map()->beat_to_time(
                              seq->get_time_map()->time_to_beat(base) + dur));
    }
    parse_error(field, n, "Unexpected character in duration");
    return dur;
}

struct loud_lookup_struct {
    char *str;
    int val;
} loud_lookup[] = { {"FFF", 127}, {"FF", 120}, {"F", 110}, {"MF", 100}, 
                    {"MP", 90}, {"P", 80}, {"PP", 70}, {"PPP", 60}, 
                    {NULL, 0} };


double Alg_reader::parse_loud(char *field)
{
    char *msg = "Loudness expected";
    if (isdigit(field[1])) {
        return parse_int(field);
    } else {
        double loud = 0.0;
        char dyn[field_max];
        strcpy(dyn, field + 1);
        char *p = dyn;
        while (*p) {
            if (islower(*p)) *p = toupper(*p);
	    p++;
        }
        for (int i = 0; loud_lookup[i].str; i++) {
            if (streql(loud_lookup[i].str, dyn)) {
                return (double) loud_lookup[i].val;
            }
        }
    }
    parse_error(field, 1, msg);
    return 100.0;
}


int key_lookup[] = {21, 23, 12, 14, 16, 17, 19};

long Alg_reader::parse_key(char *field)
{
    char *msg = "Pitch expected";
    char *pitches = "ABCDEFG";
    char *p;
    if (isdigit(field[1])) {
        return parse_int(field);
    } else if (p = strchr(pitches, field[1])) {
        long key = key_lookup[p - pitches];
        key = parse_after_key(key, field, 2);
        return key;
    }
    parse_error(field, 1, msg);
    return 0;
}


long Alg_reader::parse_after_key(int key, char *field, int n)
{
    char octave[20];
    if ((int) strlen(field) == n) {
        return key;
    }
    char c = toupper(field[n]);
    if (c == 'S') {
        return parse_after_key(key + 1, field, n + 1);
    }
    if (c == 'F') {
        return parse_after_key(key - 1, field, n + 1);
    }
    if (isdigit(c)) {
        int last = find_int_in(field, n);
        subseq(octave, field, n, last);
        int oct = atoi(octave);
        return parse_after_key(key + oct * 12, field, last);
    }
    parse_error(field, n, "Unexpected character in pitch");
    return key;
}


long Alg_reader::find_int_in(char *field, int n)
{
    while ((int) strlen(field) > n && isdigit(field[n])) {
        n = n + 1;
    }
    return n;
}


bool Alg_reader::parse_attribute(char *field, Alg_parameter_ptr param)
{
    int i = 1;
    while (i < (int) strlen(field)) {
        if (field[i] == ':') {
            char attr[80];
            subseq(attr, field, 1, i);
            char type_char = field[i - 1];
            if (strchr("iarsl", type_char)) {
                param->set_attr(symbol_table.insert_string(attr));
                parse_val(param, field, i + 1);
            } else {
				parse_error(field, 0, "attribute needs to end with typecode: i,a,r,s, or l");
			}
            return !error_flag;
        }
        i = i + 1;
    }
    return false;
}


bool Alg_reader::parse_val(Alg_parameter_ptr param, char *s, int i)
{
    int len = (int) strlen(s);
    if (i >= len) {
        return false;
    }
    if (s[i] == '"') {
        if (!check_type('s', param)) {
            return false;
        }
        char *r = new char[len - i];
        subseq(r, s, i + 1, len - 1);
        param->s = r;
    } else if (s[i] == '\'') {
        if (!check_type('a', param)) {
            return false;
        }
        char r[80];
        subseq(r, s, i + 1, len - 1);
        param->a = symbol_table.insert_string(r);
    } else if (param->attr_type() == 'l') {
        if (streql(s + i, "true") || streql(s + i, "t")) {
            param->l = true;
        } else if (streql(s + i, "false") || streql(s + i, "nil")) {
            param->l = false;
        } else return false;
    } else if (isdigit(s[i]) || s[i] == '-') {
        int pos = i + 1;
        bool period = false;
		int sign = 1;
		if (s[i] == '-') {
			sign = -1;
			pos = i + 2;
		}
        while (pos < len) {
            if (isdigit(s[pos])) {
                ;
            } else if (!period && s[pos] == '.') {
                period = true;
            } else {
                parse_error(s, pos, "Unexpected char in number");
                return false;
            }
            pos = pos + 1;
        }
        char r[80];
        subseq(r, s, i, len);
        if (period) {
            if (!check_type('r', param)) {
                return false;
            }
            param->r = atof(r);
        } else {
            if (param->attr_type() == 'r') {
                param->r = atoi(r);
            } else if (!check_type('i', param)) {
                return false;
            } else {
                param->i = sign * atoi(r);
            }
        }
    } else {
		parse_error(s, i, "invalid value");
		return false;
	}
    return true;
}


bool Alg_reader::check_type(char type_char, Alg_parameter_ptr param)
{
    return param->attr_type() == type_char;
}


//duration_lookup = {"S": 0.5, "I": 0.5, "Q": 1, "H": 2, "W": 4}
//key_lookup = {"C": 12, "D": 14, "E": 16, "F": 17, "G": 19, "A": 21, "B": 23}

/*
def test():
    reader = Alg_reader(open("data\\test.gro", "r"))
    reader.parse()
    score = reader->seq.notes
    print "score:", score
    reader = nil
*/


