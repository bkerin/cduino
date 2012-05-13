# This Makefile handles jobs related to the development of the package itself,
# the useful content is in generic.mk (see manual.html for details).

# vim: foldmethod=marker

# Delete files produced by rules the commands of which return non-zero.
.DELETE_ON_ERROR:
# Disable all suffix rules.
.SUFFIXES:

WEB_SSH := brittonk@box201.bluehost.com
WEB_ROOT := public_html/cduino

# Set up a new module (create directory and install minimum requires sym links
# and minimal Makefile).  The make variable NEW_MODULE_NAME must be set
# (probably from the command line, e.g. 'make new_module NEW_MODULE_NAME=foo').
.PHONY: new_module
new_module:
	[ -n "$(NEW_MODULE_NAME)" ] || \
          (echo NEW_MODULE_NAME not set 1>&2 && false)
	mkdir $(NEW_MODULE_NAME)
	cd $(NEW_MODULE_NAME) && \
          ln -s ../ATmegaBOOT_168_atmega328.hex && \
          ln -s ../generic.mk && \
          ln -s ../lock_and_fuse_bits_to_avrdude_options.perl && \
          echo 'include generic.mk' >Makefile

# Clean up all the modules.
.PHONY: clean_all_modules
clean_all_modules:
	find . -mindepth 1 -maxdepth 1 -type d \
               -exec $(MAKE) -C \{\} -R clean \;
	# This is needed because this lesson requires a non-bootloader upload
	# method at the moment (FIXME: remove this when fixed upstream)
	$(MAKE) -C lesson12 -R 'UPLOAD_METHOD = AVRISPmkII' clean

# Push all the latest changes to gitorious (tolerate untracked files but
# not changed files or uncommited changes).
.PHONY: git_push
git_push:
	! (git status | grep 'Changed but not updated:')
	! (git status | grep 'Changes to be committed:')
	git push origin master

# Verify that all the source html files except the lessons and the somewhat
# anomolous but useful blink.c in the crosslinked sources directory appear
# to be linked to from somewhere in the top level API document.
check_api_doc_completeness: apis_and_sources.html xlinked_source_html
	for SF in $$(ls -1 xlinked_source_html/*.[ch].html); do \
          echo $$SF | perl -n -e 'not m/\/lesson.*/ or exit 1' || continue; \
          echo $$SF | perl -n -e 'not m/blink\.c/ or exit 1' || continue; \
          grep -q -P "\Q\"$$SF\"\E" $< || ERROR="no link to $$SF in $<"; \
        done; \
        if [ -n "$$ERROR" ]; then echo $$ERROR 1>&2 && false; else true; fi

check_lesson_doc_completeness: lessons.html xlinked_source_html
	for SF in $$(ls -1 xlinked_source_html/*.[ch].html); do \
          echo $$SF | perl -n -e 'm/\/lesson.*/ or exit 1' || continue; \
          grep -q -P "\Q\"$$SF\"\E" $< || ERROR="no link to $$SF in $<"; \
        done; \
        if [ -n "$$ERROR" ]; then echo $$ERROR 1>&2 && false; else true; fi

# FIXME: would be nice to uniqueify function names between lessons and API so
# we didn't end up with messy multiple way links in the HTML-ized headers.
# Generate cross linked header and source files as a simple form of API
# documentation (and for browsable source).
.PHONY: xlinked_source_html
xlinked_source_html:
	rm -rf $@
	mkdir $@
	find . -name "*.[ch]" -exec cp \{\} $@ \;
	cd $@ ; source-highlight --gen-references=inline *.[ch]
	# This gets a bit wild.  We use ||= to take advantage of non-lexical
	# vars which aren't reinitialized every time through implicit -p loop,
	# to save prohibitively expensive per-line recomputation of known file
	# name regular expression.  The link-for-filename substitution has some
	# negative look-behind and look-ahead assertions which prevent partial
	# file names from matching and trim off some file name text that
	# source-highlight itself produces.
	cd $@ ; \
          perl -p -i \
            -e '$$frgx ||= join("|", split("\n", `ls -1 *.[ch]`));' \
            -e '$$fre_dot_escape_done ||= ($$frgx =~ s/\./\\./g);' \
            -e 's/((?<!\w)(?:$$frgx)(?!\.html|\:\d+))' \
            -e ' /<a href="$$1.html">$$1<\/a>/gx;' \
            *.html
	rm $@/*.[ch]
	rm $@/tags

# Note this doesn't nuke old pages with different names.
.PHONY: upload_html
upload_html: git_push xlinked_source_html check_api_doc_completeness
	scp *.html $(WEB_SSH):$(WEB_ROOT)
	ssh $(WEB_SSH) rm -rf '$(WEB_ROOT)/xlinked_source_html/'
	scp -r xlinked_source_html $(WEB_SSH):$(WEB_ROOT)
	ssh $(WEB_SSH) ln -s --force home_page.html $(WEB_ROOT)/index.html

# Make a release targzball.  The make variable VERSION must be set (probably
# from the command line, e.g. make targzball VERSION=0.42.42').
.PHONY: targzball
targzball: clean_all_modules
	[ -n "$(VERSION)" ] || (echo VERSION not set 1>&2 && false)
	cd /tmp ; cp -r $(shell pwd) .
	cd /tmp/cduino && rm -rf .git .gitignore ; cd .. ; \
          mv cduino cduino-$(VERSION) ; \
          tar czvf cduino-$(VERSION).tgz cduino-$(VERSION)

# Check for Consistency of Certain Files Across Projects {{{1

# Any SAMESY_FILES appearing in more than one of SAMESY_DIRS must be the
# same in each directory in which they occur (though they can be totally
# absent in some dirs and that is ok).

SAMESY_FILES := generic.mk \
                adc.h \
                adc.c \
                uart.h \
                uart.c

SAMESY_DIRS := ~/projects/temp_logger/microcode/ \
               ~/projects/smartcord/microcode/ \
               ~/projects/cduino/ \
               ~/projects/cduino/adc \
               ~/projects/cduino/uart

# WARNING: this really pushes the shell with a potentially giant cross
# product as a command line.
SAMESY_CHECK_CODE := \
  $(foreach sf,$(SAMESY_FILES), \
    $(foreach dira,$(SAMESY_DIRS), \
      $(foreach dirb,$(SAMESY_DIRS), \
        ( \
          (! [ -a $(dira)/$(sf) ]) || \
          (! [ -a $(dirb)/$(sf) ]) || \
          diff -u $(dira)/$(sf) $(dirb)/$(sf) \
        ) &&))) \
  true

# For debugging:
.PHONY: escc
escc:
	@echo '$(SAMESY_CHECK_CODE)'

.PHONY: samesys
samesys:
	$(SAMESY_CHECK_CODE)

# Make sure our samsys desires are satisfied before doing anything important.
upload_html targzball: samesys

# }}}


# Upload the targzball and documentation to the web site.  The unstable
# snapshot that we provide is uploaded first, since it should never be
# behind the stable version.
.PHONY: upload
upload: targzball upload_html update_unstable git_push
	scp /tmp/cduino-$(VERSION).tgz $(WEB_SSH):$(WEB_ROOT)/releases/
	ssh $(WEB_SSH) rm -f '$(WEB_ROOT)/releases/LATEST_IS_*'
	ssh $(WEB_SSH) ln -s --force cduino-$(VERSION).tgz \
            $(WEB_ROOT)/releases/LATEST_IS_cduino-$(VERSION).tgz

# Update the unstable snapshot by building and uploading an unstable targzball.
.PHONY: update_unstable
update_unstable:
	ssh $(WEB_SSH) mkdir -p $(WEB_ROOT)/unstable/
	ssh $(WEB_SSH) rm -f '$(WEB_ROOT)/unstable/cduino_unstable*'
	cd /tmp ; cp -r $(shell pwd) . && \
          UNSTABLE_NAME=cduino_unstable_`date +%Y-%m-%d-%H-%M-%S` && \
          mv cduino $$UNSTABLE_NAME && \
          tar czvf $$UNSTABLE_NAME.tgz $$UNSTABLE_NAME && \
          scp $$UNSTABLE_NAME.tgz $(WEB_SSH):$(WEB_ROOT)/unstable/

.PHONY: view_web_page
ifndef WEB_BROWSER
  view_web_page: WEB_BROWSER := firefox
endif
view_web_page:
	$(WEB_BROWSER) home_page.html

clean: clean_all_modules
	rm -rf xlinked_source_html
