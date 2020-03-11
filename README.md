# MPESynthesiserExploration

MIDI Polyphonic Expression (MPE) is a unique way of processing MIDI data where each individual note is processed on a separate channel allowing for multi-dimensional control over the parameters of each note event. Standard MIDI processing only allows the user to affect certain parameters on a global scale (e.g. pitch bend). With MPE, each note can be modulated individually to allow the performer a higher level of expression and musicality.

MIDI controllers that are built around this specification have been making their way to market for the last several years and are being adopted by musicians at a growing rate. 

This project aims to investigate MPE more deeply from both a hardware and software perspective. During the software investigation, a synthesizer will be built that implements MPE and tested using the popular Roli Seaboard device. The hardware investigation will look at the current state of MPE instruments and propose a new MPE device using a microcontroller such as the Arduino UNO. The project will conclude by suggesting next steps for further research and implementation of the proposed device.

The software development phase will begin with a basic MPE proof of concept and expand into a proper polyphonic software synthesizer with at least five adjustable parameters. In order to realize the synthesizer, the following will be required:
Resources

•	JUCE – A framework for delivering cross-platform musical applications. JUCE was created in C++ and provides developers with             many libraries for audio processing, MIDI events, and graphical user interfaces. It’s active community, deep documentation, and ability to target multiple devices with the same code make it a desirable tool for this project.
•	Roli Seaboard – A multitouch MIDI controller developed with MPE in mind. The Seaboard will provide a way to test the resulting software from both a technical and musical point of view

SPECIFICATIONS
4-voice polyphony
•	Synthesiser can play up to 4 notes simultaneously

Additive Synthesis
 
3 waveforms with user editable matrix to choose harmonic waves:

Filters: 
Low Pass	High Pass
Frequency 	Follows frequency of note event	Follows frequency of note event
Resonance	MPE Timbre	MPE Timbre
Q Factor	User adjustable dial	User adjustable dial
User can toggle between Hi/Low pass filters in Filter Mode.

MPE Functionality:
Pressure	Timbre	Note-On Velocity	Pitch Bend	Note-Off Velocity
Effect	Increase or decrease the number of harmonics present in voice.	Change the weight/distribution of the harmonics.	Set the Attack value for the note event.	Frequency increases or decreases.	Set the release time of the note event.
Value	Float 0-1	Float 0-1	Float 0-1	Float 0-1 added to original frequency	Float 0-1
Mapping	0: 1 Harmonic
1: max Harmonics	Depends on Mode	0-127	0-1	0-127

Timbre Modes:
•	Even harmonics increase in amplitude while odd harmonics decrease as timbre value approaches 1.
•	Tuning of upper harmonics changes based on different tuning systems (e.g. Just Intonation)
•	Filter increases/decreases. Q gets narrower. 

