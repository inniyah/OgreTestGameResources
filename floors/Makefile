MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MKFILE_DIR := $(dir $(MKFILE_PATH))

TILES_ID := $(lastword $(subst /, ,$(MKFILE_DIR)))

OPACITY = 1.0

FLOOR_PNG_SOURCES = $(shell find . -maxdepth 1 -name "*.png" | grep -v ".iso.png$$" | grep -v ".up.png$$")
FLOOR_JPG_SOURCES = $(shell find . -maxdepth 1 -name "*.jpg")

OBJ_SOURCES = $(FLOOR_PNG_SOURCES:.png=.obj) $(FLOOR_JPG_SOURCES:.jpg=.obj)
ISO_TILES = \
	$(OBJ_SOURCES:.obj=_1.iso.png) \
	$(OBJ_SOURCES:.obj=_2.iso.png) \
	$(OBJ_SOURCES:.obj=_3.iso.png) \
	$(OBJ_SOURCES:.obj=_4.iso.png) \
	$(OBJ_SOURCES:.obj=.mesh)

all: $(OBJ_SOURCES) $(ISO_TILES) ../$(TILES_ID).tsx

add-new-objs:
	ls *.png *.jpg | grep -v ".iso.png$$" | cut -f1 -d'.' | sort --unique | while read -r F; do \
		../scripts/add_metadata.py "$${F}.obj" -d metadata.json -t $(TILES_ID); done

up: $(OBJ_SOURCES:.obj=.up.png)

../TinyRenderer/TinyRenderer:
	$(MAKE) -C ../TinyRenderer

../OgreAssimp/OgreAssimpConverter:
	$(MAKE) -C ../OgreAssimp

%.obj %.mtl: %.png
	cat floor.mtl.in \
		| sed -e 's|$${MATERIAL_TEXTURE}|$<|' \
		| sed -e 's|$${MATERIAL_NAME}|Material_$(shell echo "$<" | md5sum)|' \
		> $(basename $@).mtl
	cat floor.obj.in \
		| sed -e 's|$${MATERIAL_FILE}|$(basename $@).mtl|' \
		| sed -e 's|$${MATERIAL_NAME}|Material_$(shell echo "$<" | md5sum)|' \
		> $@
	../scripts/add_metadata.py $@ -d metadata.json -t $(TILES_ID) -D Texture $<

%.obj %.mtl: %.jpg
	cat floor.mtl.in \
		| sed -e 's|$${MATERIAL_TEXTURE}|$<|' \
		| sed -e 's|$${MATERIAL_NAME}|Material_$(shell echo "$<" | md5sum)|' \
		> $(basename $@).mtl
	cat floor.obj.in \
		| sed -e 's|$${MATERIAL_FILE}|$(basename $@).mtl|' \
		| sed -e 's|$${MATERIAL_NAME}|Material_$(shell echo "$<" | md5sum)|' \
		> $@
	../scripts/add_metadata.py $@ -d metadata.json -t $(TILES_ID) -D Texture $<

%.up.png: %.obj ../TinyRenderer/TinyRenderer
	../TinyRenderer/TinyRenderer -C ../TinyRenderer/config_up.ini -w $< $@
	optipng $@

%_1.iso.png: %.obj ../TinyRenderer/TinyRenderer
	../TinyRenderer/TinyRenderer -w    $< tmp_dir_$(notdir $@)
	../TinyRenderer/TinyRenderer -w -r $< tmp_rev_$(notdir $@)
	composite -dissolve 60 -gravity Center tmp_dir_$(notdir $@) tmp_rev_$(notdir $@) -alpha Set $@
	convert $@ -channel A -evaluate Multiply $(OPACITY) +channel $@
	../scripts/add_metadata.py $< -d metadata.json -t $(TILES_ID) -D IsoTile1 $@
	rm "tmp_dir_$(notdir $@)" "tmp_rev_$(notdir $@)"

%_2.iso.png: %.obj ../TinyRenderer/TinyRenderer
	../TinyRenderer/TinyRenderer -w    $< tmp_dir_$(notdir $@) -a 90
	../TinyRenderer/TinyRenderer -w -r $< tmp_rev_$(notdir $@) -a 90
	composite -dissolve 60 -gravity Center tmp_dir_$(notdir $@) tmp_rev_$(notdir $@) -alpha Set $@
	convert $@ -channel A -evaluate Multiply $(OPACITY) +channel $@
	../scripts/add_metadata.py $< -d metadata.json -t $(TILES_ID) -D IsoTile2 $@
	rm "tmp_dir_$(notdir $@)" "tmp_rev_$(notdir $@)"

%_3.iso.png: %.obj ../TinyRenderer/TinyRenderer
	../TinyRenderer/TinyRenderer -w    $< tmp_dir_$(notdir $@) -a 180
	../TinyRenderer/TinyRenderer -w -r $< tmp_rev_$(notdir $@) -a 180
	composite -dissolve 60 -gravity Center tmp_dir_$(notdir $@) tmp_rev_$(notdir $@) -alpha Set $@
	convert $@ -channel A -evaluate Multiply $(OPACITY) +channel $@
	../scripts/add_metadata.py $< -d metadata.json -t $(TILES_ID) -D IsoTile3 $@
	rm "tmp_dir_$(notdir $@)" "tmp_rev_$(notdir $@)"

%_4.iso.png: %.obj ../TinyRenderer/TinyRenderer
	../TinyRenderer/TinyRenderer -w    $< tmp_dir_$(notdir $@) -a 270
	../TinyRenderer/TinyRenderer -w -r $< tmp_rev_$(notdir $@) -a 270
	composite -dissolve 60 -gravity Center tmp_dir_$(notdir $@) tmp_rev_$(notdir $@) -alpha Set $@
	convert $@ -channel A -evaluate Multiply $(OPACITY) +channel $@
	../scripts/add_metadata.py $< -d metadata.json -t $(TILES_ID) -D IsoTile4 $@
	rm "tmp_dir_$(notdir $@)" "tmp_rev_$(notdir $@)"

%.mesh: %.obj ../OgreAssimp/OgreAssimpConverter
	../OgreAssimp/OgreAssimpConverter -uuid `echo '$(TILES_ID)/$<' | sha1sum | head -n1 | awk '{print $$1;}'` $<
	../scripts/add_metadata.py $< -d metadata.json -t $(TILES_ID) \
		-J $(dir $<)attributes.json -D 3DModel $< -D 3DMesh $@
	../scripts/add_metadata.py $< -d metadata.json -t $(TILES_ID) -D Material \
		`cat "$(dir $@)$(basename $(notdir $@)).material" | \
			grep '^material[[:space:]]*' | \
			sed -e 's|material[[:space:]]*||'`

%.iso.png: %_1.iso.png %_2.iso.png %_3.iso.png %_4.iso.png
	mkdir -p tmp
	@echo DIR: "'$(dir $@)'", FILE: "'$(basename $(basename $(notdir $@)))'", EXT: "'$(suffix $@)'"
	convert +append $+ $@
	optipng $@

../$(TILES_ID).tsx: $(ISO_TILES) metadata.json
	../scripts/generate_tsx.py -d metadata.json -i . -o .. -n "$(TILES_ID)"

update: $(OBJ_SOURCES)
	../scripts/update_attributes.py -J attributes.json -P '*.obj'

clean:
	@rm -fv `find . -maxdepth 1 -name "*.obj"`
	@rm -fv `find . -maxdepth 1 -name "*.mtl"`
	@rm -fv `find . -maxdepth 1 -name "*.iso.png"`
	@rm -fv `find . -maxdepth 1 -name "*.up.png"`
	@rm -fv `find . -maxdepth 1 -name "*.mesh"`
	@rm -fv `find . -maxdepth 1 -name "*.mesh.xml"`
	@rm -fv `find . -maxdepth 1 -name "*.material"`
	@rm -fv `find . -maxdepth 1 -name "*.skeleton"`
	@rm -fv `find . -maxdepth 1 -name "*.skeleton.xml"`
	@rm -fv `find . -maxdepth 1 -name "*.tsx"`
	@rm -fv `find . -maxdepth 1 -name "tmp_*.png"`

.PHONY: all add-new-objs up update clean
