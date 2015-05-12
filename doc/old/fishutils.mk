# Caller has to set fishutils_dir.

fishutils_inc	:= -I$(fishutils_dir)
fishutils_obj 	:= $(fishutils_dir)/fish-utils.o
fishutils_src_dep 	:= $(shell cat $(fishutils_dir)/.obj/fish-utils.o/src_dep)
fishutils_ld 	:= $(shell cat $(fishutils_dir)/.obj/fish-utils.o/ld)
fishutils_all 	:= $(fishutils_inc) $(fishutils_obj) $(fishutils_ld)


