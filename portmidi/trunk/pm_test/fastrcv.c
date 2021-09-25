/* fastrcv.c -- send many MIDI messages very fast. 
 *
 * This is a stress test created to explore reports of
 * pm_write() call blocking (forever) on Linux when
 * sending very dense MIDI sequences.
 *
 * Modified 8 Aug 2017 with -n to send expired timestamps
 * to test a theory about why Linux ALSA hangs in Audacity.
 *
 * Roger B. Dannenberg, Aug 2017
 */

#include "portmidi.h"
#include "porttime.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "assert.h"

#define INPUT_BUFFER_SIZE 1000  /* big to avoid losing any input */
#define DRIVER_INFO NULL
#define TIME_PROC ((PmTimeProcPtr) Pt_Time)
#define TIME_INFO NULL
#define TIME_START Pt_Start(1, 0, 0) /* timer started w/millisecond accuracy */

#define STRING_MAX 80 /* used for console input */
// need to get declaration for Sleep()
#ifdef WIN32
#include "windows.h"
#else
#include <unistd.h>
#define Sleep(n) usleep(n * 1000)
#endif


int deviceno = -9999;
int verbose = FALSE;

/* read a number from console */
/**/
int get_number(const char *prompt)
{
    char line[STRING_MAX];
    int n = 0, i;
    fputs(prompt, stdout);
    while (n != 1) {
        n = scanf("%d", &i);
        fgets(line, STRING_MAX, stdin);
    }
    return i;
}


void fastrcv_test()
{
    PmStream * midi;
    PmError status, length;
    PmEvent buffer[1];

    /* It is recommended to start timer before PortMidi */
    TIME_START;

    /* open output device */
    Pm_OpenInput(&midi, 
                 deviceno, 
                 DRIVER_INFO,
                 INPUT_BUFFER_SIZE, 
                 TIME_PROC,
                 TIME_INFO);
    printf("Midi Input opened.\n");

    /* wait a sec after printing previous line */
    PmTimestamp start = Pt_Time() + 1000;
    while (start > Pt_Time()) {
        Sleep(10);
    }

    /* every 10ms read all messages, keep counts */
    /* every 1000ms, print report */
    int msgcnt = 0;
    /* expect repeating sequence of 60 through 71, alternating on/off */
    int expected_pitch = 60;
    int expected_on = TRUE;
    int report_time = Pt_Time() + 1000;  /* report every 1s */
    while (TRUE) {
        PmTimestamp now = Pt_Time();
        status = Pm_Poll(midi);
        if (status == TRUE) {
            length = Pm_Read(midi, buffer, 1);
            if (length > 0) {
                int status = Pm_MessageStatus(buffer[0].message);
                if (status == 0x80) {  /* convert NoteOff to NoteOn, vel=0 */
                    status = 0x90;
                    buffer[0].message = Pm_Message(status, 
                            Pm_MessageData1(buffer[0].message), 0);
                }
                /* only listen to NOTEON messages */
                if (status == 0x90) {
                    int pitch = Pm_MessageData1(buffer[0].message);
                    int vel = Pm_MessageData2(buffer[0].message);
                    int is_on = (vel > 0);
                    if (verbose) {
                        printf("Note pitch %d vel %d\n", pitch, vel);
                    }
                    msgcnt++;
                    if (pitch != expected_pitch || expected_on != is_on) {
                        printf("Unexpected note-on: pitch %d vel %d, "
                               "expected: pitch %d Note%s\n", pitch, vel,
                               expected_pitch, (expected_on ? "On" : "Off"));
                    }
                    if (is_on) {
                        expected_on = FALSE;
                        expected_pitch = pitch;
                    } else {
                        expected_on = TRUE;
                        expected_pitch = (pitch + 1) % 72;
                        if (expected_pitch < 60) expected_pitch = 60;
                    }
                }
            }
        }
        if (now >= report_time) {
            printf("%d msgs/sec\n", msgcnt);
            report_time += 1000;
            msgcnt = 0;
        }
    }
}


void show_usage()
{
    printf("Usage: fastrcv [-h] [-v] [-d device], where\n"
           "device is the PortMidi device number,\n"
           "-h means help,\n"
           "-v means verbose (print messages)\n");
}

int main(int argc, char *argv[])
{
    int default_in;
    int default_out;
    int i = 0, n = 0;
    int test_input = 0, test_output = 0, test_both = 0, somethingStupid = 0;
    int stream_test = 0;
    int device_valid = FALSE;
    
    if (sizeof(void *) == 8) 
        printf("Apparently this is a 64-bit machine.\n");
    else if (sizeof(void *) == 4) 
        printf ("Apparently this is a 32-bit machine.\n");
    
    if (argc <= 1) {
        show_usage();
    } else {
        for (i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-h") == 0) {
                show_usage();
            } else if (strcmp(argv[i], "-v") == 0) {
                verbose = TRUE;
            } else if (strcmp(argv[i], "-d") == 0) {
                i = i + 1;
                deviceno = atoi(argv[i]);
                printf("Device will be %d\n", deviceno);
            } else {
                show_usage();
            }
        }
    }

    /* list device information */
    default_in = Pm_GetDefaultInputDeviceID();
    default_out = Pm_GetDefaultOutputDeviceID();
    for (i = 0; i < Pm_CountDevices(); i++) {
        char *deflt;
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        if (!info->output) {
            printf("%d: %s, %s", i, info->interf, info->name);
            if (i == deviceno) {
                device_valid = TRUE;
                deflt = "selected ";
            } else if (i == default_out) {
                deflt = "default ";
            } else {
                deflt = "";
            }                      
            printf(" (%sinput)", deflt);
            printf("\n");
        }
    }
    
    if (!device_valid) {
        deviceno = get_number("Input device number: ");
    }

    fastrcv_test();
    return 0;
}
