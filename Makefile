all:
	@echo "make directhw	-	builds DirectHW dmg."
	@echo "make pciutils	-	builds pciutils dmg."
	#@echo "make tools	-	builds coreboot utilities dmg."
	@echo "make clean	-	clean up."

PCIUTILS=pciutils-3.1.7

directhw:
	$(MAKE) -C DirectHW

pciutils:
	rm -rf $(PCIUTILS)
	wget ftp://ftp.kernel.org/pub/software/utils/pciutils/$(PCIUTILS).tar.bz2
	tar xvjf $(PCIUTILS).tar.bz2
	patch -p0 < patches/$(PCIUTILS).diff
	cd $(PCIUTILS); \
	make OPT="-O2 -arch i386 -arch x86_64" LDFLAGS="-arch i386 -arch x86_64" IDSDIR=/usr/share; \
	make install DESTDIR=$(shell pwd)/$(PCIUTILS)/root PREFIX=/usr ; \
	make install-lib DESTDIR=$(shell pwd)/$(PCIUTILS)/root PREFIX=/usr ; \
	/Developer/usr/bin/packagemaker -v --doc $(shell pwd)/$(PCIUTILS)/osx/pciutils.pmdoc --id com.pciutils --out pciutils.pkg
	rm -rf $(PCIUTILS).dmg
	rm -rf out
	mkdir out
	cp -r $(PCIUTILS)/pciutils.pkg out/Install\ PCI\ Utilities.pkg
	cp -r $(PCIUTILS)/osx/Welcome.rtf out/Read\ Me.rtf
	/Developer/Tools/SetFile -a E out/Install\ PCI\ Utilities.pkg
	/Developer/Tools/SetFile -a E out/Read\ Me.rtf
	./create-dmg/create-dmg --window-size 447 337 --background \
	DirectHW/background.png --icon-size 80 \
	--volname "Install PCI Utilities" \
	--icon "Install PCI Utilities.pkg" 142 64 \
	--icon "Read Me.rtf" 310 64 $(PCIUTILS).dmg out

#tools: 
#	$(MAKE) -C tools

#patches:
#	$(MAKE) -C upstream

clean:
	#cd $(PCIUTILS); make clean; rm -rf pciutils.pkg; rm -rf root
	#cd tools; make clean
	cd DirectHW; make clean

.PHONY: all directhw pciutils tools patches clean

