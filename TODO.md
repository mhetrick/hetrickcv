This is a list of ongoing ideas and plans for existing modules. If you would like to contribute to the project, feel free to open a pull request!

## General
- Add Phasor outputs to Phasor->Gates and Phase Driven Sequencer (i.e. output a phasor that is the width of a step on active steps). This can be used to trigger patterns.
- Go over all polyphonic modules and see what can be SIMD-optimized.
- Rewrite library functions using templates to allow easier SIMD optimization.
- Upgrade remaining monophonic modules to polyphonic (where appropriate).
- Replace usage of the global Gamma Domain with individual Domains for each module.
- Add useful right-click options to the various sequencers, such as Randomize Gates Only, Rotate Sequence Left/Right, etc.
- More tutorials!

## Modules
- Phasor Humanizer
- Phasor One-Shot/Burst Generator
- Phasor Splitter (1 input, up to 8 outputs. Switch mode changes output on reset. Phase mode creates a fractional phasor on each output.)
- Phasor Adder
- Phasor Step Pattern