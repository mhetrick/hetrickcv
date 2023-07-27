### Boolean Logic
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