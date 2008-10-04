SUBDIRS := include test

include makefiles/Makefile.common

test:
	$(MAKE) -C test $@

.PHONY: test

