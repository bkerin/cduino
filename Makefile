# This Makefile handles jobs related to maintenance of the package itself, the
# useful content is in generic.mk (see manual.html for details).

# Delete files produced by rules the commands of which return non-zero.
.DELETE_ON_ERROR:
# Disable all suffix rules.
.SUFFIXES:

view_web_page:
	~/opt/firefox/firefox home_page.html

WEB_SSH := brittonk@box201.bluehost.com
WEB_ROOT := public_html/cduino

# Note this doesn't nuke old pages with different names
.PHONY: upload_html
upload_html:
	scp *.html $(WEB_SSH):$(WEB_ROOT)
	ssh $(WEB_SSH) ln -s --force home_page.html $(WEB_ROOT)/index.html

.PHONY: targzball
targzball:
	[ -n "$(VERSION)" ] || (echo VERSION not set 1>&2 && false)
	cd /tmp ; cp -r $(shell pwd) .
	cd /tmp/cduino && rm -rf .git .gitignore ; cd .. ; \
          mv cduino cduino-$(VERSION) ; \
          tar czvf cduino-$(VERSION).tgz cduino-$(VERSION)

.PHONY: upload
upload: targzball upload_html
	scp /tmp/cduino-$(VERSION).tgz $(WEB_SSH):$(WEB_ROOT)/releases
	ssh $(WEB_SSH) ln -s --force cduino-$(VERSION).tgz \
            $(WEB_ROOT)/releases/LATEST_IS_cduino-$(VERSION).tgz
