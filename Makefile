fish_util_dir 	:= fish-util
fish_utils_dir 	:= fish-utils

# - - -

cc = gcc -std=c99 -fPIC

# sets <module>_obj, <module>_src_dep, <module>_ld, and <module>_all.
include $(fish_util_dir)/fish-util.mk
include $(fish_utils_dir)/fish-utils.mk

VPATH=$(fish_util_dir):$(fish_utils_dir)

all: check-init $(fishutil_obj) $(fishutils_obj)

check-init: 
	@ if [ ! -e .init ]; then echo "\nNeed to run './configure' first.\n"; exit 1; fi

test: all test.c
	$(cc) test.c $(fishutil_all) $(fishutils_all) -o test
	./test

$(fishutil_obj): $(fishutil_src_dep)
	echo XAXAX
	echo $(fishutil_src_dep)
	make -C $(fish_util_dir)

$(fishutils_obj): $(fishutils_src_dep)
	echo YAYAY
	echo $(fishutils_src_dep)
	make -C $(fish_utils_dir)

install: 
	sh -c 'cd fish-util; make install'
	sh -c 'cd fish-utils; make install'

clean:
	cd $(fish_util_dir) && make clean
	cd $(fish_utils_dir) && make clean
	rm -f .init

.PHONY: all install clean test check-init
