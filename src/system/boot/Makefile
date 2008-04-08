all:
ifdef MIPS_COMMON
	cd mips/; perl mkheader.pl; $(MAKE) $(SILENTFLAG)
endif
ifdef ARM_COMMON
	cd arm/; perl mkheader.pl; $(MAKE) $(SILENTFLAG)
endif
