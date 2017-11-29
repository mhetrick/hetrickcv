#include "HetrickCV.hpp"


// The plugin-wide instance of the Plugin class
Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	p->slug = "HetrickCV";
#ifdef VERSION
	plugin->version = TOSTRING(VERSION);
#endif

	p->website = "https://github.com/mhetrick/hetrickcv";
 	p->manual = "https://github.com/mhetrick/hetrickcv/blob/master/README.md";

	p->addModel(createModel<TwoToFourWidget>("HetrickCV", "2To4", "2 To 4 Mix Matrix", MIXER_TAG));
	p->addModel(createModel<AnalogToDigitalWidget>("HetrickCV", "AnalogToDigital", "Analog to Digital", LOGIC_TAG));	
	p->addModel(createModel<ASRWidget>("HetrickCV", "ASR", "ASR", SEQUENCER_TAG));
	p->addModel(createModel<BitshiftWidget>("HetrickCV", "Bitshift", "Bitshift", DISTORTION_TAG, EFFECT_TAG));
	p->addModel(createModel<BlankPanelWidget>("HetrickCV", "BlankPanel", "Blank Panel"));
	p->addModel(createModel<Boolean3Widget>("HetrickCV", "Boolean3", "Boolean Logic", LOGIC_TAG));
	p->addModel(createModel<ComparatorWidget>("HetrickCV", "Comparator", "Comparator", LOGIC_TAG));
	p->addModel(createModel<ContrastWidget>("HetrickCV", "Contrast", "Contrast", EFFECT_TAG));
	p->addModel(createModel<CrackleWidget>("HetrickCV", "Crackle", "Crackle", NOISE_TAG));
	p->addModel(createModel<DigitalToAnalogWidget>("HetrickCV", "DigitalToAnalog", "Digital to Analog", LOGIC_TAG));	
	p->addModel(createModel<DustWidget>("HetrickCV", "Dust", "Dust", NOISE_TAG, GRANULAR_TAG));	
	p->addModel(createModel<ExponentWidget>("HetrickCV", "Exponent", "Exponent", WAVESHAPER_TAG));	
	p->addModel(createModel<FlipFlopWidget>("HetrickCV", "FlipFlop", "Flip-Flop", LOGIC_TAG));
	p->addModel(createModel<FlipPanWidget>("HetrickCV", "FlipPan", "Flip Pan", PANNING_TAG));
	p->addModel(createModel<GateJunctionWidget>("HetrickCV", "Gate Junction", "Gate Junction", SWITCH_TAG, LOGIC_TAG));
	p->addModel(createModel<LogicCombineWidget>("HetrickCV", "Logic Combine", "OR Logic (Gate Combiner)", LOGIC_TAG));
	p->addModel(createModel<RandomGatesWidget>("HetrickCV", "RandomGates", "Random Gates", RANDOM_TAG));
	p->addModel(createModel<RotatorWidget>("HetrickCV", "Rotator", "Rotator", SWITCH_TAG));
	p->addModel(createModel<ScannerWidget>("HetrickCV", "Scanner", "Scanner", MIXER_TAG));
	p->addModel(createModel<WaveshapeWidget>("HetrickCV", "Waveshaper", "Waveshaper", WAVESHAPER_TAG, DISTORTION_TAG, EFFECT_TAG));
	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables within this file or the individual module files to reduce startup times of Rack.
}
