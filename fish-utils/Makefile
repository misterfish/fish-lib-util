# Not necessary to make fish-util objects available to the outside -- to
# make fish-utils.o we don't need them and to make libfish-utils.so we use

fish_util_dir = ../fish-util

# necessary on pi? XX
inc_dirs = /usr/local/include $(fish_util_dir)
lib_dirs = /usr/local/lib ../fish-util
inc_dir_install = /usr/local/include
lib_dir_install = /usr/local/lib

# Add -I / -L
inc_flags = $(shell echo "$(inc_dirs)" | perl -ne 'print join " ", map { "-I$$_" } split m, \s+ ,x')
lib_flags = $(shell echo "$(lib_dirs)" | perl -ne 'print join " ", map { "-L$$_" } split m, \s+ ,x')

flags_c = -std=c99 -lm -lpthread -lpcre -fPIC $(inc_flags)
flags_shared = -shared $(lib_flags) -lfish-util

main = fish-utils.c
deps = $(main) fish-utils/vec.o fish-utils/regex.o

all: init libfish-utils.so fish-utils.o
	
init: 
	mkdir -p .deps/fish-utils

libfish-utils.so: $(deps)
	gcc $(flags_shared) $(flags_c) $(deps) \
	    -o libfish-utils.so

fish-utils.o: $(main)
	gcc $(flags_c) $(main) -c -o fish-utils.o
	touch .deps/fish-utils.o

fish-utils/vec.o: fish-utils/vec.c
	gcc $(flags_c) $(fish_util) -c fish-utils/vec.c -o fish-utils/vec.o
	touch .deps/fish-utils/vec.o

fish-utils/regex.o: fish-utils/regex.c
	gcc $(flags_c) $(fish_util) -c fish-utils/regex.c -o fish-utils/regex.o
	touch .deps/fish-utils/regex.o

install: 
	mkdir -p $(inc_dir_install)/fish-utils/fish-utils
	cp fish-utils.h $(inc_dir_install)/fish-utils
	cp -r fish-utils/*.h $(inc_dir_install)/fish-utils/fish-utils
	cp libfish-utils.so $(lib_dir_install)
	$(ldconfig)

clean: 
	rm -rf .deps
	rm -f *.o
	rm -f fish-utils/*.o
	rm -f *.so
