
# HetrickCV for VCV Rack

HetrickCV is a collection of VCV Rack modules by Michael Hetrick (https://www.unfilteredaudio.com/). Many of these are ported from Euro Reakt for Reaktor 6 (https://www.native-instruments.com/en/reaktor-community/reaktor-user-library/entry/show/9093/).

### Releases

Pre-compiled versions are not available yet. They will be ready soon.

## Contributing

I welcome Issues and Pull Requests to this repository if you have suggestions for improvement.

# HetrickCV User Manual

### 2-to-4 Mix Matrix
This simple module takes in two inputs (CV or Audio). It produces four outputs that are various combinations of the two inputs. It is based on Julius O. Smith's description of a mix matrix for Schroeder Reverberators (https://ccrma.stanford.edu/~jos/pasp/Schroeder_Reverberators.html). Despite its original use for spatializing audio, this can be a great module for creating CV permutations.

### ASR
This can be thought of as a quad Sample-and-Hold. It is useful for creating melodic rounds and canons. When it receives a positive Clock input, the current voltage at the main input will be sampled and sent to the first output. On the next positive Clock input, the current voltage of the first output will be moved to the second output, and the first output will sample the main input again.

Patch Ideas:
- Connect a sequencer to the main input. Connect the sequencer's clock to the Clock input. Connect the ASR's outputs to various oscillators that are tuned to the same base frequency. Mix the oscillator outputs together and listen to the complex voicings that are created.

### Analog to Digital
This module takes a signal and turns it into an 8-bit representation. The eight outputs represent the state of the bits (+5V if the bit is 1, 0V if the bit is 0). If a signal is present on the Sync input, the bits will only be updated upon the reception of a positive gate. The signal runs through a rectification stage before being sent to the encoder. There are multiple rectification modes:
- Half: Negative signals are replaced with 0V.
- Full: Negative signals are inverted (the absolute value of the signal is used).
- None: The input signal is not affected.

There are also multiple encoding modes:
- Uni. 8: The input is half-wave rectified (in addition to whatever rectification mode was selected before the encoder stage). Bit 8 is the Most Significant Bit (meaning it will take a fairly loud input signal for the bit to go high). In this mode, the encoder will only respond to positive voltages.
- Bi. Off.: The input is converted from a bi-polar signal to a uni-polar signal through the use of an internal offset followed by scaling. The signal is then encoded in a manner similar to Uni. 8, where Bit 8 is the Most Significant Bit.
- Bi. Sig.: In this mode, Bit 7 becomes the Most Significant Bit, while Bit 8 encodes whether or not the signal at the input is negative.

Patch ideas:
- Use a slow LFO as the input. The various outputs become semi-related gate streams. Use primitive waveforms (sine, triangle, etc.) for more predictable results. Use the Audible Instruments Wavetable Oscillator in LFO mode and morph the waveform for unpredictable patterns.
- Use this in tandem with the Digital to Analog module to create custom waveshaping effects. You can wire the bits up "correctly" and simply change the various encoder, rectification, offset, and scaling parameters on both modules to come up with unusual permutations. You can wire the bits up more randomly to produce harsher effects. For a lot of fun, try placing the Rotator and/or the Gate Junction modules between the A-to-D and D-to-A converters.

### Bitshift
This is a harsh waveshaping effect. It is particularly useful for taking a slow, smooth CV value and creating a lot of rapid discontinuities. The effect is produced by taking the internal floating-point representation of the signal and turning it into a 32-bit integer. The integer's bits are then shifted left (<<, which produces aggressive alterations to the signal) or right (>>, which is mostly just attenuation). Because the algorithm for this module has expected boundaries, you will need to select a range for the input signal. +/- 5V is the standard range for most audio generators in Rack. Some function generators will produce +/- 10V, though. Regardless, since this is a fairly harsh and experimental module, there's no need to select the "correct" range...

### Boolean Logic (2-or-3 Input)
These modules take in 2 or 3 gate inputs and produce 6 gates that represent the true-or-false states of the inputs. The input is considered true if it is currently above 1V (gates do not need to be used, but they provide the most predictable behavior... still, try throwing in all sorts of signals). The various outputs are as follows:
- OR: This output is true if any input is true.
- AND: This output is true if every input is true.
- XOR (Exclusive OR): This output is true if at least one input is true, but not every input.
- NOR: This output is true if every input is false (the opposite of OR).
- NAND: This output is true unless every input is true (the opposite of AND).
- XNOR (Exclusive NOR): This output is true if every input is the same state (the opposite of XOR).

Patch Ideas:
- These are some of the best modules for creating unusual, generative rhythms. Try connecting various outputs of a clock divider into these inputs. Alternatively, connect two clocks or square wave LFOs with different frequencies (for more fun, modulate the pulse widths of the clocks if possible).
- The AND output can be useful to manually toggle rhythm streams. Connect the gate stream that you want to toggle to one input. Connect a MIDI note gate to the other input. Now, the AND output will be the first rhythm as long as you hold down a MIDI note.

### Contrast
This is a type of phase distortion that I found in the CCRMA Snd wave editor (https://ccrma.stanford.edu/software/snd/snd/sndclm.html#contrast-enhancement). It will add brightness and saturation to a signal. Please note that the effect will still color the signal even if the knob is fully counter-clockwise. Like the Bitshift module, there is a range selector to set the expected range of the input signal.

Patch Ideas:
- Aside from the obvious use as a mix enhancer, try modulating the Amount parameter with another audio signal to use this as a creative distortion.

### Crackle
This is a chaotic system that generates a vinyl-like hiss with occasional pops. This is a direct port of a UGen from SuperCollider (https://github.com/supercollider/supercollider/blob/master/server/plugins/NoiseUGens.cpp#L452). When I originally ported this to Euro Reakt, I accidentally implemented the internal copy operations in the wrong order, leading to the fun "Broken" mode. The Broken mode produces stutters, grains, and modem noises at high Chaos values.

Patch Ideas:
- Surprising things can happen if you modulate this with an audio signal... 

### Digital to Analog
This module is the inverse of the Analog to Digital encoder. It takes in eight inputs and produces a single voltage based off of the state of the inputs and the selected decoder mode. The decoder modes are the inverse of the encoder modes described above in the Analog to Digital documentation. If you directly connect the two modules and use the same encoding/decoding modes, the output is typically identical to the input aside from accuracy degradation from the 8-bit representation.

Patch Ideas:
- See the Analog to Digital documentation above for creative ways of mangling the bits between the modules.
- Connect various, rhythmic gate streams to the inputs. The output is a stepped voltage based on the state of the inputs. This will be a jumpy voltage that is related to various rhythms happening inside of the patch.

### Dust
Like Crackle, this is a direct port of a SuperCollider Noise UGen (https://github.com/supercollider/supercollider/blob/master/server/plugins/NoiseUGens.cpp#L376). This module will produce randomly spaced impulses with random amplitudes. At low frequencies, this is useful as a random trigger generator. At high frequencies, this is a white noise source.

Patch Ideas:
- Connect the output of Dust to the input of a highly resonant filter. Use a slower frequency on Dust to ping the filter and create sine grains.
- Using the patch above, also link Dust's output to a sequencer's input. Use the sequencer to change the filter's cutoff frequency.

### Exponent
This is a simple waveshaper that will raise the input voltage to a power specified by the Amount knob. Turning the knob clockwise will make the output signal more exponential, while turning the knob counter-clockwise will make the output signal more logarithmic. This will have a mild effect on audio signals, but it is extremely useful for shaping LFOs and envelopes.

### Flip Flop

### Flip Pan

### Gate Junction

### OR Logic (Gate Combiner)

### Random Gates

### Rotator

### Scanner

### Waveshaper