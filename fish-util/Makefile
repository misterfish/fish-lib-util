install_inc_dir = /usr/local/include
install_lib_dir = /usr/local/lib

cc = gcc -std=c99 -fPIC
shared = -shared

math_lib = -lm

lib = $(math_lib)
inc =

all = $(lib) $(inc)

src = fish-util.c fish-util.h

all: libfish-util.so fish-util.o

test:
	# xxx

fish-util.o: $(src) 
	$(cc) fish-util.c -c -o fish-util.o

libfish-util.so: fish-util.o
	$(cc) $(shared) $(all) fish-util.c -o libfish-util.so

install:
	cp fish-util.h $(install_inc_dir)
	chmod a+r $(install_inc_dir)/fish-util.h
	cp libfish-util.so $(install_lib_dir)
	chmod a+r $(install_lib_dir)/libfish-util.so
	$(ldconfig)

clean:
	rm -f *.o
	rm -f *.so

mrproper: clean
	rm -rf .obj

.PHONY: all install clean mrproper
