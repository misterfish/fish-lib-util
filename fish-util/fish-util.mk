# Caller has to set fish_util_dir.

fishutil_obj 	:= $(fish_util_dir)/fish-util.o
fishutil_src_dep 	:= $(shell cat $(fish_util_dir)/.obj/fish-util.o/src_dep)
fishutil_ld 	:= $(shell cat $(fish_util_dir)/.obj/fish-util.o/ld)
fishutil_all 	:= -I$(fish_util_dir) $(fishutil_obj) $(fishutil_ld)

