fishutil_dir 	:= fish-util
fishutils_dir 	:= fish-utils

pkgconfig	= pkg-config/static/fish-util.pc \
                  pkg-config/static/fish-utils.pc \
                  pkg-config/dynamic/fish-util.pc \
                  pkg-config/dynamic/fish-utils.pc 

pkgconfig_ins	= pkg-config/static/fish-util.pc.in \
                  pkg-config/static/fish-utils.pc.in \
                  pkg-config/dynamic/fish-util.pc.in \
                  pkg-config/dynamic/fish-utils.pc.in 

# - - -

cc = gcc -std=c99 -fPIC

all: init fishutil_obj fishutils_obj

$(pkgconfig): $(pkgconfig_ins)
	scripts/gen-pkg-config

init: $(pkgconfig)

test: 
	make -C $(fishutil_dir) test
	make -C $(fishutils_dir) test

fishutil_obj: 
	make -C $(fishutil_dir)

fishutils_obj: 
	make -C $(fishutils_dir)

install: 
	sh -c 'cd $(fishutil_dir); make install'
	sh -c 'cd $(fishutils_dir); make install'

clean:
	cd $(fishutil_dir) && make clean
	cd $(fishutils_dir) && make clean
	cd pkg-config/static && rm -f *.pc
	cd pkg-config/dynamic && rm -f *.pc

mrproper: clean
	cd $(fishutil_dir) && make mrproper
	cd $(fishutils_dir) && make mrproper

.PHONY: all install clean test mrproper
