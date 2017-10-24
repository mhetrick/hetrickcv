#include "rack.hpp"


using namespace rack;


extern Plugin *plugin;

////////////////////
// module widgets
////////////////////

struct DustWidget : ModuleWidget { DustWidget();};
struct CrackleWidget : ModuleWidget { CrackleWidget();};

struct ASRWidget : ModuleWidget { ASRWidget();};
struct TwoToFourWidget : ModuleWidget { TwoToFourWidget();};

struct FlipFlopWidget : ModuleWidget { FlipFlopWidget();};
struct Boolean2Widget : ModuleWidget { Boolean2Widget();};