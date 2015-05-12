fishutil_dir 	:= fish-util
fishutils_dir 	:= fish-utils

# - - -

cc = gcc -std=c99 -fPIC

# sets <module>_inc, <module>_obj, <module>_src_dep, <module>_ld, and <module>_all.
include $(fishutil_dir)/fishutil.mk
include $(fishutils_dir)/fishutils.mk
VPATH=$(fishutil_dir):$(fishutils_dir)

all: check-init $(fishutil_obj) $(fishutils_obj)

check-init: 
	@ if [ ! -e .init ]; then echo "\nfish-util: Need to run './configure' first (pwd=$(shell pwd)).\n"; exit 1; fi

test: test.c
	$(cc) test.c $(fishutil_all) $(fishutils_all) -o test
	./test

$(fishutil_obj): $(fishutil_src_dep)
	make -C $(fishutil_dir)

$(fishutils_obj): $(fishutils_src_dep)
	make -C $(fishutils_dir)

install: 
	sh -c 'cd fish-util; make install'
	sh -c 'cd fish-utils; make install'

clean:
	cd $(fishutil_dir) && make clean
	cd $(fishutils_dir) && make clean

mrproper: clean
	cd $(fishutil_dir) && make mrproper
	cd $(fishutils_dir) && make mrproper
	rm -f .init

.PHONY: all install clean test check-init mrproper
