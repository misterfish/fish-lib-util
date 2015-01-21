include ../Makefile.inc

FLAGS_C = -std=c99 -fPIC -lm -lpthread 
#FLAGS_SHARED = -fPIC -shared
FLAGS_SHARED = -shared

all: libfish-util.so fish-util.o

test: all install test.c
	gcc $(FLAGS_C) -lfish-util -I$(inc_dir) -L$(lib_dir) test.c -o test
	./test

fish-util.o: fish-util.c fish-util.h
	gcc $(FLAGS_C) fish-util.c -c -o fish-util.o
	mkdir -p .deps
	touch .deps/fish-util.o

libfish-util.so: fish-util.c fish-util.h
	gcc $(FLAGS_SHARED) $(FLAGS_C) fish-util.c -o libfish-util.so

install: 
	cp fish-util.h $(inc_dir)
	chmod a+r $(inc_dir)/fish-util.h
	cp libfish-util.so $(lib_dir)
	chmod a+r $(lib_dir)/libfish-util.so
	$(ldconfig)

clean: 
	rm -rf .deps
	rm -f *.o
	rm -f *.so
