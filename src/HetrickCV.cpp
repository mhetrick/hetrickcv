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

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables within this file or the individual module files to reduce startup times of Rack.
}
