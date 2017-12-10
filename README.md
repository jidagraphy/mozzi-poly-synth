# mozzi-poly-synth

Poly Synth for Arduino and Mozzi
   
A simple mozzi synthesiser that supports polyphony.
   
When limited by the maximum number of polyphony, the algorithm cuts the oldest sustained note and replaces it with a newly played
note. The algorithm also prioritises bass notes to not be cut from polyphony.

Runs with Arduino MIDI Library.
Added on top of Mozzi Exmple Mozzi_MIDI_Input.
   
I've sent midi notes through the serial using hairless-midiserial tool.
To use this in a normal way, delete the line where the serial begins on the port 115200.
   
@jidagraphy

# disclaimer
the project is still in progress, I'm not even done writing this readme!