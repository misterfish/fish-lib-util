# Caller has to set fish_utils_dir.

fishutils_obj 	:= $(fish_utils_dir)/fish-utils.o
fishutils_src_dep 	:= $(shell cat $(fish_utils_dir)/.obj/fish-utils.o/src_dep)
fishutils_ld 	:= $(shell cat $(fish_utils_dir)/.obj/fish-utils.o/ld)
fishutils_all 	:= -I$(fish_utils_dir) $(fishutils_obj) $(fishutils_ld)


