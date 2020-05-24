all: build-models build-floors build-ceilings build-stairs build-columns

TinyRenderer/TinyRenderer: TinyRenderer/Makefile
	$(MAKE) -C TinyRenderer

OgreAssimp/OgreAssimpConverter: OgreAssimp/Makefile
	$(MAKE) -C OgreAssimp

build-models: TinyRenderer/TinyRenderer OgreAssimp/OgreAssimpConverter
	$(MAKE) -C models

build-floors: TinyRenderer/TinyRenderer OgreAssimp/OgreAssimpConverter
	$(MAKE) -C floors

build-ceilings: TinyRenderer/TinyRenderer OgreAssimp/OgreAssimpConverter
	$(MAKE) -C ceilings

build-stairs: TinyRenderer/TinyRenderer OgreAssimp/OgreAssimpConverter
	$(MAKE) -C stairs

build-columns: TinyRenderer/TinyRenderer OgreAssimp/OgreAssimpConverter
	$(MAKE) -C columns

up:
	$(MAKE) -C models $@
	$(MAKE) -C floors $@
	$(MAKE) -C ceilings $@
	$(MAKE) -C stairs $@
	$(MAKE) -C columns $@

clean:
	@rm -rf tmp/
	@rm -rf *.log
	@rm -fv `find . -name "*.bak"`
	@rm -fv `find . -name "*~"`
	@rm -fv `find tiled -name "*.tsx"`

	$(MAKE) -C models $@
	$(MAKE) -C floors $@
	$(MAKE) -C ceilings $@
	$(MAKE) -C stairs $@
	$(MAKE) -C columns $@

cleanall: clean
	$(MAKE) -C OgreAssimp $@
	$(MAKE) -C TinyRenderer $@

.PHONY: all build-models build-floors build-ceilings build-stairs build-columns up clean cleanall
