all:

%:
	$(MAKE) -C controller -f Makefile $*
	$(MAKE) -C hub -f Makefile $*

