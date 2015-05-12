fish_util_dir = ../fish-util

install_inc_dir = /usr/local/include
install_lib_dir = /usr/local/lib

cc = gcc -std=c99 -fPIC
shared = -shared

lib = -lm -lpthread -lpcre
inc = -I$(fish_util_dir)

src = fish-utils/vec.c fish-utils/regex.c fish-utils/main.c fish-utils.h
objs = fish-utils/vec.o fish-utils/regex.o fish-utils/main.o

all: $(objs) libfish-utils.so fish-utils.o

libfish-utils.so: fish-utils.o
	$(cc) $(shared) $(objs) \
	    -o libfish-utils.so

$(objs): %.o: %.c
	@ # < = first prereq
	$(cc) $(inc) -c $< -o $@

fish-utils.o: $(src) $(objs)
	@ # combine mulitiple .o into a total .o file.
	ld -r $(objs) -o fish-utils.o

test: 
	# xxx
	
install: 
	mkdir -p $(install_inc_dir)/fish-utils/fish-utils
	cp fish-utils.h $(install_inc_dir)/fish-utils
	cp -r fish-utils/*.h $(install_inc_dir)/fish-utils/fish-utils
	cp libfish-utils.so $(install_lib_dir)
	$(ldconfig)

clean: 
	rm -rf tmp
	rm -f *.o
	rm -f fish-utils/*.o
	rm -f *.so

mrproper: clean
	rm -rf .obj

.PHONY: all install clean mrproper
