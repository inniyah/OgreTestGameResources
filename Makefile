all: build-models build-floors build-ceilings build-stairs build-columns

DESTDIR ?= resources

install: all
	mkdir -p "$(DESTDIR)/models"
	cp -v `find models floors ceilings stairs columns -iname '*.mesh'` \
		"$(DESTDIR)/models"

	mkdir -p "$(DESTDIR)/materials"
	cp -v `find materials models floors ceilings stairs columns -iname '*.material'` \
		"$(DESTDIR)/materials"

	mkdir -p "$(DESTDIR)/textures"
	cp -v `find materials floors ceilings -type f \( \( -iname "*.png" -or -iname "*.jpg" \) -and -not \( -iname "*.iso.png" -or -iname "*.up.png" \) \)` \
		"$(DESTDIR)/textures"

	mkdir -p "$(DESTDIR)/tiled"
	tar cf - `find materials models floors ceilings stairs columns -iname '*.iso.png'` | \
		tar xvf - -C "$(DESTDIR)/tiled"
	cp -v *.tsx "$(DESTDIR)/tiled"

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

update:
	$(MAKE) -C models $@
	$(MAKE) -C floors $@
	$(MAKE) -C ceilings $@
	$(MAKE) -C stairs $@
	$(MAKE) -C columns $@

up:
	$(MAKE) -C models $@
	$(MAKE) -C floors $@
	$(MAKE) -C ceilings $@
	$(MAKE) -C stairs $@
	$(MAKE) -C columns $@

clean:
	@rm -rf tmp/
	@rm -f *.log
	@rm -f *.tsx
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
