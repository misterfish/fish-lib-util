#.init: $(src) 
#	@ # src_dep (.c and .h files) so people including us know when to remake us.
#	@ # ld so people including us link what we need (e.g. -lm)
#	mkdir -p .obj/fish-utils.o 
#	echo > .obj/fish-utils.o/src_dep $(src)
#	echo > .obj/fish-utils.o/ld $(lib)


#check-init: 
#	@ if [ ! -e .init ]; then echo "\nfish-util: Need to run './configure' first (pwd=$(shell pwd)).\n"; exit 1; fi

# sets <module>_inc, <module>_obj, <module>_src_dep, <module>_ld, and <module>_all.
include $(fishutil_dir)/fishutil.mk
include $(fishutils_dir)/fishutils.mk
VPATH=$(fishutil_dir):$(fishutils_dir)

