default:
	./configure && $(MAKE) dep && $(MAKE)


#########################################################
# just for me, some maintaining jobs.  Don't use them

checkit diff release port:
	./configure && $(MAKE) $@
