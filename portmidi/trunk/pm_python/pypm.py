# pypm implementation for Python 3.1 and above
#
# Roger B. Dannenberg
#
# Intended to emulate PyPortMidi interface
# In this implementation
#    MIDI channel numbers are zero-based.
#    Pm_Abort() is not called

import pypmbase

def Initialize():
    """
Initialize: call this first
    """
    pypmbase.Initialize()


def Terminate(): 
    """
Terminate: call this to clean up Midi streams when done.
    """
    pypmbase.Terminate()


def GetDefaultInputDeviceID():
    return pypmbase.GetDefaultInputDeviceID()


def GetDefaultOutputDeviceID():
    return pypmbase.GetDefaultOutputDeviceID()


def CountDevices():
    return pypmbase.CountDevices()


def GetDeviceInfo(i):
    """
GetDeviceInfo(<device number>): returns 5 parameters
  - underlying MIDI API
  - device name
  - TRUE iff input is available
  - TRUE iff output is available
  - TRUE iff device stream is already open
    """
    return pypmbase.GetDeviceInfo(i)


def Time():
    """
Time() returns the current time in ms
    of the PortMidi timer
    """
    return pypmbase.Time()


def GetErrorText(err):
    """
GetErrorText(<err num>) returns human-readable error
    messages translated from error numbers
    """
    return pypmbase.GetErrorText(err)

def Channel(chan):
    """
Channel(<chan>) is used with ChannelMask on input MIDI streams.
Example: to receive input on channels 1 and 10 on a MIDI
         stream called MidiIn:
MidiIn.SetChannelMask(pypm.Channel(1) | pypm.Channel(10))

note: Channels are numbered from 0 to 15 (not 1 to 16)
    """
    return pypmbase.Channel(chan)


class Output:
    """
class Output:
    define an output MIDI stream. Takes the form:
        x = pypm.Output(MidiOutputDevice, latency)
    latency is in ms.
    If latency = 0 then timestamps for output are ignored.
    """

    def __init__(self, dev, latency, buflen = 1024):
        self.midi = pypmbase.new_Stream()
        self.debug = 0
        err = pypmbase.OpenOutput(self.midi, dev, buflen, latency)
        if err < 0: raise Exception(pypmbase.GetErrorTexT(err))
        if self.debug: print("MIDI input opened")

    def __dealloc__(self):
        if self.debug: 
            print("Closing MIDI output stream and destroying instance")
        err = pypmbase.Close(self.midi)
        if err < 0: raise Exception(pypmbase.GetErrorText(err))

    def Write(self, data):
        """
Write(data)
    output a series of MIDI information in the form of a list:
         Write([[[status <,data1><,data2><,data3>],timestamp],
                [[status <,data1><,data2><,data3>],timestamp],...])
    <data> fields are optional
    example: choose program change 1 at time 20000 and
    send note 65 with velocity 100 500 ms later.
         Write([[[0xc0,0,0],20000],[[0x90,60,100],20500]])
    notes:
      1. timestamps will be ignored if latency = 0.
      2. To get a note to play immediately, send MIDI info with
         timestamp read from function Time.
      3. understanding optional data fields:
           Write([[[0xc0,0,0],20000]]) is equivalent to
           Write([[[0xc0],20000]])
        """
        err = pypmbase.Write(self.midi, data)
        if err < 0: raise Exception(pypmbase.GetErrorText(err))


    def WriteShort(self, status, data1 = 0, data2 = 0):
        """
WriteShort(status <, data1><, data2>)
     output MIDI information of 3 bytes or less.
     data fields are optional
     status byte could be:
          0xc0 = program change
          0x90 = note on
          etc.
          data bytes are optional and assumed 0 if omitted
     example: note 65 on with velocity 100
          WriteShort(0x90,65,100)
        """
        self.Write([[[status, data1, data2], 0]])


    def WriteSysEx(self, when, msg):
        """
        WriteSysEx(<timestamp>,<msg>)
        writes a timestamped system-exclusive midi message.
        <msg> can be a *list* or a *string*
        example:
            (assuming y is an input MIDI stream)
            y.WriteSysEx(0,'\\xF0\\x7D\\x10\\x11\\x12\\x13\\xF7')
                              is equivalent to
            y.WriteSysEx(pypm.Time,
            [0xF0, 0x7D, 0x10, 0x11, 0x12, 0x13, 0xF7])
        """
        if type(msg) is list:
            msg = bytes(msg)
        elif type(msg) is str:
            # this took a lot of searching. 
            # See http://docs.python.org/3.1/howto/unicode.html
            msg = bytes(msg, encoding = 'Latin-1')
        err = pypmbase.WriteSysEx(self.midi, 0, msg)
        print("WriteSysEx returns", err)
        if err < 0: raise Exception(pypmbase.GetErrorText(err))


class Input:
    """
class Input:
    define an input MIDI stream. Takes the form:
        x = pypm.Input(MidiInputDevice)
    """

    def __init__(self, dev, buflen = 1024):
        self.midi = pypmbase.new_Stream()
        self.debug = 0
        err = pypmbase.OpenInput(self.midi, dev, buflen)
        if err < 0: raise Exception(pypmbase.GetErrorTexT(err))
        if self.debug: print("MIDI input opened")

    def __dealloc__(self):
        if self.debug: 
            print("Closing MIDI input stream and destroying instance")
        err = pypmbase.Close(self.midi)
        if err < 0: raise Exception(pypmbase.GetErrorText(err))
        
    def SetFilter(self, filters):
        """
SetFilter(<filters>) sets filters on an open input stream
    to drop selected input types. By default, only active sensing
    messages are filtered. To prohibit, say, active sensing and
    sysex messages, call
    SetFilter(stream, FILT_ACTIVE | FILT_SYSEX);

    Filtering is useful when midi routing or midi thru functionality
    is being provided by the user application.
    For example, you may want to exclude timing messages
    (clock, MTC, start/stop/continue), while allowing note-related
    messages to pass. Or you may be using a sequencer or drum-machine
    for MIDI clock information but want to exclude any notes
    it may play.

    Note: SetFilter empties the buffer after setting the filter,
    just in case anything got through.
        """
        pypmbase.SetFilter(self.midi, filters)
        if err < 0: raise Exception(pypmbase.GetErrorText(err))
        while (self.Poll() != PmNoError):
            err = self.Read(1)
            if err < 0: raise Exception(pypmbase.GetErrorText(err))


    def SetChannelMask(self, mask):
        """
    SetChannelMask(<mask>) filters incoming messages based on channel.
    The mask is a 16-bit bitfield corresponding to appropriate channels
    Channel(<channel>) can assist in calling this function.
    i.e. to set receive only input on channel 1, call with
    SetChannelMask(Channel(1))
    Multiple channels should be OR'd together, like
    SetChannelMask(Channel(10) | Channel(11))
    note: PyPortMidi Channel function has been altered from
          the original PortMidi c call to correct for what
          seems to be a bug --- i.e. channel filters were
          all numbered from 0 to 15 instead of 1 to 16.
        """
        err = pypmbase.SetChannelMask(self.midi, mask)
        if err < 0: raise Exception(pypmbase.GetErrorText(err))

        
    def Poll(self):
        """
    Poll tests whether input is available,
    returning TRUE, FALSE, or an error value.
        """
        err = pypmbase.Poll(self.midi)
        if err < 0: raise Exception(pypmbase.GetErrorText(err))
        return err


    def Read(self, length):
        """
Read(length): returns up to <length> midi events stored in
    the buffer and returns them as a list:
    [[[status,data1,data2,data3],timestamp],
     [[status,data1,data2,data3],timestamp],...]
    example: Read(50) returns all the events in the buffer,
    up to 50 events.
        """
        if length < 1: raise IndexError('minimum buffer length is 1')
        x = []
        event = pypmbase.Read(self.midi)
        if type(event) is int:
            if event < 0: raise Exception(pypmbase.GetErrorText(err))
            else: event = None
        while event:
            # Read returns [message, timestamp], these are appended to list
            x.append(event)
            event = pypmbase.Read(self.midi)
            if type(event) is int:
                if event < 0: raise Exception(pypmbase.GetErrorText(err))
                else: event = None
        return x
