#include "rack.hpp"

#pragma once

#include "HetrickUtilities.hpp"

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
struct Boolean3Widget : ModuleWidget { Boolean3Widget();};
struct GateJunctionWidget : ModuleWidget { GateJunctionWidget();};
struct LogicCombineWidget : ModuleWidget { LogicCombineWidget();};

struct AnalogToDigitalWidget : ModuleWidget { AnalogToDigitalWidget();};
struct DigitalToAnalogWidget : ModuleWidget { DigitalToAnalogWidget();};
struct BitshiftWidget : ModuleWidget { BitshiftWidget();};

struct RotatorWidget : ModuleWidget { RotatorWidget();};
struct ScannerWidget : ModuleWidget { ScannerWidget();};
struct RandomGatesWidget : ModuleWidget { RandomGatesWidget();};

struct ContrastWidget : ModuleWidget { ContrastWidget();};
struct ExponentWidget : ModuleWidget { ExponentWidget();};
struct WaveshapeWidget : ModuleWidget { WaveshapeWidget();};

struct FlipPanWidget : ModuleWidget { FlipPanWidget();};


struct BlankPanelWidget : ModuleWidget 
{ 
    SVGPanel *panel1;
	SVGPanel *panel2;
    SVGPanel *panel3;
	SVGPanel *panel4;
    SVGPanel *panel5;
    BlankPanelWidget();
    void step() override;
	Menu *createContextMenu() override;
};