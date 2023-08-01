![Module](../Images/Modules/PhasorAnalyzer.png)

# Phasor Analyzer

This handy utility reports sample-accurate data regarding the state of a phasor.

DIRECTION: If the phasor is ascending, output 5V. If the phasor is descending, output -5V. Otherwise, output 0 if the phasor isn't moving.

ACTIVE: If the phasor is moving, output 10V.

**NOTE**: If you are using extremely slow phasors (around 1,000 seconds/cycle), a rounding error will rapidly change the phasor's analyzed slope down to 0.0. This can result in bursts of noise on the Direction and Active outputs.

Reset/Jump: These two outputs produce a trigger whenever the phasor's slope shifts dramatically. The Reset output uses a traditional algorithm to check if the change in slope is greater than 0.5 (i.e the phasor has reset from 1.0 to 0.0). This might miss if the phasor resets from, say, 0.25. 

The Jump detector uses a proportional slope check, so it is more sensitive to resets. Additionally, it will often fire if the phasor is paused and then starts again.

Kink: This outputs the difference between the slope this sample and the slope from the previous sample. This is 0.0 on a linear phasor until it hits a reset point. It will also produce output on shaped phasors, phasor reverses, phasor stutters, etc.