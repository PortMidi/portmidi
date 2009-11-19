/* pypm.i : interface file for SWIG
 *
 * Roger B. Dannenberg, Nov 2009
 *
 * created to interface portmidi to Python 3.1
 * (for Python 2.6, use Pyrex and pypm.pyx)
 */

%module pypm
%{
#define PDB 1 // debugging on if true
#define D if(PDB)
#if PDB
#include "stdio.h"
#endif

#include "portmidi.h"
#include "porttime.h"

#define PYPM_INVALID_ARGUMENT (pmHostError + 100)

int Initialize()
{ 
    int err = Pm_Initialize();
    if (err == pmNoError) Pt_Start(1, NULL, NULL);
    return err;
}

int Terminate() { Pt_Stop(); return Pm_Terminate(); }

int GetDefaultInputDeviceID() { return Pm_GetDefaultInputDeviceID(); }

int GetDefaultOutputDeviceID() { return Pm_GetDefaultOutputDeviceID(); }

int CountDevices() { return Pm_CountDevices(); }

PyObject *GetDeviceInfo(int device_id)
{
    PyObject *papi;
    PyObject *pdev;
    PyObject *pin;
    PyObject *pout;
    PyObject *popened;
    PyObject *rslt;
    const PmDeviceInfo *info = Pm_GetDeviceInfo(device_id);
    if (info) {
        papi = PyUnicode_FromStringAndSize(info->interf, strlen(info->interf));
        pdev = PyUnicode_FromStringAndSize(info->name, strlen(info->name));
        pin = PyInt_FromLong(info->input);
        pout = PyInt_FromLong(info->output);
        popened = PyInt_FromLong(info->opened);
        rslt = PyTuple_New(5);
        PyTuple_SET_ITEM(rslt, 0, papi);
        PyTuple_SET_ITEM(rslt, 1, pdev);
        PyTuple_SET_ITEM(rslt, 2, pin);
        PyTuple_SET_ITEM(rslt, 3, pout);
        PyTuple_SET_ITEM(rslt, 4, popened);
    } else {
        rslt = Py_None;
        Py_INCREF(Py_None);
    }
    return rslt;
}

int Time() { return Pt_Time(); }

const char *GetErrorText(int err)
{ 
    if (err == PYPM_INVALID_ARGUMENT) {
        return "pypmbase: `Invalid argumnet'";
    }
    return Pm_GetErrorText(err); 
}

PmStream **new_Stream() 
{
    PmStream **s = (PmStream *) malloc(sizeof(PmStream *)); 
    D printf("new_Stream returns %p\n", s);
    return s;
}

int OpenOutput(PmStream **midi, int device, int buflen, int latency)
{
    int rslt = Pm_OpenOutput(midi, device, NULL, buflen, 
                             (latency ? &Pt_Time : NULL), NULL, latency);
    D printf("OpenOutput midi %p *midi %p rslt %d\n", midi, *midi, rslt);
    return rslt;
}


int OpenInput(PmStream **midi, int device, int buflen)
{
    return Pm_OpenInput(midi, device, NULL, buflen, &Pt_Time, NULL);
}

int SetFilter(PmStream **midi, int filters) 
{
    return Pm_SetFilter(*midi, filters); 
}

int Channel(int chan) { return Pm_Channel(chan); }

int SetChannelMask(PmStream **midi, int mask)
{
    return Pm_SetChannelMask(*midi, mask);
}

int Abort(PmStream **midi) { return Pm_Abort(*midi); }

int Close(PmStream **midi) 
{
    int rslt = pmBadPtr;
    if (midi) {
        rslt = Pm_Close(*midi);
        *midi = NULL;
        free(midi);
    }
    return rslt;
}


int Write(PmStream **midi, PyObject *data)
{
    int i, j, k;
    // we'll send data in chunks of 16 messages
    #define PYPM_BUFFER_LEN 16
    PmEvent buffer[PYPM_BUFFER_LEN];
    int len = PyObject_Length(data);
    if (len == -1) {
        return PYPM_INVALID_ARGUMENT;
    }
    j = 0; // message index
    for (i = 0; i < len; i++) {
        int bytes_len;
        int msg;
        PyObject *bytes;
        PyObject *timestamp;
        PyObject *message = PySequence_GetItem(data, i);
        if (!message) return PYPM_INVALID_ARGUMENT;
        bytes = PySequence_GetItem(message, 0);
        if (!bytes) {
            Py_DECREF(message);
            return PYPM_INVALID_ARGUMENT;
        }
        bytes_len = PyObject_Length(bytes);
        if (bytes_len <= 0 || bytes_len > 4) return PYPM_INVALID_ARGUMENT;
        msg = 0;
        k = 0;
        for (k = 0; k < bytes_len; k++) {
            PyObject *byte = PySequence_GetItem(bytes, k);
            if (!byte) {
                Py_DECREF(message);
                Py_DECREF(bytes);
                return PYPM_INVALID_ARGUMENT;
            }
            msg = msg + ((PyInt_AsLong(byte) & 0xFF) << (8 * k));
            if (PyErr_Occurred()) {
                Py_DECREF(message);
                Py_DECREF(bytes);
                Py_DECREF(byte);
                return PYPM_INVALID_ARGUMENT;
            } else {
                Py_DECREF(byte);
            }
        }
        Py_DECREF(bytes);
        buffer[j].message = msg;
        timestamp = PySequence_GetItem(message, 1);
        if (!timestamp) {
            Py_DECREF(message);
            return PYPM_INVALID_ARGUMENT;
        }
        buffer[j++].timestamp = PyInt_AsLong(timestamp);
        if (PyErr_Occurred()) {
            Py_DECREF(message);
            Py_DECREF(timestamp);
            return PYPM_INVALID_ARGUMENT;
        }
        Py_DECREF(timestamp);
        Py_DECREF(message);
        if (j >= PYPM_BUFFER_LEN) {
            int r;
            D printf(
                "Write about to write buffer, *midi %p i %d buffer[1] %x@%d\n", 
                *midi, i, buffer[0].message, buffer[0].timestamp);
            r = Pm_Write(*midi, buffer, PYPM_BUFFER_LEN);
            D printf("Write returns %d\n", r);
            if (r < PYPM_BUFFER_LEN) return r;
            j = 0;
        }
    }
    if (j > 0) {
        int r;
        D printf(
            "Write about to write buffer, *midi %p j %d buffer[1] %x@%d\n", 
            *midi, j, buffer[0].message, buffer[0].timestamp);
        r = Pm_Write(*midi, buffer, j);
        D printf("Write returns %d\n", r);
        if (r < j) return r;
    }
    return i;
}

int WriteSysEx(PmStream **midi, int timestamp, PyObject *msg)
{
    unsigned char *bytes;
    int len;
    if (!PyBytes_Check(msg)) {
        D printf("WriteSysEx PyBytes_Check failure\n");
        D printf("Type: ");
        D PyObject_Print(msg->ob_type, stdout, 0);
        return PYPM_INVALID_ARGUMENT;
    }
    bytes = PyBytes_AsString(msg);
    if (!bytes) {
        D printf("WriteSysEx PyBytes_AsString failure\n");
        return PYPM_INVALID_ARGUMENT;
    }
    len = PyBytes_GET_SIZE(msg);
    // check for valid-looking sysex message
    if (len < 3 || *bytes != 0xF0 || bytes[len - 1] != 0xF7) {
        D printf(
        "WriteSysEx format failure: len %d, *bytes %x, bytes[len - 1] %x\n",
                 len, *bytes, bytes[len - 1]);
        return PYPM_INVALID_ARGUMENT;
    }
    return Pm_WriteSysEx(*midi, timestamp, bytes);
}


int Poll(PmStream **midi) { return Pm_Poll(*midi); }

PyObject *Read(PmStream **midi)
{
    int i, n;
    PyObject *num;
    PyObject *msg;
    PyObject *rslt;
    PmEvent buffer;
    n = Pm_Read(*midi, &buffer, 1);
    if (n <= 0) return PyInt_FromLong(n);
    // I'm not really sure how to do error recovery when allocation 
    // fails. If allocation fails here, I think Python is not going
    // to get much further, so I'll just return. This may leak some
    // previously allocated objects. I'm also not sure what happens
    // if I return NULL, but I can't return an error number because
    // that would require PyInt_FromLong() which is sure to fail.
    msg = PyList_New(4);
    rslt = PyList_New(2);
    if (!msg || !rslt) return NULL;
    for (i = 0; i < 4; i++) {
        num = PyInt_FromLong((buffer.message >> (i * 8)) & 0xFF);
        if (!num) return NULL;
        PyList_SET_ITEM(msg, i, num);
    }
    PyList_SET_ITEM(rslt, 0, msg);
    msg = PyInt_FromLong(buffer.timestamp);
    if (!msg) return NULL;
    PyList_SET_ITEM(rslt, 1, msg);
    return rslt;
}

const int FILT_ACTIVE = PM_FILT_ACTIVE;
const int FILT_SYSEX = PM_FILT_SYSEX;
const int FILT_CLOCK = PM_FILT_CLOCK;
const int FILT_PLAY = PM_FILT_PLAY;
const int FILT_F9 = PM_FILT_TICK;
const int FILT_TICK = PM_FILT_TICK;
const int FILT_FD = PM_FILT_FD;
const int FILT_UNDEFINED = PM_FILT_UNDEFINED;
const int FILT_RESET = PM_FILT_RESET;
const int FILT_REALTIME = PM_FILT_REALTIME;
const int FILT_NOTE = PM_FILT_NOTE;
const int FILT_CHANNEL_AFTERTOUCH = PM_FILT_CHANNEL_AFTERTOUCH;
const int FILT_POLY_AFTERTOUCH = PM_FILT_POLY_AFTERTOUCH;
const int FILT_AFTERTOUCH = PM_FILT_AFTERTOUCH;
const int FILT_PROGRAM = PM_FILT_PROGRAM;
const int FILT_CONTROL = PM_FILT_CONTROL;
const int FILT_PITCHBEND = PM_FILT_PITCHBEND;
const int FILT_MTC = PM_FILT_MTC;
const int FILT_SONG_POSITION = PM_FILT_SONG_POSITION;
const int FILT_SONG_SELECT = PM_FILT_SONG_SELECT;
const int FILT_TUNE = PM_FILT_TUNE;

const int pypmInvalidArgument = PYPM_INVALID_ARGUMENT;

%}

const int FILT_ACTIVE;
const int FILT_ACTIVE;
const int FILT_SYSEX;
const int FILT_CLOCK;
const int FILT_PLAY;
const int FILT_F9;
const int FILT_TICK;
const int FILT_FD;
const int FILT_UNDEFINED;
const int FILT_RESET;
const int FILT_REALTIME;
const int FILT_NOTE;
const int FILT_CHANNEL_AFTERTOUCH;
const int FILT_POLY_AFTERTOUCH;
const int FILT_AFTERTOUCH;
const int FILT_PROGRAM;
const int FILT_CONTROL;
const int FILT_PITCHBEND;
const int FILT_MTC;
const int FILT_SONG_POSITION;
const int FILT_SONG_SELECT;
const int FILT_TUNE;

const int pypmInvalidArgument;

const int pmNoError;
const int pmNoData;
const int pmGotData;
const int pmHostError;
const int pmInvalidDeviceId;
const int pmInsufficientMemory;
const int pmBufferTooSmall;
const int pmBufferOverflow;
const int pmBadPtr;
const int pmBadData;
const int pmInternalError;
const int pmBufferMaxSize;


int Initialize();
int Terminate();
const char *GetErrorText(int err);
int GetDefaultInputDeviceID();
int GetDefaultOutputDeviceID();
int CountDevices();
PyObject *GetDeviceInfo(int device_id);
PmStream **new_Stream();
int OpenOutput(PmStream **midi, int device, int buflen, int latency);
int OpenInput(PmStream **midi, int device, int buflen);
int SetFilter(PmStream **midi, int filters);
int Channel(int chan);
int SetChannelMask(PmStream **midi, int mask);
int Abort(PmStream **midi);
int Close(PmStream **midi);
int Write(PmStream **midi, PyObject *data);
int WriteSysEx(PmStream **midi, int timestamp, PyObject *msg);
int Poll(PmStream **midi);
PyObject *Read(PmStream **midi);
int Time();

