#include "HetrickCV.hpp"


// The plugin-wide instance of the Plugin class
Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	p->slug = "HetrickCV";
#ifdef VERSION
	plugin->version = TOSTRING(VERSION);
#endif

	//p->addModel(createModel<E340Widget>("ESeries", "E-Series", "E340", "E340 Cloud Generator"));
	p->addModel(createModel<DustWidget>("HetrickCV", "HetrickCV", "Dust", "Dust"));
	p->addModel(createModel<CrackleWidget>("HetrickCV", "HetrickCV", "Crackle", "Crackle"));
	p->addModel(createModel<ASRWidget>("HetrickCV", "HetrickCV", "ASR", "ASR"));
	p->addModel(createModel<TwoToFourWidget>("HetrickCV", "HetrickCV", "2To4", "2 To 4 Mix Matrix"));
	p->addModel(createModel<FlipFlopWidget>("HetrickCV", "HetrickCV", "FlipFlop", "Flip-Flop"));
	p->addModel(createModel<Boolean2Widget>("HetrickCV", "HetrickCV", "Boolean2", "Boolean Logic (2 In)"));
	p->addModel(createModel<Boolean3Widget>("HetrickCV", "HetrickCV", "Boolean3", "Boolean Logic (3 In)"));
	p->addModel(createModel<LogicInvertWidget>("HetrickCV", "HetrickCV", "Logic Invert", "NOT Logic (Gate Inverter)"));
	p->addModel(createModel<LogicCombineWidget>("HetrickCV", "HetrickCV", "Logic Combine", "OR Logic (Gate Combiner)"));
	p->addModel(createModel<RotatorWidget>("HetrickCV", "HetrickCV", "Rotator", "Rotator"));
	p->addModel(createModel<ScannerWidget>("HetrickCV", "HetrickCV", "Scanner", "Scanner"));
	p->addModel(createModel<RandomGatesWidget>("HetrickCV", "HetrickCV", "RandomGates", "Random Gates"));

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables within this file or the individual module files to reduce startup times of Rack.
}
