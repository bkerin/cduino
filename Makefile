# This Makefile handles jobs related to the development of the package itself,
# the useful content is in generic.mk (see manual.html for details).

# vim: foldmethod=marker

# Make Make more sane and useful {{{1

# Make the user set the .DEFAULT_GOAL variable if they want a default rule,
# rather than just having it be the first rule in the Makefile.
default_default_rule:
	@echo Sorry, this Makefile does not support default rules 2>&1 && false

# Delete files produced by rules the commands of which return non-zero.
.DELETE_ON_ERROR:

# Don't let make parallelize anything.
.NOTPARALLEL:

# Don't automagically remove intermediate files.
.SECONDARY:

# Enable a second expsion of variables in the prerequisite parts of rules.
# So $$(OBJS) will give us what we want if we have made a target- or
# pattern-local version of OBJS, for example.
.SECONDEXPANSION:

# Disable all suffix rules.
.SUFFIXES:

SHELL= /bin/bash

# }}}1

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
# directory.  The idea is to try to catch interface changes.  This isn't
# a complete test of course, but it helps catch obvious stuff at least.
# FIXME: At the moment we don't build dio because it comes with some
# fatal warning thingies, or lesson12 because it requires the AVRISPmkII
# programming method (though with the Uno it supposedly doesn't anymore :).
# We should perhaps require a manually placed stamp to represent that these
# modules have been checked out.
.PHONY: build_all_programs
build_all_test_programs: MODULE_DIRS = \
  $(shell echo $$(for gmk in $$(find . -name "generic.mk" -print | \
                                  grep --invert-match '^\./generic.mk'); do \
                    dirname $$gmk; \
                  done))
build_all_test_programs:
	# Because many targets normally do a full rebuild of all the tests, we
	# support this envirnoment variable which can be used to circumvent our
	# own careful checking.  When appropriate :)
	test -n "$$SKIP_TEST_PROGRAMS_BUILD" || ( \
          for md in $(MODULE_DIRS); do \
            echo "$$md" | grep --silent 'dio$$' || \
            echo "$$md" | grep --silent 'lesson12$$' || ( \
              $(MAKE) -rR -C $$md clean && \
              $(MAKE) -rR -C $$md program_to_upload.out && \
              $(MAKE) -rR -C $$md clean \
            ) || \
            exit 1; \
          done \
        )

# Clean up all the modules.
.PHONY: clean_all_modules
clean_all_modules:
	find . -mindepth 1 -maxdepth 1 -type d \
               -exec $(MAKE) -rR -C \{\} clean \;
	# This is needed because this lesson requires a non-bootloader upload
	# method at the moment (FIXME: remove this when fixed upstream)
	$(MAKE) -C lesson12 -rR 'UPLOAD_METHOD = AVRISPmkII' clean

# Push all the latest changes to gitorious (tolerate untracked files but
# not changed files or uncommited changes).
.PHONY: git_push
git_push: build_all_test_programs clean_all_modules
	! (git status | grep 'Changed but not updated:')
	! (git status | grep 'Changes to be committed:')
	git push origin master

# Verify that all the source html files except the lessons, private headers,
# and the somewhat anomolous but useful blink.c and some other junk that
# creeps in from Arduino lib in the crosslinked sources directory appear
# to be linked to from somewhere in the top level API document.  NOTE: FNP
# below is the "File No Path".  NOTE: the check for the text of the link
# depends on finding that text on a line with nothing else but white space.
check_api_doc_completeness: apis_and_sources.html \
                            xlinked_source_html \
                            build_all_test_programs
	for SF in $$(ls -1 xlinked_source_html/*.[ch].html); do \
          FNP=`echo $$SF | perl -p -e 's/.*\/(.*)\.html/$$1/'`; \
          echo $$SF | perl -n -e 'not m/\/lesson.*/ or exit 1' || continue; \
          echo $$SF | perl -n -e 'not m/\/blink\.c/ or exit 1' || continue; \
          echo $$SF | perl -n -e 'not m/\/\w+_private\.[hc]/ or exit 1' || \
            continue; \
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

check_lesson_doc_completeness: lessons.html            \
                               xlinked_source_html     \
                               build_all_test_programs
	for SF in $$(ls -1 xlinked_source_html/*.[ch].html); do             \
          echo $$SF | perl -n -e 'm/\/lesson.*/ or exit 1' || continue;     \
          grep -q -P "\Q\"$$SF\"\E" $< || ERROR="no link to $$SF in $<";    \
        done;                                                               \
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
	# To exclude some model code that we keep in subdirs in the modules we
	# use the -maxdepth 2 option to find.
	find . -maxdepth 2 -name "*.[ch]" -exec cp \{\} $@ \;
	./nuke_non_highlightable_files.perl $@;
	# It doesn't quite work to let source-highlight run ctags for us, since
	# we want to remove a few spurious tags that get generated due to
	# X macros and such.  We also force the language selection for .h files
	# to C (instead of the default C++), and add a C language regex that
	# creates enumerated constant tags for capitalized X macro arguments
	# (since we use X macros for enumerated types, and comment the
	# enumerated constants at this point).
	cd $@ &&                                                         \
          ctags                                                          \
            --excmd=n --tag-relative=yes                                 \
            --langmap=C:+.h                                              \
            --regex-C='/_?X[[:space:]]*\(([A-Z0-9_]+)\)/\1/enumerator/e' \
            *.[ch]
	# Now we filter out some tag lines that ctags produces under the
	# influence of X macro drugs :)  Note that we hard-wire an expression
	# that matches what we use for our X macro list define.
	cd $@ &&                                                          \
	  perl                                                            \
            -n -i                                                         \
            -e 'm/^(?:\w+_)?X\s+.*\s+.*\s+d\s+.*$$/ or '                  \
            -e 'm/^\w+_(?:RESULT|ERROR)_CODES\s+.*\s+e\s+enum:.*$$/ or '  \
            -e 'print' \
             tags
	# The --ctags and --ctags-file options used here together tell
	# source-highlight not to run ctags itself, but instead use the already
	# generated tags file.
	cd $@ &&                                        \
          source-highlight --gen-references=inline      \
                           --ctags='' --ctags-file=tags \
                           *.[ch]

	# Change mentions of source files in the source-highlight-generated
	# HTML into hyperlinks.  This gets a bit wild.  We use ||= to take
	# advantage of non-lexical vars which aren't reinitialized every time
	# through implicit -p loop, to save prohibitively expensive per-line
	# recomputation of known file name regular expression.  The
	# link-for-filename substitution has some negative look-behind and
	# look-ahead assertions which prevent partial file names from matching
	# and trim off some file name text that source-highlight itself
	# produces.
	cd $@ &&                                                    \
          perl -p -i                                                \
            -e '$$frgx ||= join("|", split("\n", `ls -1 *.[ch]`));' \
            -e '$$fre_dot_escape_done ||= ($$frgx =~ s/\./\\./g);'  \
            -e 's/((?<!\w)(?:$$frgx)(?!\.html|\:\d+))'              \
            -e ' /<a href="$$1.html">$$1<\/a>/gx;'                  \
            *.html
	# Add all .pdf files to the mix and link them (for datasheets, etc).
	cd $@ && find .. -maxdepth 2 -name "*.pdf" -exec ln -s \{\} \;
	# This works almost like the perl snippet above in this recipe, see the
	# comments for that.
	cd $@ && \
          perl -p -i \
            -e '$$frgx ||= join("|", split("\n", `ls -1 *.pdf`));'   \
            -e '$$fre_dot_escape_done ||= ($$frgx =~ s/\./\\./g);'   \
            -e 's/($$frgx)(,\s+page\s+(\d+))?'                       \
            -e ' /"<a href=\"$$1".($$2?"":"")."\">$$1<\/a>$$2"/egx;' \
            *.html
	# This version supports page number references, which would be great if
	# the browsers supported page number references sanely, but they don't.
	# Firefox seems to ignore them in URLs, and chrome helpfully takes us
	# to the page *after* the one requested for some datasheets (page one
	# apparently counting as contents or something even though its not, and
	# is labeled page one).  Note that this takes an extra eval pass on
	# the substitution part (e regex modifier).  FIXME: enable this
	# instead of the above if PDFs/browsers ever stop sucking.
	#cd $@ && \
        #  perl -p -i \
        #    -e '$$frgx ||= join("|", split("\n", `ls -1 *.pdf`));'            \
        #    -e '$$fre_dot_escape_done ||= ($$frgx =~ s/\./\\./g);'            \
        #    -e 's/($$frgx)(,\s+page\s+(\d+))?'                                \
        #    -e ' /"<a href=\"$$1".($$2?"#page=$$3":"")."\">$$1$$2<\/a>"/egx;' \
        #    *.html
	rm $@/*.[ch]
	rm $@/tags

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
.PHONY: targzball
targzball: build_all_test_programs clean_all_modules xlinked_source_html
	[ -n "$(VERSION)" ] || (echo VERSION not set 1>&2 && false)
	cd /tmp ; cp -r $(shell pwd) .
	# We determine the Distribution Name (DN) from the source dir
	DN=$(shell basename `pwd`) && \
        cd /tmp/$$DN && \
        rm -rf .git .gitignore && \
        cd .. && \
        mv $$DN $$DN-$(VERSION) && \
        tar czvf $$DN-$(VERSION).tgz $$DN-$(VERSION)

# Upload the targzball and documentation to the web site.  The unstable
# snapshot that we provide is uploaded first, since it should never be
# behind the stable version.
.PHONY: upload
upload: targzball upload_html update_unstable git_push
	# We determine the Distribution Name (DN) from the source dir
	DN=$(shell basename `pwd`) && \
        scp /tmp/$$DN-$(VERSION).tgz $(WEB_SSH):$(WEB_ROOT)/releases/ && \
        ssh $(WEB_SSH) rm -f '$(WEB_ROOT)/releases/LATEST_IS_*' && \
        ssh $(WEB_SSH) ln -s --force $$DN-$(VERSION).tgz \
                       $(WEB_ROOT)/releases/LATEST_IS_$$DN-$(VERSION).tgz

# Update the unstable snapshot by building and uploading an unstable targzball.
.PHONY: update_unstable
update_unstable: build_all_test_programs clean_all_modules
	ssh $(WEB_SSH) mkdir -p $(WEB_ROOT)/unstable/
	# We determine the Distribution Name (DN) from the source dir
	DN=$(shell basename `pwd`) && \
        ssh $(WEB_SSH) rm -f '$(WEB_ROOT)/unstable/$${DN}_unstable*' && \
        cd /tmp ; cp -r $(shell pwd) . && \
          UNSTABLE_NAME=$${DN}_unstable_`date +%Y-%m-%d-%H-%M-%S` && \
          mv $$DN $$UNSTABLE_NAME && \
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
# big to upload (WHY? cpp constructors are always in scope or just too
# much static data or what? Is there a way to strip more junk out?) See
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
