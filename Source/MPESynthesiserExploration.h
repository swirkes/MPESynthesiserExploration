/*
  ==============================================================================

   This file is part of the JUCE tutorials.
   Copyright (c) 2017 - ROLI Ltd.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             MPEIntroductionTutorial
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Synthesiser using MPE specifications.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2017, linux_make

 type:             Component
 mainClass:        MainComponent

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once
#include "Maximilian/maximilian.h"

//==============================================================================
class NoteComponent : public Component
{
public:
    NoteComponent (const MPENote& n)
        : note (n), colour (Colours::white)
    {}

    //==============================================================================
    void update (const MPENote& newNote, Point<float> newCentre)
    {
        note = newNote;
        centre = newCentre;

        setBounds (getSquareAroundCentre (jmax (getNoteOnRadius(), getNoteOffRadius(), getPressureRadius()))
                     .getUnion (getTextRectangle())
                     .getSmallestIntegerContainer()
                     .expanded (3));
        

        repaint();
    }

    //==============================================================================
    void paint (Graphics& g) override
    {
        if (note.keyState == MPENote::keyDown || note.keyState == MPENote::keyDownAndSustained)
            drawPressedNoteCircle (g, colour);
        else if (note.keyState == MPENote::sustained)
            drawSustainedNoteCircle (g, colour);
        else
            return;

        drawNoteLabel (g, colour);
    }

    //==============================================================================
    MPENote note;
    Colour colour;
    Point<float> centre;

private:
    //==============================================================================
    void drawPressedNoteCircle (Graphics& g, Colour zoneColour)
    {
        g.setColour (zoneColour.withAlpha (0.3f));
        g.fillEllipse (translateToLocalBounds (getSquareAroundCentre (getNoteOnRadius())));
        g.setColour (zoneColour);
        g.drawEllipse (translateToLocalBounds (getSquareAroundCentre (getPressureRadius())), 2.0f);
    }

    //==============================================================================
    void drawSustainedNoteCircle (Graphics& g, Colour zoneColour)
    {
        g.setColour (zoneColour);
        Path circle, dashedCircle;
        circle.addEllipse (translateToLocalBounds (getSquareAroundCentre (getNoteOffRadius())));
        const float dashLengths[] = { 3.0f, 3.0f };
        PathStrokeType (2.0, PathStrokeType::mitered).createDashedStroke (dashedCircle, circle, dashLengths, 2);
        g.fillPath (dashedCircle);
    }

    //==============================================================================
    void drawNoteLabel (Graphics& g, Colour)
    {
        auto textBounds = translateToLocalBounds (getTextRectangle()).getSmallestIntegerContainer();
        g.drawText ("+", textBounds, Justification::centred);
        g.drawText (MidiMessage::getMidiNoteName (note.initialNote, true, true, 3), textBounds, Justification::centredBottom);
        g.setFont (Font (22.0f, Font::bold));
        g.drawText (String (note.midiChannel), textBounds, Justification::centredTop);
    }

    //==============================================================================
    Rectangle<float> getSquareAroundCentre (float radius) const noexcept
    {
        return Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);
    }

    Rectangle<float> translateToLocalBounds (Rectangle<float> r) const noexcept
    {
        return r - getPosition().toFloat();
    }

    Rectangle<float> getTextRectangle() const noexcept
    {
        return Rectangle<float> (30.0f, 50.0f).withCentre (centre);
    }

    float getNoteOnRadius()   const noexcept   { return note.noteOnVelocity.asUnsignedFloat() * maxNoteRadius; }
    float getNoteOffRadius()  const noexcept   { return note.noteOffVelocity.asUnsignedFloat() * maxNoteRadius; }
    float getPressureRadius() const noexcept   { return note.pressure.asUnsignedFloat() * maxNoteRadius; }

    static constexpr auto maxNoteRadius = 100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteComponent)
};

//==============================================================================
class Visualiser : public Component,
                   public MPEInstrument::Listener,
                   private AsyncUpdater
{
public:
    //==============================================================================
    Visualiser() {}

    //==============================================================================
    void paint (Graphics& g) override
    {
        g.fillAll (Colours::black);

        auto noteDistance = float (getWidth()) / 128;

        for (auto i = 0; i < 128; ++i)
        {
            auto x = noteDistance * i;
            auto noteHeight = int (MidiMessage::isMidiNoteBlack (i) ? 0.7 * getHeight() : getHeight());
            g.setColour (MidiMessage::isMidiNoteBlack (i) ? Colours::white : Colours::grey);
            g.drawLine (x, 0.0f, x, (float) noteHeight);

            if (i > 0 && i % 12 == 0)
            {
                g.setColour (Colours::grey);
                auto octaveNumber = (i / 12) - 2;
                g.drawText ("C" + String (octaveNumber), (int) x - 15, getHeight() - 30, 30, 30, Justification::centredBottom);
            }
        }
    }

    //==============================================================================
    void noteAdded (MPENote newNote) override
    {
        const ScopedLock sl (lock);
        activeNotes.add (newNote);
        triggerAsyncUpdate();
    }

    void notePressureChanged  (MPENote note) override { noteChanged (note); }
    void notePitchbendChanged (MPENote note) override { noteChanged (note); }
    void noteTimbreChanged    (MPENote note) override { noteChanged (note); }
    void noteKeyStateChanged  (MPENote note) override { noteChanged (note); }

    void noteChanged (MPENote changedNote)
    {
        const ScopedLock sl (lock);

        for (auto& note : activeNotes)
            if (note.noteID == changedNote.noteID)
                note = changedNote;

        triggerAsyncUpdate();
    }

    void noteReleased (MPENote finishedNote) override
    {
        const ScopedLock sl (lock);

        for (auto i = activeNotes.size(); --i >= 0;)
            if (activeNotes.getReference(i).noteID == finishedNote.noteID)
                activeNotes.remove (i);

        triggerAsyncUpdate();
    }


private:
    //==============================================================================
    const MPENote* findActiveNote (int noteID) const noexcept
    {
        for (auto& note : activeNotes)
            if (note.noteID == noteID)
                return &note;

        return nullptr;
    }

    NoteComponent* findNoteComponent (int noteID) const noexcept
    {
        for (auto& noteComp : noteComponents)
            if (noteComp->note.noteID == noteID)
                return noteComp;

        return nullptr;
    }

    //==============================================================================
    void handleAsyncUpdate() override
    {
        const ScopedLock sl (lock);

        for (auto i = noteComponents.size(); --i >= 0;)
            if (findActiveNote (noteComponents.getUnchecked(i)->note.noteID) == nullptr)
                noteComponents.remove (i);

        for (auto& note : activeNotes)
            if (findNoteComponent (note.noteID) == nullptr)
                addAndMakeVisible (noteComponents.add (new NoteComponent (note)));

        for (auto& noteComp : noteComponents)
            if (auto* noteInfo = findActiveNote (noteComp->note.noteID))
                noteComp->update (*noteInfo, getCentrePositionForNote (*noteInfo));
    }

    //==============================================================================
    Point<float> getCentrePositionForNote (MPENote note) const
    {
        auto n = float (note.initialNote) + float (note.totalPitchbendInSemitones);
        auto x = getWidth() * n / 128;
        auto y = getHeight() * (1 - note.timbre.asUnsignedFloat());

        return { x, y };
    }

    //==============================================================================
    OwnedArray<NoteComponent> noteComponents;
    CriticalSection lock;
    Array<MPENote> activeNotes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Visualiser)
};

//==============================================================================
class MPEDemoSynthVoice : public MPESynthesiserVoice
{
public:
    //==============================================================================
    MPEDemoSynthVoice() {}

    //==============================================================================
    void noteStarted() override
    {
        jassert (currentlyPlayingNote.isValid());
        jassert (currentlyPlayingNote.keyState == MPENote::keyDown
                 || currentlyPlayingNote.keyState == MPENote::keyDownAndSustained);

        env1.trigger = 1;
        
        // get data from the current MPENote
        level.setTargetValue(currentlyPlayingNote.pressure.asUnsignedFloat());
        frequency.setTargetValue(currentlyPlayingNote.getFrequencyInHertz());
        timbre.setTargetValue(currentlyPlayingNote.timbre.asUnsignedFloat());

//        phase = 0.0;
//        auto cyclesPerSample = frequency.getNextValue() / currentSampleRate;
//        phaseDelta = 2.0 * MathConstants<double>::pi * cyclesPerSample;

        //tailOff = 0.0;
        
        
    }

    void noteStopped (bool allowTailOff) override
    {
        jassert (currentlyPlayingNote.keyState == MPENote::off);
        
        
//        if (level == 0)
//            clearCurrentNote();

        if (allowTailOff)
        {
            // start a tail-off by setting this flag. The render callback will pick up on
            // this and do a fade out, calling clearCurrentNote() when it's finished.


            if (tailOff == 0.0) // we only need to begin a tail-off if it's not already doing so - the
                                // stopNote method could be called more than once
                tailOff = 1.0 ;
        }
        else
        {
            // we're being told to stop playing immediately, so reset everything..
            env1.trigger = 0;
            clearCurrentNote();
            //phaseDelta = 0.0;
        }
    }

    void notePressureChanged() override
    {
        level.setTargetValue(currentlyPlayingNote.pressure.asUnsignedFloat());
    }

    void notePitchbendChanged() override
    {
        frequency.setTargetValue(currentlyPlayingNote.getFrequencyInHertz());
        
    }

    void noteTimbreChanged() override
    {
        timbre.setTargetValue(currentlyPlayingNote.timbre.asUnsignedFloat());
        
    }

    void noteKeyStateChanged() override {}

    void setCurrentSampleRate (double newRate) override
    {
        if (currentSampleRate != newRate)
        {
            noteStopped (true);
            currentSampleRate = newRate;

            level    .reset (currentSampleRate, smoothingLengthInSeconds);
            timbre   .reset (currentSampleRate, smoothingLengthInSeconds);
            frequency.reset (currentSampleRate, smoothingLengthInSeconds);
        }
    }

    //==============================================================================
    void renderNextBlock (AudioBuffer<float>& outputBuffer,
                          int startSample,
                          int numSamples) override
    {
        env1.setAttack(2000);
        env1.setDecay(500);
        env1.setSustain(0.8);
        env1.setRelease(2000);
        
        
        for (auto sample = 0; sample < numSamples; ++sample)
        {
            
            
            float wave1 = osc1.sinebuf(frequency.getNextValue());
            float sound1 = env1.adsr(wave1, env1.trigger) * level.getNextValue();
            //float filteredsound1 = filter1.hires(sound1, 200, timbre.getNextValue());
            
            
            for (auto channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
            {
                outputBuffer.addSample(channel, startSample, sound1);
            }
            
            ++startSample;
        }
        
    }

private:
    //==============================================================================
   // float getNextSample() noexcept
  //  {
//        auto levelDb = (level.getNextValue() - 1.0) * maxLevelDb;
//        auto amplitude = std::pow (10.0f, 0.05f * levelDb) * maxLevel;
//
//        // timbre is used to blend between a sine and a square.
//        auto f1 = std::sin (phase);
//        auto f2 = std::copysign (1.0, f1);
//        auto a2 = timbre.getNextValue();
//        auto a1 = 1.0 - a2;
//
//        auto nextSample = float (amplitude * ((a1 * f1) + (a2 * f2)));
//
//        auto cyclesPerSample = frequency.getNextValue() / currentSampleRate;
//        phaseDelta = 2.0 * MathConstants<double>::pi * cyclesPerSample;
//        phase = std::fmod (phase + phaseDelta, 2.0 * MathConstants<double>::pi);
//
//        return nextSample;
   // }

    //==============================================================================
    //float level, timbre;
    SmoothedValue<float> level, timbre, frequency;

    double phase      = 0.0;
    double phaseDelta = 0.0;
    double tailOff    = 0.0;
    maxiOsc osc1;
    maxiOsc osc2;
    maxiOsc osc3;
    maxiOsc osc4;
    maxiOsc osc5;
    maxiEnv env1;
    maxiFilter filter1;

    // some useful constants
    static constexpr auto maxLevel = 0.05;
    static constexpr auto maxLevelDb = 31.0;
    static constexpr auto smoothingLengthInSeconds = 0.01;
};

//==============================================================================
class MainComponent : public Component,
                      private AudioIODeviceCallback,  // [1]
                      private MidiInputCallback       // [2]
{
public:
    //==============================================================================
    MainComponent()
        : audioSetupComp (audioDeviceManager, 0, 0, 0, 256,
                          true, // showMidiInputOptions must be true
                          true, true, false)
    {
        audioDeviceManager.initialise (0, 2, nullptr, true, {}, nullptr);
        audioDeviceManager.addMidiInputDeviceCallback ({}, this); // [6]
        audioDeviceManager.addAudioCallback (this);

        addAndMakeVisible (audioSetupComp);
        addAndMakeVisible (visualiserViewport);

        visualiserViewport.setScrollBarsShown (false, true);
        visualiserViewport.setViewedComponent (&visualiserComp, false);
        visualiserViewport.setViewPositionProportionately (0.5, 0.0);

        visualiserInstrument.addListener (&visualiserComp);

        for (auto i = 0; i < 15; ++i)
            synth.addVoice (new MPEDemoSynthVoice());

        synth.enableLegacyMode (24);
        synth.setVoiceStealingEnabled (false);

        visualiserInstrument.enableLegacyMode (24);

        setSize (650, 560);
    }

    ~MainComponent() override
    {
        audioDeviceManager.removeMidiInputDeviceCallback ({}, this);
    }

    //==============================================================================
    void resized() override
    {
        auto visualiserCompWidth = 2800;
        auto visualiserCompHeight = 300;

        auto r = getLocalBounds();

        visualiserViewport.setBounds (r.removeFromBottom (visualiserCompHeight));
        visualiserComp.setBounds ({ visualiserCompWidth,
                                    visualiserViewport.getHeight() - visualiserViewport.getScrollBarThickness() });

        audioSetupComp.setBounds (r);
    }

    //==============================================================================
    void audioDeviceIOCallback (const float** /*inputChannelData*/, int /*numInputChannels*/,
                                float** outputChannelData, int numOutputChannels,
                                int numSamples) override
    {
        // make buffer
        AudioBuffer<float> buffer (outputChannelData, numOutputChannels, numSamples);

        // clear it to silence
        buffer.clear();

        MidiBuffer incomingMidi;

        // get the MIDI messages for this audio block
        midiCollector.removeNextBlockOfMessages (incomingMidi, numSamples);

        // synthesise the block
        synth.renderNextBlock (buffer, incomingMidi, 0, numSamples);
    }

    void audioDeviceAboutToStart (AudioIODevice* device) override
    {
        auto sampleRate = device->getCurrentSampleRate();
        midiCollector.reset (sampleRate);
        synth.setCurrentPlaybackSampleRate (sampleRate);
    }

    void audioDeviceStopped() override {}

private:
    //==============================================================================
    void handleIncomingMidiMessage (MidiInput* /*source*/,
                                    const MidiMessage& message) override
    {
        visualiserInstrument.processNextMidiEvent (message);
        midiCollector.addMessageToQueue (message);
    }

    //==============================================================================
    AudioDeviceManager audioDeviceManager;         // [3]
    AudioDeviceSelectorComponent audioSetupComp;   // [4]

    Visualiser visualiserComp;
    Viewport visualiserViewport;

    MPEInstrument visualiserInstrument;
    MPESynthesiser synth;
    MidiMessageCollector midiCollector;            // [5]

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
