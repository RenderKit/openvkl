SHELL := /bin/bash

webpages := $(addprefix www/, $(addsuffix .html, index documentation examples downloads related_projects legal))
process_version := $(addprefix tmp/, $(addsuffix .md, readme_head))
images_png := $(addprefix images/, $(addsuffix .png, structured_spherical_coords vdb_structure vklExamples))

webimages := $(addprefix www/, $(images_png))

PANDOC := pandoc
PDMINVERSION := 2000000
PDOK := $(shell expr `$(PANDOC) --version|head -n 1|cut -d' ' -f 2| sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/' -e 's/^[0-9]\{5,6\}$$/&00/'` \>= $(PDMINVERSION))
ifneq "$(PDOK)" "1"
  $(error You need at least pandoc v2.0)
endif

all: www doc spec
www: $(webpages) www/stylesheet.css $(webimages)
wwwsync: www
doc: ../README.md
spec: tmp/openvkl-spec.rst

.PHONY: all www wwwsync doc clean spec

tmp/version: ../CMakeLists.txt
	sed -n -e "s/^project(openvkl VERSION \(.*\) LANGUAGES.*)/\1/p" $< | tr -d '\n' > $@

replace_version = sed -e "s/<OPENVKL_VERSION>/`cat tmp/version`/g" $< > $@

tmp/%.md: %.md tmp/version
	$(replace_version)

tmp/links.md: links.md

tmp/images_web.md: images.md
	sed -e "s@: @: https://openvkl.github.io/@" $< > $@

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
## Spec
## ---------------------------------------------------------------------------

tmp/api_spec.md: api.md images.md tmp/links.md
	$(PANDOC) $+ -t markdown-fenced_code_attributes -o $@

tmp/openvkl-spec.rst: tmp/api_spec.md
	$(PANDOC) --wrap=none --email-obfuscation=none -f markdown $+ -V select_$(basename $(@F)) --to rst -o $@
	rst2html tmp/openvkl-spec.rst tmp/openvkl-spec.html

## ----------------------------------------------------------------------------
## Webpages
## ----------------------------------------------------------------------------

markdown2web = $(PANDOC) --email-obfuscation=none -f markdown $(filter-out webtemplate.html,$+) --template webtemplate -V select_$(basename $(@F)) -o $@

$(webpages): tmp/links.md webtemplate.html

$(webimages): | wwwimgdir
wwwimgdir:
	@mkdir -p www/images

www/%.html:
	$(markdown2web)

www/stylesheet.css: stylesheet.css
	cp $< $@

tmp/api_html.md: api.md images.md tmp/links.md
	$(PANDOC) $+ --indented-code-classes=cpp -t markdown-fenced_code_attributes -o $@

www/index.html: teaser.html overview.md changelog.md support.md
www/documentation.html: tmp/api_html.md
www/examples.html: examples.md images.md
www/downloads.html: compilation.md
www/related_projects.html: related_projects.md
www/legal.html: legal.md

## ----------------------------------------------------------------------------
## Images
## ----------------------------------------------------------------------------

www/images/%: images/%
	cp $< $@

## ----------------------------------------------------------------------------
## Webpages sync
## ----------------------------------------------------------------------------

wwwsync:
ifndef WWW_REPO
	$(error environment variable WWW_REPO is not set)
endif
	rsync -arP www/* $(WWW_REPO)/

## ----------------------------------------------------------------------------
## Markdown
## ----------------------------------------------------------------------------

tmp/api.md: api.md tmp/links.md tmp/images_web.md
	$(PANDOC) $+ --indented-code-classes=cpp -t markdown-fenced_code_attributes -o $@

tmp/examples.md: examples.md tmp/links.md tmp/images_web.md
	$(PANDOC) $+ --indented-code-classes=cpp -t markdown-fenced_code_attributes -o $@

../README.md: tmp/readme_head.md overview.md changelog.md support.md tmp/api.md tmp/examples.md compilation.md related_projects.md tmp/links.md
	$(PANDOC) $+ -t gfm > $@

## ----------------------------------------------------------------------------
## Clean
## ----------------------------------------------------------------------------

clean:
	rm -rf www tmp changelog.md __pycache__
