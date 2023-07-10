# HetrickCV Changelog

## 2.3.0 (in progress)
- Add Phasor Generator module.
- Add Gate Delay module.
- Add Thomas chaotic attractor to 1-Op Chaos (https://sprott.physics.wisc.edu/chaos/symmetry.htm).
- Make Mode knobs on 1-Op Chaos, 2-Op Chaos, Chaotic Attactors, and Clocked Noise snap to values and print the current mode.
- Add "Low-frequency Oscillator" tag to Feedback Sine Chaos.
- TODO: Make Scanner polyphonic.

## 2.2.1
- Add XOR feedback toggle to Rungler. Apply XOR feedback to locked and unlocked shift register.

## 2.2.0
- Add Probability module.
- Fix 2-to-4 Mix Matrix outputs 3 and 4, which were accidental duplicates of 1 and 2 when updated for polyphony.
- Fix polyphony for individual outs on Vector Mix.
- Add missing polyphony tag to Vector Mix.
- Fix Rungler feedback behavior.

## 2.1.0
- Add Apple Silicon support to build script.
- Add Vector Mix module.
- Add Gate Junction Expanded module.
- Add poly jacks to Analog<->Digital converters for easier patching.
- Add poly input jack to Gate Combiner. This is combined with the main 8 inputs (for 24 possible gate ins).
- Add port names to Gate Combiner and Gate Junction.
- Trigger times on Gate Combiner and Random Gates have been reduced from 1 ms to 1 sample.
- Trigger lights on Gate Combiner and Random Gates should now work more consistently across sample rates.

## 2.0.0
- Add 1-Op Chaos module.
- Add 2-Op Chaos module.
- Add 3-Op Chaos module.
- Add Binary Gate module.
- Add Binary Noise module.
- Add Chaotic Attractors module.
- Add Clocked Noise module.
- Add polyphonic Data Compander module.
- Add Feedback Sine Chaos module.
- Add Gingerbread Chaos module.
- Add polyphonic Mid/Side module.
- Add Rungler module.
- Add polyphonic XY<->Polar module.
- Add Gamma DSP library requirement. This is an excellent DSP library by Lance Putnam, a former colleague from grad school. We use this library as the foundation for Unfiltered Audio's DSP.
- Resaved all panels with Effra typeface instead of Gibson (very similar, but o and p rendering is fixed on export)
- Redid all panels in Affinity Designer instead of Adobe Illustrator.
- Fixed missing knob parameter names.
- Add port labels to in and out jacks.
- Add bypass behavior to all effects.
- The following modules are now polyphonic and SIMD optimized:
    - 2-to-4 Mix Matrix
    - Bitshift
    - Contrast
    - Exponent
    - Flip Pan
    - Min-Max
    - Waveshaper
    
- Knobs on Rotator and Random Gates are now snapped and display the correct stage number.
- PATCH BREAKING CHANGE: Rotator's Rotate knob now rotates channels in a more intuitive order. This will affect existing patches.

## 1.1.0
- I suppose that I forgot to update this CHANGELOG for 1.0.0. Whoops!
- Added Min-Max module.
- Waveshaper is now polyphonic (thanks, nickfeisst!)
- Random Gates now output 10V instead of 5V (thanks, giogramegna!)

## 0.5.3
- BREAKING CHANGE: Removed Boolean Logic (2-input). The 3-input version will behave identically now if nothing is connected to the third input.
- Added Comparator module.
- Added Delta module.
- Added internal hysteresis to Boolean Logic. This will reduce gate jitter when using non-gate inputs.
- Added this changelog.

## 0.5.2 (November 28, 2017)
- Added Blank Panel. Right-click to select between five designs.
- Expanded Gate Combiner to have eight inputs. Massive code cleanup.
- Added input normalization to Gate Junction.

## 0.5.1 (November 21, 2017)
- Fixes for Gate Junction initialization.

## 0.5.0 (November 18, 2017)
- Initial release.