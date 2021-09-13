/* sendvirtual.c -- test for creating a virtual device and sending to it */
/*
 * Roger B. Dannenberg
 * Sep 2021
 */
#include "portmidi.h"
#include "porttime.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "assert.h"

#define OUTPUT_BUFFER_SIZE 0
#define DRIVER_INFO NULL
#define TIME_PROC ((int32_t (*)(void *)) Pt_Time)
#define TIME_INFO NULL
#define TIME_START Pt_Start(1, 0, 0) /* timer started w/millisecond accuracy */

#define STRING_MAX 80 /* used for console input */

int latency = 0;

static void prompt_and_exit(void)
{
    char line[STRING_MAX];
    printf("type ENTER...");
    fgets(line, STRING_MAX, stdin);
    /* this will clean up open ports: */
    exit(-1);
}


static PmError checkerror(PmError err)
{
    if (err == pmHostError) {
        /* it seems pointless to allocate memory and copy the string,
         * so I will do the work of Pm_GetHostErrorText directly
         */
        char errmsg[80];
        Pm_GetHostErrorText(errmsg, 80);
        printf("PortMidi found host error...\n  %s\n", errmsg);
        prompt_and_exit();
    } else if (err < 0) {
        printf("PortMidi call failed...\n  %s\n", Pm_GetErrorText(err));
        prompt_and_exit();
    }
    return err;
}


void wait_until(PmTimestamp when)
{
    PtTimestamp now = TIME_PROC(TIME_INFO);
    if (when > now) {
        Pt_Sleep(when - now);
    }
}


void main_test_output(int num)
{
    PmStream *midi;
    int32_t next_time;
    PmEvent buffer[1];
    PmTimestamp timestamp;
    int pitch = 60;
    char line[STRING_MAX];

    /* It is recommended to start timer before Midi; otherwise, PortMidi may
       start the timer with its (default) parameters
     */
    TIME_START;

    /* create a virtual output device */
    checkerror(Pm_CreateVirtualOutput(&midi, "portmidi", NULL, DRIVER_INFO, 
                         OUTPUT_BUFFER_SIZE, TIME_PROC, TIME_INFO, latency));

    printf("Midi Output Virtual Device \"portmidi\" created.\n");
    printf("Type ENTER to send messages: ");
    fgets(line, STRING_MAX, stdin);

    buffer[0].timestamp = TIME_PROC(TIME_INFO);
#define PROGRAM 0
    buffer[0].message = Pm_Message(0xC0, PROGRAM, 0);
    Pm_Write(midi, buffer, 1);
    next_time = TIME_PROC(TIME_INFO) + 1000;  /* wait 1s */
    while (num > 0) {
        wait_until(next_time);
        Pm_WriteShort(midi, next_time, Pm_Message(0x90, pitch, 100));
        printf("Note On pitch %d\n", pitch);
        num--;
        next_time += 500;

        wait_until(next_time);
        Pm_WriteShort(midi, next_time, Pm_Message(0x90, pitch, 100));
        printf("Note Off pitch %d\n", pitch);
        num--;
        pitch = (pitch + 1) % 12 + 60;
        next_time += 500;
    }

    /* close device (this not explicitly needed in most implementations) */
    printf("ready to close...");

    Pm_Close(midi);
    printf("done closing...");
}


void show_usage()
{
    printf("Usage: sendvirtual [-h] [-l latency-in-ms] [n]\n"
           "    -h for this message,\n"
           "    -l ms designates latency for precise timing (default 0),\n"
           "    n is number of message to send.\n"
           "sends change program to 1, then one note per second with 0.5s on,\n"
           "0.5s off, for n/2 seconds. Latency >0 uses the device driver for \n"
           "precise timing (see PortMidi documentation).\n"); 
    exit(0);
}


int main(int argc, char *argv[])
{
    char line[STRING_MAX];
    int num = 10;
    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            show_usage();
        } else if (strcmp(argv[i], "-l") == 0 && (i + 1 < argc)) {
            i = i + 1;
            latency = atoi(argv[i]);
            printf("Latency will be %d\n", latency);
        } else {
            num = atoi(argv[1]);
            if (num <= 0) {
                show_usage();
            }
            printf("Sending %d messages.\n", num);
        }
    }

    main_test_output(num);
    
    printf("finished sendvirtual test...type ENTER to quit...");
    fgets(line, STRING_MAX, stdin);
    return 0;
}
