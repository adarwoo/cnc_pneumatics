all:

%:
	$(MAKE) -C relay -f Makefile $*


# $(MAKE) -C controller -f Makefile $*
# $(MAKE) -C hub -f Makefile $*
# $(MAKE) -C console -f Makefile $*

controller:
	$(MAKE) -C controller -f Makefile $*

hub:
	$(MAKE) -C hub -f Makefile $*

relay:
	$(MAKE) -C relay -f Makefile $*

console:
	$(MAKE) -C console -f Makefile $*
