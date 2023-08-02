# Phasors
A "phasor" in DSP terms is a rising, linear ramp wave that moves from a value of 0.0 to 1.0. They are enormously useful, and are used for almost all time-based, cyclical behaviors. For example, phasors are used for most oscillator algorithms, and they are also used a sample accurate timers.

What does that mean for VCV, though, and why are they useful? Two clear functions are **Phase-based sequencing** and **Phase Distortion Synthesis**.

### First, some credits:

Thank you immensely to Graham Wakefield and Gregory Taylor. Their remarkable book [Generating Sound and Organizing Time](https://cycling74.com/books/go) made up the bulk of the research and design ideas that went into this set of modules. Graham was a graduate student at UCSB's Medaia Arts and Technology department as the same time as me, and has long been a source of inspiration.

Additionally, I would like to thank Claes and the Bitwig team for the Grid, and David Alexander for the [Toybox Nano series](https://www.toyboxaudio.com/pages/nano-pack), a powerful collection of Reaktor modules that really put phase-based sequencing onto my radar. These designers have greatly influenced how I put together my modules.

Finally, I would also like to thank [Polarity](https://www.youtube.com/channel/UC6fkScAhWG63SUSr3D1MI6w) and [Philip Meyer](https://www.youtube.com/@p__meyer). Their excellent YouTube channels have really taught me a lot of creative ideas regarding how to use phasors. If you are a Max user, it's worth checking out Philip's "Rhythm and Time Toolkit" package, available in the Max Package Manager.

## Phase Based Sequencing

**Phase-based sequencing** is a fairly old concept in modular synthesis, with one of the earliest examples being the "Analog Stage Select" input on the Buchla 245. Phase-based sequencing has been increasingly more popular lately, with modern software examples including the Bitwig Grid, Toybox Audio's Nano Pack for Reaktor 6, K-Devices' ESQ sequencer, and the Max 8.3 update (which included a large number of sample-accurate phase generators and modifiers). In hardware, phasor-based sequencing concepts are used in exciting ways in Mutable Instruments' classic Marbles, or ioLabs' FLUX.

That's great, but what is it?!

If you are reading this document, then you are likely familiar with how most sequencers work in modular synthesis: the sequencer receives a gate or trigger, and that gate or trigger advances the sequencer by one step.

Phase-based sequencing takes a different approach: a control voltage is used to select the active stage of the sequencer. For advancing forward through a sequence, an ascending ramp is used. For a backwards sequence, a descending ramp is used.

This simple difference provides significantly more information to a sequencer. With only two samples of an incoming phasor, a sequencer is able to calculate the frequency/BPM of a sequence, along with the direction that the sequencer should be heading. With only one sample, the sequencer is able to look up the exact step that it should be on along with the exact phase of the step (i.e. how "far" through the step we are). What this means in practice is that you can have a number of sequencers all responding to a leader phasor in lockstep: all BPM changes, directional changes, and any other shifts occur instantly without artifacts since the sequencers do not need to calculate timing between pulses.

If this sounds confusing, let's go through some patching tutorials!

- [1: First Steps](./PhasorTutorials/1-FirstSteps.md)
- [2: Warp Speed](./PhasorTutorials/2-WarpSpeed.md)
- [3: Ratcheting Up](./PhasorTutorials/3-RatchetingUp.md)
- [4: Polymeters and Polyrhythms](./PhasorTutorials/4-PolymeterPolyrhythm.md)
- [5: Phasors as Modulation](./PhasorTutorials/5-PhasorsAsModulation.md)
- [6: Navigating Rhythmic Space](./PhasorTutorials/6-NavigatingRhythmicSpace.md)

## Phase Distortion Synthesis

- Coming soon.

### Phasor Modules:
- [Phase Driven Sequencer](../Modules/PhaseDrivenSequencer.md)
- [Phasor Analyzer](../Modules/PhasorAnalyzer.md)
- [Phasor Burst Generator](../Modules/PhasorBurstGen.md)
- [Phasor Divide & Multiply](../Modules/PhasorDivMult.md)
- [Phasor Generator](../Modules/PhaseGen.md)
- [Phasor Geometry](../Modules/PhasorGeometry.md)
- [Phasor Octature](../Modules/PhasorQuadrature.md)
- [Phasor Quadrature](../Modules/PhasorQuadrature.md)
- [Phasor Randomizer](../Modules/PhasorRandom.md)
- [Phasor Ranger](../Modules/PhasorRanger.md)
- [Phasor Reset](../Modules/PhasorReset.md)
- [Phasor Rhythm Group](../Modules/PhasorRhythmGroup.md)
- [Phasor Shaper](../Modules/PhasorShape.md)
- [Phasor Shifter](../Modules/PhasorShifter.md)
- [Phasor Stutter](../Modules/PhasorStutter.md)
- [Phasor Substep Shaper](../Modules/PhasorShape.md)
- [Phasor Timetable](../Modules/PhasorTimetable.md)
- [Phasor to Clock](../Modules/PhasorToClock.md)
- [Phasor to Euclidean](../Modules/PhasorEuclidean.md)
- [Phasor to Gates](../Modules/PhaseToGates.md)
- [Phasor to LFO](../Modules/PhasorToLFO.md)
- [Phasor to Waveforms](../Modules/PhasorToWaveforms.md)