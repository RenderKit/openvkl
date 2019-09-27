SHELL := /bin/bash

webpages := $(addprefix www/, $(addsuffix .html, index documentation examples downloads related_projects legal))
process_version := $(addprefix tmp/, $(addsuffix .md, readme_head))

PANDOC := pandoc
PDMINVERSION := 2000000
PDOK := $(shell expr `$(PANDOC) --version|head -n 1|cut -d' ' -f 2| sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/' -e 's/^[0-9]\{5,6\}$$/&00/'` \>= $(PDMINVERSION))
ifneq "$(PDOK)" "1"
  $(error You need at least pandoc v2.0)
endif

all: www doc
www: $(webpages) www/stylesheet.css
doc: ../README.md

.PHONY: all www doc clean realclean

tmp/version: ../CMakeLists.txt
	sed -n -e "s/^project(openvkl VERSION \(.*\) LANGUAGES.*)/\1/p" $< | tr -d '\n' > $@

replace_version = sed -e "s/<OPENVKL_VERSION>/`cat tmp/version`/g" $< > $@

tmp/%.md: %.md tmp/version
	$(replace_version)

tmp/links.md: links.md

changelog.md:
	ln -s ../CHANGELOG.md $@

## ----------------------------------------------------------------------------
## Directories
## ----------------------------------------------------------------------------

$(webpages) www/stylesheet.css: | wwwdir
wwwdir:
	@mkdir -p www

$(process_version) tmp/version: | tmpdir
tmpdir:
	@mkdir -p tmp

## ----------------------------------------------------------------------------
## Webpages
## ----------------------------------------------------------------------------

markdown2web = $(PANDOC) --email-obfuscation=none -f markdown $(filter-out webtemplate.html,$+) --template webtemplate -V select_$(basename $(@F)) -o $@

$(webpages): tmp/links.md webtemplate.html

www/%.html:
	$(markdown2web)

www/stylesheet.css: stylesheet.css
	cp $< $@

tmp/api_html.md: api.md tmp/links.md
	$(PANDOC) $+ --indented-code-classes=cpp -t markdown-fenced_code_attributes -o $@

www/index.html: overview.md changelog.md support.md
www/documentation.html: tmp/api_html.md
www/examples.html: examples.md
www/downloads.html: compilation.md
www/related_projects.html: related_projects.md
www/legal.html: legal.md

## ----------------------------------------------------------------------------
## Markdown
## ----------------------------------------------------------------------------

# Pandoc doesn't convert the alt text of an image to a caption in markdown, so we need to do it manually
convert_image_alt_to_caption = perl -0pe 's/!\[((?:\[.*?\]|.)+?)\]\((.*?)\)/![]\(\2)\n\1/sg'

tmp/api.md: api.md tmp/links.md
	$(PANDOC) $+ --indented-code-classes=cpp -t markdown-fenced_code_attributes -o $@

../README.md: tmp/readme_head.md overview.md changelog.md support.md tmp/api.md examples.md compilation.md related_projects.md tmp/links.md
	$(PANDOC) $+ -t gfm | $(convert_image_alt_to_caption) > $@

## ----------------------------------------------------------------------------
## Clean
## ----------------------------------------------------------------------------

clean:
	rm -rf www tmp changelog.md __pycache__

realclean: clean
	rm -irf images
