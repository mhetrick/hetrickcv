### OR Logic (Gate Combiner)
This module can be used to combine many separate gate streams into one gate stream. The OR output is true if any of the inputs are above 1V, the NOR output is true if (and only if) all of the inputs are below 1V. The TRIGS output fires a 1ms trigger when any of the inputs go above 1V.

Patch Ideas:
- Route at least two outputs from the Boolean Logic module into the OR Logic inputs. The TRIGS output will now provide interesting rhythms.