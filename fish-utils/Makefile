include ../Makefile.inc

# fish-util objects not necessary -- to make fish-utils.o we don't need them
# and to make libfish-utils.so we use libfish-util.so.
fish_util_dir = ../fish-util

FLAGS_C = -std=c99 -lm -lpthread -lpcre -fPIC -I$(inc_dir) -I$(fish_util_dir)
FLAGS_SHARED = -shared -L$(lib_dir) -lfish-util

main = fish-utils.c
deps = $(main) fish-utils/vec.o fish-utils/regex.o

all: init libfish-utils.so fish-utils.o
	
init: 
	mkdir -p .deps/fish-utils

libfish-utils.so: $(deps)
	gcc $(FLAGS_SHARED) $(FLAGS_C) $(deps) \
	    -o libfish-utils.so

fish-utils.o: $(main)
	gcc $(FLAGS_C) $(main) -c -o fish-utils.o
	touch .deps/fish-utils.o

fish-utils/vec.o: fish-utils/vec.c
	gcc $(FLAGS_C) $(fish_util) -c fish-utils/vec.c -o fish-utils/vec.o
	touch .deps/fish-utils/vec.o

fish-utils/regex.o: fish-utils/regex.c
	gcc $(FLAGS_C) $(fish_util) -c fish-utils/regex.c -o fish-utils/regex.o
	touch .deps/fish-utils/regex.o

install: 
	mkdir -p $(inc_dir)/fish-utils/fish-utils
	cp fish-utils.h $(inc_dir)/fish-utils
	cp -r fish-utils/*.h $(inc_dir)/fish-utils/fish-utils
	cp libfish-utils.so $(lib_dir)
	$(ldconfig)

clean: 
	rm -rf .deps
	rm -f *.o
	rm -f fish-utils/*.o
	rm -f *.so
