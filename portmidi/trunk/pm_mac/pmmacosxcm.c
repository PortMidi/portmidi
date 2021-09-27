/*
 * Platform interface to the MacOS X CoreMIDI framework
 * 
 * Jon Parise <jparise at cmu.edu>
 * and subsequent work by Andrew Zeldis and Zico Kolter
 * and Roger B. Dannenberg
 *
 * $Id: pmmacosx.c,v 1.17 2002/01/27 02:40:40 jon Exp $
 */
 
/* Notes:
    since the input and output streams are represented by MIDIEndpointRef
    values and almost no other state, we store the MIDIEndpointRef on
    descriptors[midi->device_id].descriptor. The only other state we need
    is for errors: we need to know if there is an error and if so, what is
    the error text. We use a structure with two kinds of
    host error: "error" and "callback_error". That way, asynchronous callbacks
    do not interfere with other error information.
    
    OS X does not seem to have an error-code-to-text function, so we will
    just use text messages instead of error codes.
 */

#include <stdlib.h>

//#define CM_DEBUG 1

#include "portmidi.h"
#include "pmutil.h"
#include "pminternal.h"
#include "porttime.h"
#include "pmmac.h"
#include "pmmacosxcm.h"

#include <stdio.h>
#include <string.h>

#include <CoreServices/CoreServices.h>
#include <CoreMIDI/MIDIServices.h>
#include <CoreAudio/HostTime.h>
#include <unistd.h>

#define PACKET_BUFFER_SIZE 1024
/* maximum overall data rate (OS X limits MIDI rate in case there
 * is a cycle among IAC ports.
 */
#define MAX_BYTES_PER_S 14000

/* Apple reports that packets are dropped when the MIDI bytes/sec
   exceeds 15000. This is computed by "tracking the number of MIDI
   bytes scheduled into 1-second buckets over the last six seconds and
   averaging these counts." This is confirmed in recent measurements
   (2021) with pm_test/fast.c and pm_test/fastrcv.c test programs show
   problems begin above 4900 (3 bytes each) messages/second. We use
   14000 rather than 15000 to avoid getting too close to the limit.
 
   Experiments show this restriction applies to IAC bus MIDI, but not
   to hardware interfaces. (I measured 0.5 Mbps each way over USB to a
   Teensy 3.2 microcontroller implementing a USB MIDI loopback. Maybe
   it would get 1 Mbps one-way, which would make the CoreMIDI
   restriction 8x slower than USB. Maybe other USB MIDI
   implementations are faster -- USB top speed for other protocols is
   certainly higher than 1 Mbps!)

   This is apparently based on timestamps, not on real time, so we
   have to avoid constructing packets that schedule high speed output
   regardless of when writes occur. The solution is to alter
   timestamps to limit data rates.  This adds a slight time
   distortion, e.g. an 11 note chord with all notes on the same
   timestamp will be altered so that the last message is delayed by 30
   bytes/14000 bps = 2.1ms. Note that this is about 5x MIDI speed, but
   at least 4x slower than USB MIDI.
 
   Altering timestamps creates another problem, which is that a sender
   that exceeds the maximum rate can queue up an unbounded number of
   messages. With non-USB MIDI devices, you could be writing 5x faster
   to CoreMIDI than the hardware interface can send, causing an
   unbounded backlog. There seems to be no general solution, so
   PortMidi does not guarantee delivery if, over the long run, you
   write faster than the hardware can send.
   
   The LIMIT_RATE symbol, if defined (which is the default), enables
   code to modify timestamps for output to an IAC device as follows:

     Before a packet is formed, the message timestamp is set to the
     maximum of the PortMidi timestamp (converted to CoreMIDI time)
     and min_next_time. After each send, min_next_time is updated to
     the packet time + packet length * delay_per_byte, which limits
     the scheduled bytes-per-second. Also, after each packet list
     flush, min_next_time is updated to the maximum of min_next_time
     and the real time, which prevents many bytes to be scheduled in
     the past. (We could more directly just say packets are never
     scheduled in the past, but we prefer to get the current time -- a
     system call -- only when we perform the more expensive operation
     of flushing packets, so that's when we update min_next_time to
     the current real time. If we are sending a lot, we have to flush
     a lot, so the time will be updated frequently when it matters.)

     This possible adjustment to timestamps can distort accurate
     timestamps by up to 214 us per 3-byte MIDI message.
 
   Nothing blocks the sender from queueing up an arbitrary number of
   messages. Timestamps should be used for accurate timing by sending
   timestamped messages a little ahead of real time, not for
   scheduling an entire MIDI sequence at once!
 */
#define LIMIT_RATE 1

#define SYSEX_BUFFER_SIZE 128
/* What is the maximum PortMidi device number for an IAC device? A
 * cleaner design would be to not use the endpoint as our device
 * representation. Instead, we could have a private extensible struct
 * to keep all device information, including whether the device is
 * implemented with the AppleMIDIIACDriver, which we need because we
 * have to limit the data rate to this particular driver to avoid
 * dropping messages. Rather than rewrite a lot of code, I am just
 * allocating 64 bytes to flag which devices are IAC ones. If an IAC
 * device number is greater than 63, PortMidi will fail to limit
 * writes to it, but will not complain and will not access memory
 * outside the 64-element array of char.
 */
#define MAX_IAC_NUM 63

#define VERBOSE_ON 1
#define VERBOSE if (VERBOSE_ON)

#define MIDI_SYSEX       0xf0
#define MIDI_EOX         0xf7
#define MIDI_CLOCK       0xf8
#define MIDI_STATUS_MASK 0x80

// "Ref"s are pointers on 32-bit machines and ints on 64 bit machines
// NULL_REF is our representation of either 0 or NULL
#ifdef __LP64__
#define NULL_REF 0
#else
#define NULL_REF NULL
#endif

static MIDIClientRef	client = NULL_REF; 	/* Client handle to the MIDI server */
static MIDIPortRef	portIn = NULL_REF;	/* Input port handle */
static MIDIPortRef	portOut = NULL_REF;	/* Output port handle */
static char             isIAC[MAX_IAC_NUM + 1]; /* is device an IAC device */

extern pm_fns_node pm_macosx_in_dictionary;
extern pm_fns_node pm_macosx_out_dictionary;

typedef struct coremidi_info_struct {
    int is_virtual;     /* virtual device (TRUE) or actual device (FALSE)? */
    PmTimestamp sync_time; /* when did we last determine delta? */
    UInt64 delta;	/* difference between stream time and real time in ns */
    int first_message;  /* tells midi_write to sychronize timestamps */
    int sysex_mode;     /* middle of sending sysex */
    uint32_t sysex_word; /* accumulate data when receiving sysex */
    uint32_t sysex_byte_count; /* count how many received */
    char error[PM_HOST_ERROR_MSG_LEN];
    char callback_error[PM_HOST_ERROR_MSG_LEN];
    Byte packetBuffer[PACKET_BUFFER_SIZE];
    MIDIPacketList *packetList; /* a pointer to packetBuffer */
    MIDIPacket *packet;
    Byte sysex_buffer[SYSEX_BUFFER_SIZE]; /* temp storage for sysex data */
    MIDITimeStamp sysex_timestamp; /* timestamp to use with sysex data */
    /* allow for running status (is running status possible here? -rbd): -cpr */
    unsigned char last_command; 
    int32_t last_msg_length;
    UInt64 min_next_time; /* when can the next send take place? */
    int isIACdevice;
    Float64 us_per_host_tick; /* host clock frequency, units of min_next_time */
    UInt64 host_ticks_per_byte; /* host clock units per byte at maximum rate */
} coremidi_info_node, *coremidi_info_type;

/* private function declarations */
MIDITimeStamp timestamp_pm_to_cm(PmTimestamp timestamp);
PmTimestamp timestamp_cm_to_pm(MIDITimeStamp timestamp);

char* cm_get_full_endpoint_name(MIDIEndpointRef endpoint, int *isIAC);


static int midi_length(int32_t msg)
{
    int status, high, low;
    static int high_lengths[] = {
        1, 1, 1, 1, 1, 1, 1, 1,         /* 0x00 through 0x70 */
        3, 3, 3, 3, 2, 2, 3, 1          /* 0x80 through 0xf0 */
    };
    static int low_lengths[] = {
        1, 2, 3, 2, 1, 1, 1, 1,         /* 0xf0 through 0xf8 */
        1, 1, 1, 1, 1, 1, 1, 1          /* 0xf9 through 0xff */
    };

    status = msg & 0xFF;
    high = status >> 4;
    low = status & 15;

    return (high != 0xF) ? high_lengths[high] : low_lengths[low];
}

static PmTimestamp midi_synchronize(PmInternal *midi)
{
    coremidi_info_type info = (coremidi_info_type) midi->api_info;
    UInt64 pm_stream_time_2 = 
            AudioConvertHostTimeToNanos(AudioGetCurrentHostTime());
    PmTimestamp real_time;
    UInt64 pm_stream_time;
    /* if latency is zero and this is an output, there is no 
       time reference and midi_synchronize should never be called */
    assert(midi->time_proc);
    assert(midi->is_input || midi->latency != 0);
    do {
         /* read real_time between two reads of stream time */
         pm_stream_time = pm_stream_time_2;
         real_time = (*midi->time_proc)(midi->time_info);
         pm_stream_time_2 = AudioConvertHostTimeToNanos(AudioGetCurrentHostTime());
         /* repeat if more than 0.5 ms has elapsed */
    } while (pm_stream_time_2 > pm_stream_time + 500000);
    info->delta = pm_stream_time - ((UInt64) real_time * (UInt64) 1000000);
    info->sync_time = real_time;
    return real_time;
}


static void process_packet(MIDIPacket *packet, PmEvent *event, 
                           PmInternal *midi, coremidi_info_type info)
{
    /* handle a packet of MIDI messages from CoreMIDI */
    /* there may be multiple short messages in one packet (!) */
    unsigned int remaining_length = packet->length;
    unsigned char *cur_packet_data = packet->data;
    while (remaining_length > 0) {
        if (cur_packet_data[0] == MIDI_SYSEX ||
            /* are we in the middle of a sysex message? */
            (info->last_command == 0 &&
             !(cur_packet_data[0] & MIDI_STATUS_MASK))) {
            info->last_command = 0; /* no running status */
            unsigned int amt = pm_read_bytes(midi, cur_packet_data, 
                                             remaining_length, 
                                             event->timestamp);
            remaining_length -= amt;
            cur_packet_data += amt;
        } else if (cur_packet_data[0] == MIDI_EOX) {
            /* this should never happen, because pm_read_bytes should
             * get and read all EOX bytes*/
            midi->sysex_in_progress = FALSE;
            info->last_command = 0;
        } else if (cur_packet_data[0] & MIDI_STATUS_MASK) {
            /* compute the length of the next (short) msg in packet */
	    unsigned int cur_message_length = midi_length(cur_packet_data[0]);
            if (cur_message_length > remaining_length) {
#ifdef DEBUG
                printf("PortMidi debug msg: not enough data");
#endif
		/* since there's no more data, we're done */
		return;
	    }
            if (cur_packet_data[0] < MIDI_SYSEX) {
                /* channel messages set running status */
                info->last_command = cur_packet_data[0];
                info->last_msg_length = cur_message_length;
            } else if (cur_packet_data[0] < MIDI_CLOCK) {
                /* system messages clear running status */
                info->last_command = 0;
                info->last_msg_length = 0;
            }
	    switch (cur_message_length) {
	    case 1:
	        event->message = Pm_Message(cur_packet_data[0], 0, 0);
		break; 
	    case 2:
	        event->message = Pm_Message(cur_packet_data[0], 
					    cur_packet_data[1], 0);
		break;
	    case 3:
	        event->message = Pm_Message(cur_packet_data[0],
					    cur_packet_data[1], 
					    cur_packet_data[2]);
		break;
	    default:
                /* PortMIDI internal error; should never happen */
                assert(cur_message_length == 1);
	        return; /* give up on packet if continued after assert */
	    }
	    pm_read_short(midi, event);
	    remaining_length -= cur_message_length;
	    cur_packet_data += cur_message_length;
	} else if (info->last_msg_length > remaining_length + 1) {
	    /* we have running status, but not enough data */
#ifdef DEBUG
	    printf("PortMidi debug msg: not enough data in CoreMIDI packet");
#endif
	    /* since there's no more data, we're done */
	    return;
	} else { /* output message using running status */
	    switch (info->last_msg_length) {
	    case 1:
	        event->message = Pm_Message(info->last_command, 0, 0);
		break;
	    case 2:
	        event->message = Pm_Message(info->last_command,
					    cur_packet_data[0], 0);
		break;
	    case 3:
	        event->message = Pm_Message(info->last_command,
					    cur_packet_data[0], 
					    cur_packet_data[1]);
		break;
	    default:
	        /* last_msg_length is invalid -- internal PortMIDI error */
	        assert(info->last_msg_length == 1);
	    }
	    pm_read_short(midi, event);
	    remaining_length -= (info->last_msg_length - 1);
	    cur_packet_data += (info->last_msg_length - 1);
	}
    }
}


/* called when MIDI packets are received */
static void read_callback(const MIDIPacketList *newPackets, PmInternal *midi)
{
    coremidi_info_type info = (coremidi_info_type) midi->api_info;
    PmEvent event;
    MIDIPacket *packet;
    unsigned int packetIndex;
    uint32_t now;
    unsigned int status;
    
#ifdef CM_DEBUG
    printf("read_callback: numPackets %d: ", newPackets->numPackets);
#endif

    /* Retrieve the context for this connection */
    assert(info);
    
    /* synchronize time references every 100ms */
    now = (*midi->time_proc)(midi->time_info);
    if (info->first_message || info->sync_time + 100 /*ms*/ < now) {
        /* time to resync */
        now = midi_synchronize(midi);
        info->first_message = FALSE;
    }
    
    packet = (MIDIPacket *) &newPackets->packet[0];
    /* printf("read_callback packet status %x length %d\n", packet->data[0], 
               packet->length); */
    for (packetIndex = 0; packetIndex < newPackets->numPackets; packetIndex++) {
        /* Set the timestamp and dispatch this message */
        event.timestamp = (PmTimestamp) /* explicit conversion */ (
                (AudioConvertHostTimeToNanos(packet->timeStamp) - info->delta) /
                (UInt64) 1000000);
        status = packet->data[0];
        /* process packet as sysex data if it begins with MIDI_SYSEX, or
           MIDI_EOX or non-status byte with no running status */
#ifdef CM_DEBUG
        printf(" %d", packet->length);
#endif
        if (status == MIDI_SYSEX || status == MIDI_EOX || 
            ((!(status & MIDI_STATUS_MASK)) && !info->last_command)) {
	    /* previously was: !(status & MIDI_STATUS_MASK)) {
             * but this could mistake running status for sysex data
             */
            /* reset running status data -cpr */
	    info->last_command = 0;
	    info->last_msg_length = 0;
            /* printf("sysex packet length: %d\n", packet->length); */
            pm_read_bytes(midi, packet->data, packet->length, event.timestamp);
        } else {
            process_packet(packet, &event, midi, info);
	}
        packet = MIDIPacketNext(packet);
    }
#ifdef CM_DEBUG
    printf("\n");
#endif
}

/* callback for real devices - redirects to read_callback */
static void device_read_callback(const MIDIPacketList *newPackets, 
                                 void *refCon, void *connRefCon)
{
    read_callback(newPackets, (PmInternal *) connRefCon);
}


/* callback for virtual devices - redirects to read_callback */
static void virtual_read_callback(const MIDIPacketList *newPackets, 
                                  void *refCon, void *connRefCon)
{
    read_callback(newPackets, (PmInternal *) refCon);
}


/* allocate and initialize our internal coremidi connection info */
static coremidi_info_type create_macosxcm_info(int is_virtual, int is_input)
{
    coremidi_info_type info = (coremidi_info_type)
            pm_alloc(sizeof(coremidi_info_node));
    if (!info) {
        return NULL;
    }
    info->is_virtual = is_virtual;
    info->error[0] = 0;
    info->callback_error[0] = 0;
    info->sync_time = 0;
    info->delta = 0;
    info->first_message = TRUE;
    info->sysex_mode = FALSE;
    info->sysex_word = 0;
    info->sysex_byte_count = 0;
    info->packet = NULL;
    info->last_command = 0;
    info->last_msg_length = 0;
    info->min_next_time = 0;
    info->isIACdevice = FALSE;
    info->us_per_host_tick = 1000000.0 / AudioGetHostClockFrequency();
    info->host_ticks_per_byte =
            (UInt64) (1000000.0 / (info->us_per_host_tick * MAX_BYTES_PER_S));
    info->packetList = (is_input ? NULL :
                                   (MIDIPacketList *) info->packetBuffer);
    return info;
}



static PmError midi_in_open(PmInternal *midi, void *driverInfo)
{
    MIDIEndpointRef endpoint;
    coremidi_info_type info;
    OSStatus macHostError;
    
    /* insure that we have a time_proc for timing */
    if (midi->time_proc == NULL) {
        if (!Pt_Started()) 
            Pt_Start(1, 0, 0);
        /* time_get does not take a parameter, so coerce */
        midi->time_proc = (PmTimeProcPtr) Pt_Time;
    }
    endpoint = (MIDIEndpointRef) (long) descriptors[midi->device_id].descriptor;
    if (endpoint == NULL_REF) {
        return pmInvalidDeviceId;
    }

    info = create_macosxcm_info(FALSE, TRUE);
    midi->api_info = info;
    if (!info) {
        return pmInsufficientMemory;
    }
    macHostError = MIDIPortConnectSource(portIn, endpoint, midi);
    if (macHostError != noErr) {
        pm_hosterror = macHostError;
        sprintf(pm_hosterror_text, 
                "Host error %ld: MIDIPortConnectSource() in midi_in_open()",
                (long) macHostError);
        midi->api_info = NULL;
        pm_free(info);
        return pmHostError;
    }
    
    return pmNoError;
}

static PmError midi_in_close(PmInternal *midi)
{
    MIDIEndpointRef endpoint;
    OSStatus macHostError;
    PmError err = pmNoError;
    
    coremidi_info_type info = (coremidi_info_type) midi->api_info;
    
    if (!info) return pmBadPtr;

    endpoint = (MIDIEndpointRef) (long) descriptors[midi->device_id].descriptor;
    if (endpoint == NULL_REF) {
        pm_hosterror = pmBadPtr;
    }
    
    if (info->is_virtual) {
        macHostError = MIDIEndpointDispose(endpoint);
        if (macHostError != noErr) {
            pm_hosterror = macHostError;
            sprintf(pm_hosterror_text, "Host error %ld: "
                    "MIDIEndpointDispose() in midi_in_close()",
                    (long) macHostError);
            err = pmHostError;
        }
    } else {
        /* shut off the incoming messages before freeing data structures */
        macHostError = MIDIPortDisconnectSource(portIn, endpoint);
        /* If the source closes, you get paramErr == -50 here. It seems
         * possible to monitor changes like sources closing by getting
         * notifications ALL changes, but the CoreMIDI documentation is
         * really terrible overall, and it seems easier to just ignore
         * this host error.
         */
        if (macHostError != noErr && macHostError != -50) {
            pm_hosterror = macHostError;
            sprintf(pm_hosterror_text, "Host error %ld: "
                    "MIDIPortDisconnectSource() in midi_in_close()",
                    (long) macHostError);
            err = pmHostError;
        }
    }
    midi->api_info = NULL;
    pm_free(info);
    
    return err;
}


static PmError midi_out_open(PmInternal *midi, void *driverInfo)
{
    coremidi_info_type info;

    info = create_macosxcm_info(FALSE, FALSE);
    if (midi->device_id <= MAX_IAC_NUM) {
        info->isIACdevice = isIAC[midi->device_id];
#ifdef CM_DEBUG
        printf("midi_out_open isIACdevice %d\n", info->isIACdevice);
#endif
    }
    midi->api_info = info;
    if (!info) {
        return pmInsufficientMemory;
    }
    return pmNoError;
}


static PmError midi_out_close(PmInternal *midi)
{
    coremidi_info_type m = (coremidi_info_type) midi->api_info;
    if (!m) return pmBadPtr;
    
    midi->api_info = NULL;
    pm_free(midi->api_info);
    
    return pmNoError;
}


static PmError midi_create_virtual(struct pm_internal_struct *midi,
                  int is_input, const char *name, void *driverInfo)
{
    OSStatus macHostError;
    MIDIEndpointRef endpoint;
    CFStringRef nameRef = CFStringCreateWithCString(NULL, name,
                                                    kCFStringEncodingASCII);
    coremidi_info_type info;
    int id;

    /* insure that we have a time_proc for timing */
    if (midi->time_proc == NULL) {
        if (!Pt_Started())
            Pt_Start(1, 0, 0);
        /* time_get does not take a parameter, so coerce */
        midi->time_proc = (PmTimeProcPtr) Pt_Time;
    }
    info = create_macosxcm_info(TRUE, is_input);
    midi->api_info = info;
    if (!info) {
        return pmInsufficientMemory;
    }
    if (is_input) {
        macHostError = MIDIDestinationCreate(client, nameRef, 
                               virtual_read_callback, midi, &endpoint);
    } else {
        macHostError = MIDISourceCreate(client, nameRef, &endpoint);
        /* based on experiments, virtual outputs are subject to the
         * same rate limiting as IAC Bus ports. Setting isIACdevice
         * turns on rate limiting for this output port. Note that
         * virtual inputs are not rate limited.
         */
        info->isIACdevice = TRUE;
    }
    CFRelease(nameRef);

    if (macHostError != noErr) {
        pm_hosterror = macHostError;
        sprintf(pm_hosterror_text, "Host error %ld: %s() in "
                "midi_create_virtual()", (long) macHostError, 
                (is_input ? "MIDIDestinationCreate" : "MIDISourceCreate"));
        pm_free(info);
        return pmHostError;
    }
    id = pm_add_device("CoreMIDI", name, is_input, endpoint, 
                       (is_input ? &pm_macosx_in_dictionary :
                                   &pm_macosx_out_dictionary));
    if (id < 0) {  /* error -- out of memory? */
        pm_free(info);
        MIDIEndpointDispose(endpoint);
        return id;
    }
    midi->dictionary = descriptors[id].dictionary;;
    midi->device_id = id;
    return id;
  }


static PmError midi_abort(PmInternal *midi)
{
    PmError err = pmNoError;
    OSStatus macHostError;
    MIDIEndpointRef endpoint =
            (MIDIEndpointRef) (long) descriptors[midi->device_id].descriptor;
    macHostError = MIDIFlushOutput(endpoint);
    if (macHostError != noErr) {
        pm_hosterror = macHostError;
        sprintf(pm_hosterror_text,
                "Host error %ld: MIDIFlushOutput()", (long) macHostError);
        err = pmHostError;
    }
    return err;
}


static PmError midi_write_flush(PmInternal *midi, PmTimestamp timestamp)
{
    OSStatus macHostError;
    coremidi_info_type info = (coremidi_info_type) midi->api_info;
    MIDIEndpointRef endpoint = 
            (MIDIEndpointRef) (long) descriptors[midi->device_id].descriptor;
    assert(info);
    assert(endpoint);
    if (info->packet != NULL) {
        /* out of space, send the buffer and start refilling it */
        /* update min_next_time each flush to support rate limit */
        UInt64 now = AudioGetCurrentHostTime();
        if (now > info->min_next_time) 
            info->min_next_time = now;
        if (info->is_virtual) {
            macHostError = MIDIReceived(endpoint, info->packetList);
        } else {
            macHostError = MIDISend(portOut, endpoint, info->packetList);
        }
        info->packet = NULL; /* indicate no data in packetList now */
        if (macHostError != noErr) goto send_packet_error;
    }
    return pmNoError;
    
send_packet_error:
    pm_hosterror = macHostError;
    sprintf(pm_hosterror_text, "Host error %ld: %s() in midi_write()", 
            (long) macHostError,
            (info->is_virtual ? "MIDIReceived" : "MIDISend"));
    return pmHostError;

}


static PmError send_packet(PmInternal *midi, Byte *message,
                       unsigned int messageLength, MIDITimeStamp timestamp)
{
    PmError err;
    coremidi_info_type info = (coremidi_info_type) midi->api_info;
    assert(info);
    
    /* printf("add %d to packet %p len %d\n",
              message[0], info->packet, messageLength); */
    info->packet = MIDIPacketListAdd(info->packetList,
                                     sizeof(info->packetBuffer), info->packet,
                                     timestamp, messageLength, message);
#if LIMIT_SEND_RATE
    info->byte_count += messageLength;
#endif
    if (info->packet == NULL) {
        /* out of space, send the buffer and start refilling it */
        /* make midi->packet non-null to fool midi_write_flush into sending */
        info->packet = (MIDIPacket *) 4;
        /* timestamp is 0 because midi_write_flush ignores timestamp since
         * timestamps are already in packets. The timestamp parameter is here
         * because other API's need it. midi_write_flush can be called 
         * from system-independent code that must be cross-API.
         */
        if ((err = midi_write_flush(midi, 0)) != pmNoError) return err;
        info->packet = MIDIPacketListInit(info->packetList);
        assert(info->packet); /* if this fails, it's a programming error */
        info->packet = MIDIPacketListAdd(info->packetList,
                               sizeof(info->packetBuffer), info->packet,
                               timestamp, messageLength, message);
        assert(info->packet); /* can't run out of space on first message */
    }
    return pmNoError;
}    


static PmError midi_write_short(PmInternal *midi, PmEvent *event)
{
    PmTimestamp when = event->timestamp;
    PmMessage what = event->message;
    MIDITimeStamp timestamp;
    UInt64 when_ns;
    coremidi_info_type info = (coremidi_info_type) midi->api_info;
    Byte message[4];
    unsigned int messageLength;

    if (info->packet == NULL) {
        info->packet = MIDIPacketListInit(info->packetList);
        /* this can never fail, right? failure would indicate something 
           unrecoverable */
        assert(info->packet);
    }
    
    /* compute timestamp */
    if (when == 0) when = midi->now;
    /* if latency == 0, midi->now is not valid. We will just set it to zero */
    if (midi->latency == 0) when = 0;
    when_ns = ((UInt64) (when + midi->latency) * (UInt64) 1000000) +
              info->delta;
    timestamp = (MIDITimeStamp) AudioConvertNanosToHostTime(when_ns);

    message[0] = Pm_MessageStatus(what);
    message[1] = Pm_MessageData1(what);
    message[2] = Pm_MessageData2(what);
    messageLength = midi_length(what);

#ifdef LIMIT_RATE
    /* make sure we go forward in time */
    if (timestamp < info->min_next_time) 
        timestamp = info->min_next_time;

    if (info->isIACdevice) {
        info->min_next_time = timestamp + messageLength * 
                                          info->host_ticks_per_byte;
    }
#endif
    /* Add this message to the packet list */
    return send_packet(midi, message, messageLength, timestamp);
}


static PmError midi_begin_sysex(PmInternal *midi, PmTimestamp when)
{
    UInt64 when_ns;
    coremidi_info_type info = (coremidi_info_type) midi->api_info;
    assert(info);
    info->sysex_byte_count = 0;
    
    /* compute timestamp */
    if (when == 0) when = midi->now;
    /* if latency == 0, midi->now is not valid. We will just set it to zero */
    if (midi->latency == 0) when = 0;
    when_ns = ((UInt64) (when + midi->latency) * (UInt64) 1000000) +
              info->delta;
    info->sysex_timestamp =
              (MIDITimeStamp) AudioConvertNanosToHostTime(when_ns);

    if (info->packet == NULL) {
        info->packet = MIDIPacketListInit(info->packetList);
        /* this can never fail, right? failure would indicate something 
           unrecoverable */
        assert(info->packet);
    }
    return pmNoError;
}


static PmError midi_end_sysex(PmInternal *midi, PmTimestamp when)
{
    PmError err;
    coremidi_info_type info = (coremidi_info_type) midi->api_info;
    assert(info);
    
#ifdef LIMIT_RATE
    /* make sure we go foreward in time */
    if (info->sysex_timestamp < info->min_next_time)
        info->sysex_timestamp = info->min_next_time;

    if (info->isIACdevice) {
        info->min_next_time = info->sysex_timestamp + info->sysex_byte_count *
                                                      info->host_ticks_per_byte;
    }
#endif
    
    /* now send what's in the buffer */
    err = send_packet(midi, info->sysex_buffer, info->sysex_byte_count,
                      info->sysex_timestamp);
    info->sysex_byte_count = 0;
    if (err != pmNoError) {
        info->packet = NULL; /* flush everything in the packet list */
        return err;
    }
    return pmNoError;
}


static PmError midi_write_byte(PmInternal *midi, unsigned char byte, 
                               PmTimestamp timestamp)
{
    coremidi_info_type info = (coremidi_info_type) midi->api_info;
    assert(info);
    if (info->sysex_byte_count >= SYSEX_BUFFER_SIZE) {
        PmError err = midi_end_sysex(midi, timestamp);
        if (err != pmNoError) return err;
    }
    info->sysex_buffer[info->sysex_byte_count++] = byte;
    return pmNoError;
}


static PmError midi_write_realtime(PmInternal *midi, PmEvent *event)
{
    /* to send a realtime message during a sysex message, first
       flush all pending sysex bytes into packet list */
    PmError err = midi_end_sysex(midi, 0);
    if (err != pmNoError) return err;
    /* then we can just do a normal midi_write_short */
    return midi_write_short(midi, event);
}


static unsigned int midi_has_host_error(PmInternal *midi)
{
    coremidi_info_type info = (coremidi_info_type) midi->api_info;
    return (info->callback_error[0] != 0) || (info->error[0] != 0);
}


static void midi_get_host_error(PmInternal *midi, char *msg, unsigned int len)
{
    coremidi_info_type info = (coremidi_info_type) midi->api_info;
    msg[0] = 0; /* initialize to empty string */
    if (info) { /* make sure there is an open device to examine */
        if (info->error[0]) {
            strncpy(msg, info->error, len);
            info->error[0] = 0; /* clear the error */
        } else if (info->callback_error[0]) {
            strncpy(msg, info->callback_error, len);
            info->callback_error[0] = 0; /* clear the error */
        }
        msg[len - 1] = 0; /* make sure string is terminated */
    }
}


MIDITimeStamp timestamp_pm_to_cm(PmTimestamp timestamp)
{
    UInt64 nanos;
    if (timestamp <= 0) {
        return (MIDITimeStamp)0;
    } else {
        nanos = (UInt64)timestamp * (UInt64)1000000;
        return (MIDITimeStamp)AudioConvertNanosToHostTime(nanos);
    }
}


PmTimestamp timestamp_cm_to_pm(MIDITimeStamp timestamp)
{
    UInt64 nanos;
    nanos = AudioConvertHostTimeToNanos(timestamp);
    return (PmTimestamp)(nanos / (UInt64)1000000);
}


//
// Code taken from http://developer.apple.com/qa/qa2004/qa1374.html
//////////////////////////////////////
// Obtain the name of an endpoint without regard for whether it has connections.
// The result should be released by the caller.
CFStringRef EndpointName(MIDIEndpointRef endpoint, bool isExternal, int *isIAC)
{
    CFMutableStringRef result = CFStringCreateMutable(NULL, 0);
    CFStringRef str;
    *isIAC = FALSE;
  
    // begin with the endpoint's name
    str = NULL;
    MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &str);
    if (str != NULL) {
        CFStringAppend(result, str);
        CFRelease(str);
    }
  
    MIDIEntityRef entity = NULL_REF;
    MIDIEndpointGetEntity(endpoint, &entity);
    if (entity == NULL_REF)
        // probably virtual
        return result;
  
    if (!isExternal) { /* detect IAC devices */
        //extern const CFStringRef kMIDIPropertyDriverOwner;
        MIDIObjectGetStringProperty(entity, kMIDIPropertyDriverOwner, &str);
        if (str != NULL) {
            char s[32]; /* driver name may truncate, but that's OK */
            CFStringGetCString(str, s, 31, kCFStringEncodingUTF8);
            s[31] = 0;  /* make sure it is terminated just to be safe */
#ifdef CM_DEBUG
            printf("driver %s\n", s);
#endif
            *isIAC = (strcmp(s, "com.apple.AppleMIDIIACDriver") == 0);
        }
    }

    if (CFStringGetLength(result) == 0) {
        // endpoint name has zero length -- try the entity
        str = NULL;
        MIDIObjectGetStringProperty(entity, kMIDIPropertyName, &str);
        if (str != NULL) {
            CFStringAppend(result, str);
            CFRelease(str);
        }
    }
    // now consider the device's name
    MIDIDeviceRef device = NULL_REF;
    MIDIEntityGetDevice(entity, &device);
    if (device == NULL_REF)
        return result;
  
    str = NULL;
    MIDIObjectGetStringProperty(device, kMIDIPropertyName, &str);
    if (CFStringGetLength(result) == 0) {
        CFRelease(result);
        return str;
    }
    if (str != NULL) {
        // if an external device has only one entity, throw away
        // the endpoint name and just use the device name
        if (isExternal && MIDIDeviceGetNumberOfEntities(device) < 2) {
            CFRelease(result);
            return str;
        } else {
            if (CFStringGetLength(str) == 0) {
                CFRelease(str);
                return result;
            }
            // does the entity name already start with the device name?
            // (some drivers do this though they shouldn't)
            // if so, do not prepend
            if (CFStringCompareWithOptions(result, /* endpoint name */
                        str, /* device name */
                        CFRangeMake(0, CFStringGetLength(str)), 0) != 
                kCFCompareEqualTo) {
                // prepend the device name to the entity name
                if (CFStringGetLength(result) > 0)
                    CFStringInsert(result, 0, CFSTR(" "));
                CFStringInsert(result, 0, str);
            }
            CFRelease(str);
        }
    }
    // DEBUG:
    char cstr[80];
    CFStringGetCString(result, cstr, 80, kCFStringEncodingUTF8);
#ifdef CM_DEBUG
    printf("    EndpointName %s\n", cstr);
#endif
    return result;
}


// Obtain the name of an endpoint, following connections.
// The result should be released by the caller.
static CFStringRef ConnectedEndpointName(MIDIEndpointRef endpoint, int *isIAC)
{
    CFMutableStringRef result = CFStringCreateMutable(NULL, 0);
    CFStringRef str;
    OSStatus err;
    long i;
  
    // Does the endpoint have connections?
    CFDataRef connections = NULL;
    long nConnected = 0;
    bool anyStrings = false;
    err = MIDIObjectGetDataProperty(endpoint, kMIDIPropertyConnectionUniqueID,
                                    &connections);
    if (connections != NULL) {
        // It has connections, follow them
        // Concatenate the names of all connected devices
        nConnected = CFDataGetLength(connections) / 
                     (int32_t) sizeof(MIDIUniqueID);
        if (nConnected) {
            const SInt32 *pid = (const SInt32 *)(CFDataGetBytePtr(connections));
            for (i = 0; i < nConnected; ++i, ++pid) {
                MIDIUniqueID id = EndianS32_BtoN(*pid);
                MIDIObjectRef connObject;
                MIDIObjectType connObjectType;
                err = MIDIObjectFindByUniqueID(id, &connObject, 
                                               &connObjectType);
                if (err == noErr) {
                    if (connObjectType == kMIDIObjectType_ExternalSource  ||
                        connObjectType == kMIDIObjectType_ExternalDestination) {
                        // Connected to an external device's endpoint (>=10.3)
                        str = EndpointName((MIDIEndpointRef)(connObject), true,
                                           isIAC);
                    } else {
                        // Connected to an external device (10.2) 
                        // (or something else, catch-all)
                        str = NULL;
                        MIDIObjectGetStringProperty(connObject, 
                                                    kMIDIPropertyName, &str);
                    }
                    if (str != NULL) {
                        if (anyStrings)
                            CFStringAppend(result, CFSTR(", "));
                        else anyStrings = true;
                        CFStringAppend(result, str);
                        CFRelease(str);
                    }
                }
            }
        }
        CFRelease(connections);
    }
    if (anyStrings)
        return result; // caller should release result

    CFRelease(result);

    // Here, either the endpoint had no connections, or we failed to
    // obtain names for any of them.
    return EndpointName(endpoint, false, isIAC);
}


char *cm_get_full_endpoint_name(MIDIEndpointRef endpoint, int *isIAC)
{
    /* Thanks to Dan Wilcox for fixes for Unicode handling */
    CFStringRef fullName = ConnectedEndpointName(endpoint, isIAC);
    CFIndex utf16_len = CFStringGetLength(fullName) + 1;
    CFIndex max_byte_len = CFStringGetMaximumSizeForEncoding(
                                   utf16_len, kCFStringEncodingUTF8) + 1;
    char* pmname = (char *) pm_alloc(CFStringGetLength(fullName) + 1);

    /* copy the string into our buffer; note that there may be some wasted
       space, but the total waste is not large */
    CFStringGetCString(fullName, pmname, max_byte_len, kCFStringEncodingUTF8);

    /* clean up */
    if (fullName) CFRelease(fullName);

    return pmname;
}


pm_fns_node pm_macosx_in_dictionary = {
    none_write_short,
    none_sysex,
    none_sysex,
    none_write_byte,
    none_write_short,
    none_write_flush,
    none_synchronize,
    midi_in_open,
    midi_abort,
    midi_in_close,
    success_poll,
    midi_has_host_error,
    midi_get_host_error,
};

pm_fns_node pm_macosx_out_dictionary = {
    midi_write_short,
    midi_begin_sysex,
    midi_end_sysex,
    midi_write_byte,
    midi_write_realtime,
    midi_write_flush,
    midi_synchronize,
    midi_out_open,
    midi_abort,
    midi_out_close,
    success_poll,
    midi_has_host_error,
    midi_get_host_error,
};


PmError pm_macosxcm_init(void)
{
    ItemCount numInputs, numOutputs, numDevices;
    MIDIEndpointRef endpoint;
    int i;
    OSStatus macHostError;
    char *error_text;

    memset(isIAC, 0, sizeof(isIAC)); /* initialize all FALSE */

    /* Register interface CoreMIDI with create_virtual fn */
    pm_add_interf("CoreMIDI", &midi_create_virtual);
    /* no check for error return because this always succeeds */

    /* Determine the number of MIDI devices on the system */
    numDevices = MIDIGetNumberOfDevices();
    numInputs = MIDIGetNumberOfSources();
    numOutputs = MIDIGetNumberOfDestinations();

    /* Return prematurely if no devices exist on the system
       Note that this is not an error. There may be no devices.
       Pm_CountDevices() will return zero, which is correct and
       useful information
     */
    if (numDevices <= 0) {
        return pmNoError;
    }

    /* Initialize the client handle */
    macHostError = MIDIClientCreate(CFSTR("PortMidi"), NULL, NULL, &client);
    if (macHostError != noErr) {
        error_text = "MIDIClientCreate() in pm_macosxcm_init()";
        goto error_return;
    }

    /* Create the input port */
    macHostError = MIDIInputPortCreate(client, CFSTR("Input port"), 
                                       device_read_callback, NULL, &portIn);
    if (macHostError != noErr) {
        error_text = "MIDIInputPortCreate() in pm_macosxcm_init()";
        goto error_return;
    }

    /* Create the output port */
    macHostError = MIDIOutputPortCreate(client, CFSTR("Output port"), &portOut);
    if (macHostError != noErr) {
        error_text = "MIDIOutputPortCreate() in pm_macosxcm_init()";
        goto error_return;
    }

    /* Iterate over the MIDI input devices */
    for (i = 0; i < numInputs; i++) {
        int isIACflag;
        endpoint = MIDIGetSource(i);
        if (endpoint == NULL_REF) {
            continue;
        }

        /* set the first input we see to the default */
        if (pm_default_input_device_id == -1)
            pm_default_input_device_id = pm_descriptor_index;
        
        /* Register this device with PortMidi */
        pm_add_device("CoreMIDI", 
                      cm_get_full_endpoint_name(endpoint, &isIACflag),
                      TRUE, (void *) (long) endpoint, &pm_macosx_in_dictionary);
    }

    /* Iterate over the MIDI output devices */
    for (i = 0; i < numOutputs; i++) {
        int isIACflag;
        int id;
        endpoint = MIDIGetDestination(i);
        if (endpoint == NULL_REF) {
            continue;
        }

        /* set the first output we see to the default */
        if (pm_default_output_device_id == -1)
            pm_default_output_device_id = pm_descriptor_index;

        /* Register this device with PortMidi */
        id = pm_add_device("CoreMIDI", 
                     cm_get_full_endpoint_name(endpoint, &isIACflag), FALSE,
                     (void *) (long) endpoint, &pm_macosx_out_dictionary);
        /* if this is an IAC device, tuck that info away for write functions */
        if (isIACflag && id <= MAX_IAC_NUM) {
            isIAC[id] = TRUE;
        }
    }
    return pmNoError;
    
error_return:
    pm_hosterror = macHostError;
    sprintf(pm_hosterror_text, "Host error %ld: %s\n", (long) macHostError, 
            error_text);
    pm_macosxcm_term(); /* clear out any opened ports */
    return pmHostError;
}

void pm_macosxcm_term(void)
{
    if (client != NULL_REF) MIDIClientDispose(client);
    if (portIn != NULL_REF) MIDIPortDispose(portIn);
    if (portOut != NULL_REF) MIDIPortDispose(portOut);
}
