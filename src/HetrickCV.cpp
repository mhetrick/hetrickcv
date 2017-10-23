#include "HetrickCV.hpp"


// The plugin-wide instance of the Plugin class
Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	plugin->slug = "HetrickCV";
	plugin->name = "HetrickCV";
	plugin->homepageUrl = "https://github.com/VCVRack/Tutorial";
#ifdef VERSION
	plugin->version = TOSTRING(VERSION);
#endif

	createModel<DustWidget>(plugin, "Dust", "Dust");
	createModel<CrackleWidget>(plugin, "Crackle", "Crackle");

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables within this file or the individual module files to reduce startup times of Rack.
}
