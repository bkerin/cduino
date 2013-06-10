# This Makefile handles jobs related to the development of the package itself,
# the useful content is in generic.mk (see manual.html for details).

# vim: foldmethod=marker

# FIXME: I think there are some more things I normally do to make Make more
# sane now, clone them in from other projects.

# Delete files produced by rules the commands of which return non-zero.
.DELETE_ON_ERROR:
# Disable all suffix rules.
.SUFFIXES:

SHELL= /bin/bash

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
          ln -s ../optiboot_atmega328.hex && \
          ln -s ../generic.mk && \
          ln -s ../lock_and_fuse_bits_to_avrdude_options.perl && \
          ln -s ../guess_arduino_attribute.perl && \
          ln -s ../util.h && \
          echo 'include generic.mk' >Makefile

# Do a clean, then a full program build, then another clean in each module
# directory.  The idea is to try to catch interface changes.  This isn't a
# complete test of course, but it helps catch obvious stuff at least.  FIXME:
# At the moment we don't test dio because it comes with some fatal warning
# thingies, or lesson12 because it requires the AVRISPmkII programming method
# (though with the Uno it supposedly doesn't anymore :).  We should require a
# manually placed stamp to represent that these modules have been checked out.
.PHONY: build_all_programs
build_all_test_programs: MODULE_DIRS = \
  $(shell echo $$(for gmk in $$(find . -name "generic.mk" -print | \
                                  grep --invert-match '^\./generic.mk'); do \
                    dirname $$gmk; \
                  done))
build_all_test_programs:
	(for md in $(MODULE_DIRS); do \
           echo "$$md" | grep --silent 'dio$$' || \
           echo "$$md" | grep --silent 'lesson12$$' || \
           ( \
             $(MAKE) -C $$md -R clean && \
             $(MAKE) -R -C $$md program_to_upload.out && \
             $(MAKE) -C $$md -R clean \
           ) || \
           exit 1; \
         done)

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
git_push: build_all_test_programs clean_all_modules
	! (git status | grep 'Changed but not updated:')
	! (git status | grep 'Changes to be committed:')
	git push origin master

# Verify that all the source html files except the lessons and the somewhat
# anomolous but useful blink.c and some other junk that creeps in from
# Arduino lib in the crosslinked sources directory appear to be linked to
# from somewhere in the top level API document.  NOTE: FNP below is the
# "File No Path".  NOTE: the check for the text of the link depends on
# finding that text on a line with nothing else but white space.
check_api_doc_completeness: apis_and_sources.html \
                            xlinked_source_html \
                            build_all_test_programs
	for SF in $$(ls -1 xlinked_source_html/*.[ch].html); do \
          FNP=`echo $$SF | perl -p -e 's/.*\/(.*)\.html/$$1/'`; \
          echo $$SF | perl -n -e 'not m/\/lesson.*/ or exit 1' || continue; \
          echo $$SF | perl -n -e 'not m/\/blink\.c/ or exit 1' || continue; \
          grep -q -P "\Q\"$$SF\"\E" $< || \
            ERROR="probably no link to $$SF in $<"; \
          grep -q -P "^\s*\Q$$FNP\E\s*$$" $< || \
            ERROR="probably no link named $$FNP in $<"; \
        done; \
        if [ -n "$$ERROR" ]; then echo $$ERROR 1>&2 && false; else true; fi

# Verify that the different related source files that go into a module
# mention each other in the way that we like them to.
#check_api_test_driver_interface_implementation_crossrefs: \
#  xlinked_source_html build_all_test_programs
check_api_test_driver_interface_implementation_crossrefs: \
  xlinked_source_html
	( \
          for SF in $$(ls -1 xlinked_source_html/*.[ch].html); do \
            ./check_crossrefs.perl $$SF || exit 1; \
          done \
        )

check_lesson_doc_completeness: lessons.html \
                               xlinked_source_html \
                               build_all_test_programs
	for SF in $$(ls -1 xlinked_source_html/*.[ch].html); do \
          echo $$SF | perl -n -e 'm/\/lesson.*/ or exit 1' || continue; \
          grep -q -P "\Q\"$$SF\"\E" $< || ERROR="no link to $$SF in $<"; \
        done; \
        if [ -n "$$ERROR" ]; then echo $$ERROR 1>&2 && false; else true; fi

# FIXME: would be nice to uniqueify function names between lessons and API so
# we didn't end up with messy multiple way links in the HTML-ized headers.
# I think the lesson-API conflicts are pretty much gone, lots of lessons
# still have overlapping tags though.

# Generate cross linked header and source files as a simple form of API
# documentation (and for browsable source).
.PHONY: xlinked_source_html
xlinked_source_html:
	rm -rf $@
	mkdir $@
	find . -name "*.[ch]" -exec cp \{\} $@ \;
	./nuke_non_highlightable_files.perl $@;
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

# FIXME: could add Make compile timish checks that the proper vars are
# defined on the command line when one of the convenience targets like
# upload_html are used as was done in beaglebone_symbols project (currently
# VERSION must be given on command line in this project, other symbols like
# WEB_SSH and WEB_ROOT are defined in this Makefile but not in the one for
# the beaglebone_symbol project).

# FIXME: this should run checklink script after install
# Note this doesn't nuke old pages with different names.
.PHONY: upload_html
upload_html: git_push xlinked_source_html check_api_doc_completeness
	scp *.html $(WEB_SSH):$(WEB_ROOT)
	ssh $(WEB_SSH) rm -rf '$(WEB_ROOT)/xlinked_source_html/'
	scp -r xlinked_source_html $(WEB_SSH):$(WEB_ROOT)
	ssh $(WEB_SSH) ln -s --force home_page.html $(WEB_ROOT)/index.html

# Make a release targzball.  The make variable VERSION must be set (probably
# from the command line, e.g. make targzball VERSION=0.42.42').
# FIXME: get rid of these hard coded cduino distribution names and replace
# with refs to the dir as we do the first time below (make shell var for long
# command).
.PHONY: targzball
targzball: build_all_test_programs clean_all_modules xlinked_source_html
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

# This is useful if we need to ensure that arbitrary sets of files are
# identical.  NOTE: SAMESY_CHECK_CODE could in theory be implemented in
# terms of this, if it was worth doing.  This make function which expands to
# shell code that fails if all of the files listed in the space- (NOT comma-)
# seperated list of files given as an argument aren't identical.  This check
# will fail if any of the mentioned files are missing as well. NOTE:
# somewhat weirdly we need to use '||' after the subshell '(echo... exit
# 1) to prevent the shell from somehow eating our diff output and error
# message... I suppose because '||' guarantees via short-circuit execution
# that the command before it is fully executed, and '&&' doesn't.  Then we
# need another 'exit 1' to exit the shell executing the loop, rather than
# just the subshell (because for doesn't care if commands return non-zero).
DIFFN = for fa in $(1); do \
          for fb in $(1); \
            do diff -u $$fa $$fb || \
               (echo "Files $(1) are not all identical" 1>&2 && exit 1) || \
               exit 1; \
          done; \
        done

# For debugging:
.PHONY: escc
escc:
	@echo '$(SAMESY_CHECK_CODE)'

# Pronounced "samsees" or "samsies" or however you write that pronounciation :)
.PHONY: samesys
samesys:
	$(SAMESY_CHECK_CODE)

# Make sure our samsys desires are satisfied before doing anything important.
# FIXME: sync back up and reenble this!
#upload_html targzball: samesys

# }}}

# Upload the targzball and documentation to the web site.  The unstable
# snapshot that we provide is uploaded first, since it should never be
# behind the stable version.
# FIXME: might want to generalize away from hardcoded cduino name as was
# done in beaglebone_symbols project.
.PHONY: upload
upload: targzball upload_html update_unstable git_push
	scp /tmp/cduino-$(VERSION).tgz $(WEB_SSH):$(WEB_ROOT)/releases/
	ssh $(WEB_SSH) rm -f '$(WEB_ROOT)/releases/LATEST_IS_*'
	ssh $(WEB_SSH) ln -s --force cduino-$(VERSION).tgz \
            $(WEB_ROOT)/releases/LATEST_IS_cduino-$(VERSION).tgz

# Update the unstable snapshot by building and uploading an unstable targzball.
.PHONY: update_unstable
update_unstable: build_all_test_programs clean_all_modules
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

# This target is intended to be used to dump a large part of the official
# arduino libraries into a module subdirectory, to make it easier to
# clone-and-adapt code designed for those libraries (including parts of the
# libraries themselves).  Since this should be done from a module subdirectory
# it must be invoked like this: 'make -f ../Makefile grab_arduino_libs'.
# Note that compiling all this junk together will result in a program too
# big to upload (FIXME: WHY? cpp constructors are always in scope or just
# too much static data or what? Is there a way to strip more junk out?) See
# arduino_lib_code/README for more details.
.PHONY:
grab_arduino_libs:
	cp --backup=numbered \
          ../arduino_lib_code/*.h \
          ../arduino_lib_code/*.cpp \
          ../arduino_lib_code/*.c \
          .
	@echo NOTE: backup files or any existing files have been made, they
	@echo all end in \"~\".
	@echo
	@echo NOTE: Compiling all that junk together seems to result in a
	@echo program too big for the chip.  So you will have to go through
	@echo and weed out whatever junk your module test program does not need.

clean: clean_all_modules
	rm -rf xlinked_source_html
