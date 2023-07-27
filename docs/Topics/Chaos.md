# Chaos and Noise

### 1/2/3-Op Chaos and Chaos Attractors
These four modules cover a wide variety of chaotic equations, many of which have multidimensional outputs. These can run at audio or modulation rates, or they can be manually clocked. Additionally, the Slewed mode linearly interpolates between values. This mode is perfect for smooth modulation, but it is also useful as a low-pass filter when running at audio rates. The intensity of the slew is connected to the Sample Rate control, so if you are manually clocking this module make sure that you tweak the Sample Rate if you are in Slewed mode.
The Sample Rate control is present instead of a Frequency control, since these chaotic maps are typically not periodic (Yes, some of the maps, like the 1-Op Logistic map, oscillate at low Chaos values, but these are exceptions more than the norm). As such, it's important to note that changing the sampling rate of your patch will greatly affect these modules.
The 3-Op Chaos module has only one output, while the rest of the modules are multi-dimensional. On the 1-Op and 2-Op Chaos modules, some of the maps (like Logistic) are one-dimensional. For these maps, the Y output is simply -X. On Chaotic Attractors, there are X, Y, and Z outputs. For the two-dimensional maps on this module, the Z output is X*Y.
Additionally, two of the maps have been modified from their original descriptions. On the Latoocarfian map, I replaced a Sine call with a Cosine call to help it stay away from values that kill it (i.e. set all the outputs to zero and prevent new values from being generated). On the Tinkerbell map, if all values reach 0.0, all values will be updated with a random value. This can produce fun, periodic noise bursts.
Speaking of which, if any of these maps seem to get stuck, be sure to try the Reseed button/input. This will "restart" the map by inserting new, random values into it.

### Binary Noise
This noise generator will randomly generate a value of +/- 5V (or +5V and 0V in unipolar mode). At audio rates, this generates very loud, digital noise, while at lower rates this can be used to generate unpredictable gates. With slew mode enabled, it will interpolate cleanly between the two states, making it useful as a modulation source as well.

Patch ideas:
- Plug a different gate source into the Clock input. Use the probability control to shape how many gates reach the output.
- Run at audio rate and bipolar mode for really harsh square wave noise.

### Clocked Noise
This is similar to the Chaos modules above, but it uses traditional noise modes. The Flux knob sets a random modulation amount to the Sample Rate parameter whenever a new sample is generated.

### Crackle
This is a chaotic system that generates a vinyl-like hiss with occasional pops. This is a direct port of [a UGen from SuperCollider](https://github.com/supercollider/supercollider/blob/master/server/plugins/NoiseUGens.cpp#L452). When I originally ported this to Euro Reakt, I accidentally implemented the internal copy operations in the wrong order, leading to the fun "Broken" mode. The Broken mode produces stutters, grains, and modem noises at high Chaos values.

Patch Ideas:
- Surprising things can happen if you modulate this with an audio signal... 

### Dust
Like Crackle, this is a direct port of [a SuperCollider Noise UGen](https://github.com/supercollider/supercollider/blob/master/server/plugins/NoiseUGens.cpp#L376). This module will produce randomly spaced impulses with random amplitudes. At low frequencies, this is useful as a random trigger generator. At high frequencies, this is a white noise source.

Patch Ideas:
- Connect the output of Dust to the input of a highly resonant filter. Use a slower frequency on Dust to ping the filter and create sine grains.
- Using the patch above, also link Dust's output to a sequencer's input. Use the sequencer to change the filter's cutoff frequency.

### Feedback Sine Chaos
This is an algorithm ported over from Supercollider. This shares a similar set of controls with the Chaos modules described above. However, instead of a traditional chaos map, it uses a sine wave oscillator. The chaotic behavior occurs when manipulating the underlying phasor in unusual ways, including self feedback.

### Gingerbread Chaos
This is a simpler chaos module than the ones described above. Instead of having a dedicated Chaos control, the sound/shape of the map is determined by its initial conditions. The initial conditions can be rerolled using the Reseed gate input or button.