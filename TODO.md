This is a list of ongoing ideas and plans for existing modules. If you would like to contribute to the project, feel free to open a pull request!

- Add Phasor outputs to Phasor->Gates and Phase Driven Sequencer (i.e. output a phasor that is the width of a step on active steps). This can be used to trigger patterns.
- Go over all polyphonic modules and see what can be SIMD-optimized.
- Rewrite library functions using templates to allow easier SIMD optimization.
- Upgrade remaining monophonic modules to polyphonic (where appropriate).