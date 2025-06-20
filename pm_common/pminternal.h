/** @file pminternal.h header for PortMidi implementations */

/* this file is included by files that implement library internals */
/* Here is a guide to implementers:
     provide an initialization function similar to pm_winmm_init()
     add your initialization function to pm_init()
     Note that your init function should never require not-standard
         libraries or fail in any way. If the interface is not available,
         simply do not call pm_add_device. This means that non-standard
         libraries should try to do dynamic linking at runtime using a DLL
         and return without error if the DLL cannot be found or if there
         is any other failure.
     implement functions as indicated in pm_fns_type to open, read, write,
         close, etc.
     call pm_add_device() for each input and output device, passing it a
         pm_fns_type structure.
     assumptions about pm_fns_type functions are given below.
 */

/* add INTERNAL to Doxygen ENABLED_SECTIONS to include this: */
/** @cond INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

extern int pm_initialized; /* see note in portmidi.c */
extern PmDeviceID pm_default_input_device_id;
extern PmDeviceID pm_default_output_device_id;

/* these are defined in system-specific file */
void *pm_alloc(size_t s);
void pm_free(void *ptr);

/* if a host error (an error reported by the host MIDI API that is not
 * mapped to a PortMidi error code) occurs in a synchronous operation 
 * (i.e., not in a callback from another thread) set these: */
extern int pm_hosterror;  /* boolean */
extern char pm_hosterror_text[PM_HOST_ERROR_MSG_LEN];
 
struct pm_internal_struct;

/* these do not use PmInternal because it is not defined yet... */
typedef PmError (*pm_write_short_fn)(struct pm_internal_struct *midi, 
                                     PmEvent *buffer);
typedef PmError (*pm_begin_sysex_fn)(struct pm_internal_struct *midi,
                                     PmTimestamp timestamp);
typedef PmError (*pm_end_sysex_fn)(struct pm_internal_struct *midi,
                                   PmTimestamp timestamp);
typedef PmError (*pm_write_byte_fn)(struct pm_internal_struct *midi,
                                    unsigned char byte, PmTimestamp timestamp);
typedef PmError (*pm_write_realtime_fn)(struct pm_internal_struct *midi,
                                        PmEvent *buffer);
typedef PmError (*pm_write_flush_fn)(struct pm_internal_struct *midi,
                                     PmTimestamp timestamp);
typedef PmTimestamp (*pm_synchronize_fn)(struct pm_internal_struct *midi);
/* pm_open_fn should clean up all memory and close the device if any part
   of the open fails */
typedef PmError (*pm_open_fn)(struct pm_internal_struct *midi,
                              void *driverInfo);
typedef PmError (*pm_create_fn)(int is_input, const char *name,
                                void *driverInfo);
typedef PmError (*pm_delete_fn)(PmDeviceID id);
typedef PmError (*pm_abort_fn)(struct pm_internal_struct *midi);
/* pm_close_fn should clean up all memory and close the device if any
   part of the close fails. */
typedef PmError (*pm_close_fn)(struct pm_internal_struct *midi);
typedef PmError (*pm_poll_fn)(struct pm_internal_struct *midi);
typedef unsigned int (*pm_check_host_error_fn)(struct pm_internal_struct *midi);

typedef struct {
    pm_write_short_fn write_short; /* output short MIDI msg */
    pm_begin_sysex_fn begin_sysex; /* prepare to send a sysex message */
    pm_end_sysex_fn end_sysex; /* marks end of sysex message */
    pm_write_byte_fn write_byte; /* accumulate one more sysex byte */
    pm_write_realtime_fn write_realtime; /* send real-time msg within sysex */
    pm_write_flush_fn write_flush; /* send any accumulated but unsent data */
    pm_synchronize_fn synchronize; /* synchronize PM time to stream time */
    pm_open_fn open;   /* open MIDI device */
    pm_abort_fn abort; /* abort */
    pm_close_fn close; /* close device */
    pm_poll_fn poll;   /* read pending midi events into portmidi buffer */
    pm_check_host_error_fn check_host_error; /* true when device has had host */
          /* error; sets pm_hosterror and writes message to pm_hosterror_text */
} pm_fns_node, *pm_fns_type;


/* when open fails, the dictionary gets this set of functions: */
extern pm_fns_node pm_none_dictionary;

typedef struct {
    PmDeviceInfo pub; /* some portmidi state also saved in here (for automatic
                         device closing -- see PmDeviceInfo struct) */
    int deleted; /* is this is a deleted virtual device? */
    void *descriptor; /* ID number passed to win32 multimedia API open, 
                       * coreMIDI endpoint, etc., representing the device */
    struct pm_internal_struct *pm_internal; /* points to PmInternal device */
               /* when the device is open, allows automatic device closing */
    pm_fns_type dictionary;
} descriptor_node, *descriptor_type;

extern int pm_descriptor_max;
extern descriptor_type pm_descriptors;
extern int pm_descriptor_len;

typedef uint32_t (*time_get_proc_type)(void *time_info);

typedef struct pm_internal_struct {
    int device_id; /* which device is open (index to pm_descriptors) */
    short is_input; /* MIDI IN (true) or MIDI OUT (false) */
    short is_removed;  /* MIDI device was removed */
    PmTimeProcPtr time_proc; /* where to get the time */
    void *time_info; /* pass this to get_time() */
    int32_t buffer_len; /* how big is the buffer or queue? */
    PmQueue *queue;

    int32_t latency; /* time delay in ms between timestamps and actual output */
                  /* set to zero to get immediate, simple blocking output */
                  /* if latency is zero, timestamps will be ignored; */
                  /* if midi input device, this field ignored */
    
    int sysex_in_progress; /* when sysex status is seen, this flag becomes
        * true until EOX is seen. When true, new data is appended to the
        * stream of outgoing bytes. When overflow occurs, sysex data is 
        * dropped (until an EOX or non-real-timei status byte is seen) so
        * that, if the overflow condition is cleared, we don't start 
        * sending data from the middle of a sysex message. If a sysex
        * message is filtered, sysex_in_progress is false, causing the
        * message to be dropped. */
    PmMessage message; /* buffer for 4 bytes of sysex data */
    int message_count; /* how many bytes in sysex_message so far */
    int short_message_count; /* how many bytes are expected in short message */
    unsigned char running_status; /* running status byte or zero if none */
    int32_t filters; /* flags that filter incoming message classes */
    int32_t channel_mask; /* filter incoming messages based on channel */
    PmTimestamp last_msg_time; /* timestamp of last message */
    PmTimestamp sync_time; /* time of last synchronization */
    PmTimestamp now; /* set by PmWrite to current time */
    int first_message; /* initially true, used to run first synchronization */
    pm_fns_type dictionary; /* implementation functions */
    void *api_info; /* system-dependent state */
    /* the following are used to expedite sysex data */
    /* on windows, in debug mode, based on some profiling, these optimizations
     * cut the time to process sysex bytes from about 7.5 to 0.26 usec/byte,
     * but this does not count time in the driver, so I don't know if it is
     * important
     */
    unsigned char *fill_base; /* addr of ptr to sysex data */
    uint32_t *fill_offset_ptr; /* offset of next sysex byte */
    uint32_t fill_length; /* how many sysex bytes to write */
} PmInternal;

/* what is the length of this short message? */
int pm_midi_length(PmMessage msg);

/* defined by system specific implementation, e.g. pmwinmm, used by PortMidi */
void pm_init(void); 
void pm_term(void); 

/* defined by portMidi, used by pmwinmm */
PmError none_write_short(PmInternal *midi, PmEvent *buffer);
PmError none_write_byte(PmInternal *midi, unsigned char byte, 
                        PmTimestamp timestamp);
PmTimestamp none_synchronize(PmInternal *midi);

PmError pm_fail_fn(PmInternal *midi);
PmError pm_fail_timestamp_fn(PmInternal *midi, PmTimestamp timestamp);
PmError pm_success_fn(PmInternal *midi);
PmError pm_add_interf(const char *interf, pm_create_fn create_fn,
                      pm_delete_fn delete_fn);
PmError pm_add_device(const char *interf, const char *name, int is_input,
                      int is_virtual, void *descriptor, pm_fns_type dictionary);
void pm_undo_add_device(int id);
uint32_t pm_read_bytes(PmInternal *midi, const unsigned char *data, int len,
                           PmTimestamp timestamp);
void pm_read_short(PmInternal *midi, PmEvent *event);

#define none_write_flush pm_fail_timestamp_fn
#define none_sysex pm_fail_timestamp_fn
#define none_poll pm_fail_fn
#define success_poll pm_success_fn

#define MIDI_REALTIME_MASK 0xf8
#define is_real_time(msg) \
    ((Pm_MessageStatus(msg) & MIDI_REALTIME_MASK) == MIDI_REALTIME_MASK)

#ifdef __cplusplus
}
#endif

/** @endcond */
