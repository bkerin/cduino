# This Makefile handles jobs related to maintenance of the package itself, the
# useful content is in generic.mk (see manual.html for details).

view_web_page:
	~/opt/firefox/firefox home_page.html

WEB_SSH := brittonk@box201.bluehost.com
WEB_ROOT := public_html/cduino

# Note this doesn't nuke old pages or contents
deploy_web_page: targzball
        scp *.html $(WEB_SSH):$(WEB_ROOT)

# WORK POINT: this freaking creator still doesn't work
targzball:
	[ -n "$(VERSION)" ] || (echo VERSION not set 1>&2 && false)
	cd /tmp ; cp -r $(shell pwd) .
	cd /tmp/cduino && rm -rf .git .gitignore ; cd .. ; \
          mv cduino cduino-$(VERSION) ; \
          tar czvf cduino-$(VERSION).tgz cduino-$(VERSION)
