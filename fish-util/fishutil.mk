# Caller has to set fishutil_dir.

fishutil_inc	:= -I$(fishutil_dir)
fishutil_obj 	:= $(fishutil_dir)/fish-util.o
fishutil_src_dep 	:= $(shell cat $(fishutil_dir)/.obj/fish-util.o/src_dep)
fishutil_ld 	:= $(shell cat $(fishutil_dir)/.obj/fish-util.o/ld)
fishutil_all 	:= $(fishutil_inc) $(fishutil_obj) $(fishutil_ld)

