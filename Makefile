SLUG = HetrickCV
VERSION = 0.5.4

# FLAGS will be passed to both C and C++ compiler
FLAGS +=
CFLAGS +=
CXXFLAGS +=

# Careful about linking to libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine.
LDFLAGS +=

# Add .cpp and .c files to the build
SOURCES = $(wildcard src/*.cpp)


# Must include the VCV plugin Makefile framework
include ../../plugin.mk


# Convenience target for including files in the distributable release
.PHONY: dist
dist: all
	mkdir -p dist/$(SLUG)
	cp LICENSE* dist/$(SLUG)/
	cp plugin.* dist/$(SLUG)/
	cp -R res dist/$(SLUG)/
	cd dist && zip -5 -r $(SLUG)-$(VERSION)-$(ARCH).zip $(SLUG)
