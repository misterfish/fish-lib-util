all: util utils

util:
	sh -c 'cd fish-util; make'

utils:
	sh -c 'cd fish-utils; make'

install: 
	sh -c 'cd fish-util; make install'
	sh -c 'cd fish-utils; make install'
