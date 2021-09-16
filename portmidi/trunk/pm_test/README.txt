README.txt - for pm_test directory

These are all test programs for PortMidi

Because device numbers depend on the system, there is no automated
script to run all tests on PortMidi.

To run the full set of tests manually:

Note: everything is run from the ../Debug or ../Release directory.
Actual or example input is marked with >>, e.g., >>0 means type 0<ENTER>
Comments are shown in square brackets [like this]

1. ./qtest -- output should show a bunch of tests and no error message.

2. ./test [for test input]
Latency in ms: >>0
enter your choice... >>1
Type input number: >>6  [pick a working input device]
[play some notes, look for note-on (0x90) with pitch and velocity data]

3. ./test [for test input (fail w/assert)
Latency in ms: >>0
enter your choice... >>3
Type input number: >>6  [pick a working input device]
[play some notes, program will abort after 5 messages]

4. ./test [for test input (fail w/NULL assign)
Latency in ms: >>0
enter your choice... >>3
Type input number: >>6  [pick a working input device]
[play some notes, program will Segmentation fault after 5 messages]

5. ./test [for test output, no latency]
Latency in ms: >>0
enter your choice... >>4
Type output number: >>2  [pick a working output device]
>> [type ENTER when prompted (7 times)]
[hear note on, note off, note on, note off, chord]

6. ./test [for test output, latency > 0]
Latency in ms: >>300
enter your choice... >>4
Type output number: >>2  [pick a working output device]
>> [type ENTER when prompted (7 times)]
[hear note on, note off, note on, note off, arpeggiated chord
 (delay of 300ms should be apparent)]

7. ./test [for both, no latency]
Latency in ms: >>0
enter your choice... >>5
Type input number: >>6  [pick a working input device]
Type output number: >>2  [pick a working output device]
[play notes on input, hear them on output]

8. ./test [for both, latency > 0]
Latency in ms: >>300
enter your choice... >>5
Type input number: >>6  [pick a working input device]
Type output number: >>2  [pick a working output device]
[play notes on input, hear them on output (delay of 300ms is apparent)]

9. ./test [stream test]
Latency in ms: >>0 [does not matter]
enter your choice... >>6
Type output number: >>2  [pick a working output device]
>> [type ENTER to start]
[hear 4 notes: C D E F# at one note per second, then all turn off]
ready to close and terminate... (type ENTER) :>> [type ENTER (twice)]

10. ./test [isochronous out]
Latency in ms: >>300
enter your choice... >>7
Type output number: >>2  [pick a working output device]
ready to send program 1 change... (type ENTER): >> [type ENTER]
[hear ~40 notes, exactly 4 notes per second, no jitter]

11. ./latency [no MIDI, histogram]
Choose timer period (in ms, >= 1): >>1
? >>1 [No MIDI traffic option]
[wait about 10 seconds]
>> [type ENTER]
[output should be something like ... Maximum latency: 1 milliseconds]

12. ./latency [MIDI input, histogram]
Choose timer period (in ms, >= 1): >>1
? >>2 [MIDI input option]
Midi input device number: >>6  [pick a working input device]
[wait about 5 seconds, play input for 10 seconds ]
>> [type ENTER]
[output should be something like ... Maximum latency: 3 milliseconds]

13. ./latency [MIDI output, histogram]
Choose timer period (in ms, >= 1): >>1
? >>3 [MIDI output option]
Midi output device number: >>2  [pick a working output device]
Midi output should be sent every __ callback iterations: >>50
[wait until you hear notes for 5 or 10 seconds]
>> [type ENTER to stop]
[output should be something like ... Maximum latency: 2 milliseconds]

14. ./latency [MIDI input and output, histogram]
Choose timer period (in ms, >= 1): >>1
? >>4 [MIDI input and output option]
Midi input device number: >>6  [pick a working input device]
Midi output device number: >>2  [pick a working output device]
Midi output should be sent every __ callback iterations: >>50
[wait until you hear notes, simultaneously play notes for 5 or 10 seconds]
>> [type ENTER to stop]
[output should be something like ... Maximum latency: 1 milliseconds]

15. ./mm [test with device input]
Type input device number: >>6  [pick a working input device]
[play some notes, see notes printed]

16. ./midithread -i 6 -o 2 [use working input/output device numbers]
>>5  [enter a transposition number]
[play some notes, hear parallel 4ths]
>>q [quit after ENTER a couple of times]

17. ./midiclock         [in one shell]
    ./mm                [in another shell]
[Goal is send clock messages to MIDI monitor program. This requires
 either a hardware loopback (MIDI cable from OUT to IN on interface)
 or a software loopback (macOS IAC bus or ALSA MIDI Through Port)]
[For midiclock application:]
    Type output device number: >>0  [pick a device with loopback]
    Type ENTER to start MIDI CLOCK: >> [type ENTER]
[For mm application:]
    Type input device number: >>1 [pick device with loopback]
    [Wait a few seconds]
    >>s  [to get Clock Count]
    >>s  [expect to get a higher Clock Count]
[For midiclock application:]
    >>c  [turn off clocks]
[For mm application:]
    >>s  [to get Clock Count]
    >>s  [expect to Clock Count stays the same]
[For midiclock application:]
    >>t  [turn on time code, see Time Code Quarter Frame messages from mm]
    >>q  [to quit]
[For mm application:]
    >>q  [to quit]

18. ./midithru -i 6 -o 2 [use working input/output device numbers]
[Play notes on input device; notes are sent immediately and also with a
 2 sec delay to the output device; program terminates in 60 seconds]
>> [ENTER to exit]


19. ./recvvirtual [in one shell]
    ./test [in another shell]
[For test application:]
    Latency in ms: >>0
    enter your choice... >>4 [test output]
    Type output number: >>9 [select the "portmidi (output)" device]
    [type ENTER to each prompt, see that recvvirtual "Got message 0"
     through "Got message 9"]
    >> [ENTER to quit]
[For recvvirtual application:]
    >> [ENTER to quit]

20. ./sendvirtual [in one shell]
    ./mm [in another shell]
[For mm application:]
    Type input device number: >>10 [select the "portmidi" device]
[For sendvirtual application:]
    Type ENTER to send messages: >> [type ENTER]
    [see NoteOn and off messages received by mm for Key 60-64]
    >> [ENTER to quit]
[For mm application:]
    >>q [and ENTER twice to quit]

21. ./sysex [no latency]
[This requires either a hardware loopback (MIDI cable from OUT to IN
 on interface) or a software loopback (macOS IAC bus or ALSA MIDI
 Through Port)]
>>l [for loopback test]
Type output device number: >>0 [pick output device to loopback]
Latency in milliseconds: >>0
Type input  device number: >>0 [pick input device for loopback]
[Program will run forever. After awhile, quit with ^C. You can read
 the Cummulative bytes/sec value.]

22. ./sysex [latency > 0]
[This requires either a hardware loopback (MIDI cable from OUT to IN
 on interface) or a software loopback (macOS IAC bus or ALSA MIDI
 Through Port)]
>>l [for loopback test]
Type output device number: >>0 [pick output device to loopback]
Latency in milliseconds: >>100
Type input  device number: >>0 [pick input device for loopback]
[Program will run forever. After awhile, quit with ^C. You can read
 the Cummulative bytes/sec value; it is affected by latency.]

23. ./fast [no latency]
    ./fastrcv [in another shell]
[This is a speed check, especially for macOSX IAC bus connections,
 which are known to drop messages if you send messages too fast.
 fast and fastrcv must use a loopback to function.]
[In fastrcv:]
    Input device number: >>1 [pick a non-hardware device if possible]
[In fast:]
    Latency in ms: >>0
    Rate in messages per second: >>10000
    Duration in seconds: >>10
    Output device number: >>0 [pick a non-hardware device if possible]
    sending output...
[see message counts and times; on Linux you should get about 1000
 messages/second, e.g. the final count displayed could be 89992 at
 around 9000ms; on macOS you should get less than 10, e.g. the
 final count could be 60244 at 8205ms, and data will not be
 printed every 1000ms due to blocking; Windows does not have
 software ports, so data rate might be limited by device.

Check output of fastrcv: there should be no errors, just msg/sec.]
 
24. ./fast [latency > 0]
    ./fastrcv [in another shell]
[This is a speed check, especially for macOSX IAC bus connections,
 which are known to drop messages if you send messages too fast.
 fast and fastrcv must use a loopback to function.]
[In fastrcv:]
    Input device number: >>1 [pick a non-hardware device if possible]
[In fast:]
    Latency in ms: >>30 [Note for ALSA, use latency * msgs/ms < 400]
    Rate in messages per second: >>10000
    Duration in seconds: >>10
    Output device number: >>0 [pick a non-hardware device if possible]
    sending output...
[see message counts and times; on Linux you should get about 10
 messages/ms, e.g. the final count displayed could be 89322 at
 around 9032ms; on macOS you should get less than 10, e.g. the
 final count could be 60244 at 8205ms, and data will not be
 printed every 1000ms due to blocking; Windows does not have
 software ports, so data rate might be limited by device.

Check output of fastrcv: there should be no errors, just msg/sec.]
