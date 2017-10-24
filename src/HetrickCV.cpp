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
	p->addModel(createModel<Boolean2Widget>("HetrickCV", "HetrickCV", "Boolean 2", "Boolean Logic (2 In)"));

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables within this file or the individual module files to reduce startup times of Rack.
}
