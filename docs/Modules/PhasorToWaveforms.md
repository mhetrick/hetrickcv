![Module](../Images/Modules/PhasorToWaveforms.png)

# Phasor to Waveforms

This module takes in a phasor signal and outputs traditional LFO waveforms. This can be used primarily to generate modulation signals. Additionally, it can be used at audio-rates for Phase Distortion Synthesis, but be aware that the waveforms are not anti-aliased in any way.

The left set of outputs are unipolar, while the right set is bipolar. 

One thing worth noting is that the Unipolar Saw output is almost always going to be identical to the input as long as the input is within the proper unipolar range. However, there is a protective phase wrapper at the input, so you could try unusual things here like placing a mixer before the phasor input and mixing together multiple signals to get unpredictable phase wraps.