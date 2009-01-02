import pypm

def playChord(MidiOut,notes,chan,vel,dur,time):
    ChordList = []
    for i in range(len(notes)):
        ChordList.append([[0x90 + chan,notes[i],vel], time])
    MidiOut.Write(ChordList)
    ChordList = []
    for i in range(len(notes)):
        ChordList.append([[0x90 + chan,notes[i],0], time+dur*1000])
    MidiOut.Write(ChordList)   

def test():
        latency=200 #latency is in ms, 0 means ignore timestamps
        dev = 0
        MidiOut = pypm.Output(dev, latency)

        tempo=100
        dur=60.0/tempo/2
        notes=[60, 63, 67]
        n=0
        t=pypm.Time() #current time in ms
        while n<10:
                t+=2*dur*1000
                playChord(MidiOut,[notes[0]-12],1,127,dur,t)
                playChord(MidiOut,notes,0,60,dur,t)
                n+=1
        while pypm.Time()<t+2*dur*1000: pass
        del MidiOut          
        
pypm.Initialize()
test()
pypm.Terminate()


