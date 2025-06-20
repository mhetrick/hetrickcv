# HetrickCV Changelog

## 2.5.3 (In Progress)
- Add polyphony to 1-Op Chaos, 2-Op Chaos, 3-Op Chaos, ASR, Binary Gate, Binary Noise, Chaotic Attractors, Clocked Noise, Comparator, Crackle, Delta, Dust, Gingerbread Chaos.
- Add missing polyphony tag to Scanner.

## 2.5.2
- Add Clock to Phasor.
- Add Normals.
- Add Phasor to Random.
- Add Polymetric Phasors.
- Phasor Swing's default Steps is now "4" for more intuitive behavior.

## 2.5.1
- Add Binary Counter.

## 2.5.0
- Add Phasor Freezer.
- Add Phasor Probability.
- Add Phasor Splitter.
- Add Trigonometric Shaper.
- Add missing port label to Phasor Reset.
- Cleanup SVG and class names again to match module slugs.

## 2.4.1
- FIX: The HCVPhasorDivMult class has been updated to use double precision on important calculations. This greatly improves the accuracy of the Phasor Timetable and Phasor Div/Mult modules and should eliminate drift in most use cases.
- Cleanup SVG names to all match C++ names.

## 2.4.0
- Added dark mode for all panels!
- Add Phasor Humanize module.
- Add Phasor Mixer module.
- Add Phasor Swing module.
- Add Phasor and !Gates outputs to Phasor->Gates 32 and 64.
- Add Run input to Phasor->Gates 32 and 64 along with Phase Driven Sequencer and Phase Driven Sequencer 32.
- Fix Phasor Randomizer, Phasor Shifter, Phasor Substep Shaper, and Phasor Stutter Steps knobs initializing with a 0 display.

## 2.3.2
- Fix Phasor to Euclidean step detector initialization.
- Fix text value entry for frequency knobs on Phasor Generator and Phasor Burst Generator.
- Phasor to Euclidean's Rotate parameter now shows values scaled by the number of steps.
- Phasor to Euclideans's Rotate parameter is now unquantized when Steps is unquantized.

## 2.3.1
- Add Phasor Burst Generator module.
- Fix some labels on Phasor Generator.
- Set Phasor Generator to load in Slow mode by default.
- Fix Finish output polyphony on Phasor Generator.
- Add better default values to Phasor Rhythm Group.
- Fix Phasor to Waveforms crash.
- Change Phasor to Clock default Steps value to 16. It was erroneously defaulting to 0 which caused a display bug.
- BREAKING CHANGE: Fix frequency knob state saving on Phasor Generator. The parameter was converted from variable min/max range to static +/- 1.0f internally. This will load an incorrect value on previously saved projects, but resaving will now maintain a stable/correct value.
- SEMI-BREAKING CHANGE: The Sinusoid output of Phasor to Waveforms was 90 degrees shifted from the triangle output. The two outputs now have the same phase.

## 2.3.0
- Version 2.3.0 introduces a suite of phasor-based sequencing and Phase Distortion synthesis tools.
    - Add Phasor Generator module.
    - Add Phasor Geometry module.
    - Add Phasor Randomizer module.
    - Add Phasor Ranger module.
    - Add Phasor Shaper module.
    - Add Phasor Sub-Step Shaper module.
    - Add Phasor->Euclidean module.
    - Add Phasor->Gates module.
    - Add Phasor->Gates 32 module.
    - Add Phasor->Gates 64 module.
    - Add Phasor->LFO module.
    - Add Phasor Reset module.
    - Add Phase Driven Sequencer module.
    - Add Phase Driven Sequencer 32 module.
    - Add Phasor->Waveforms module.
    - Add Phasor Quadrature module.
    - Add Phasor Octature module.
    - Add Phasor Timetable module.
    - Add Phasor Stutter module.
    - Add Phasor Shift module.
    - Add Phasor Divider & Multiplier module.
    - Add Phasor->Clock module.
    - Add Phasor Rhythm Group module.
    - Add Phasor Analyzer module.
- Add Gate Delay module.
- Complete revision of User Manual and Documentation.
- Add Thomas chaotic attractor to 1-Op Chaos (https://sprott.physics.wisc.edu/chaos/symmetry.htm).
- Make Mode knobs on 1-Op Chaos, 2-Op Chaos, Chaotic Attactors, and Clocked Noise snap to values and print the current mode.
- Add "Low-frequency Oscillator" tag to Feedback Sine Chaos.
- Scanner is now polyphonic.
- Tweak LED positions on some modules.
- Add LEDs to noise and chaos modules.
- BREAKING CHANGE: in 2.1.0, Gate Combiner and Random Gates had their trigger times reduced from 1ms to a single sample (another BREAKING CHANGE, but this was not listed as such). This is not within VCV voltage standards and failed to trigger a number of modules. Now, all HetrickCV modules use the 1ms trigger standard (including Comparator and Delta).
- BREAKING CHANGE: All unipolar gate outputs are now 0-10V instead of 0-5V. Bipolar gate outputs have not changed.

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