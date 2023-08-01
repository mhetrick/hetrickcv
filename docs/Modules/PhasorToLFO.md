![Module](../Images/Modules/PhasorLFO.png)

# Phasor to LFO

The name is a bit of a misnomer as this module does no oscillation on its own. The name was chosen because the outputs are very useful for cyclical modulation signals, but they are not anti-aliased to provide useful audio outputs.

The design of this module is taken from [Generating Sound and Organizing Time](https://cycling74.com/books/go) by Graham Wakefield and Gregory Taylor.

Inside of this module, a phasor is run through a triangle shaper, a trapezoid shaper, and finally a sine shaper.

Inputs:

SKEW - Sets the angle of the triangle shaper, from ramp to triangle to saw.

WIDTH - Changes the table-top size of the trapezoid shaper.

SHAPE - Changes the slope of the angles from the trapezoid shaper. This heads toward infinity, creating pulse outputs.

CURVE - Blends between the raw trapezoid shaper output and an additional copy of the output that has been processed by a sine shaper.


Outputs:

MAIN - This is the output taken from the three sequential shapers.

TRI - This is an output taken from the first stage triangle shaper (before trapezoid or sinusoidal shaping).

PULSE - This output is unrelated to the shapers and is instead a pulse wave with a pulse width set by the WIDTH control. Try plugging this output back into one of the CV inputs to create assymetrical waves.