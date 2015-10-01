
GADEPS		?= $(HOME)/myprog/gaminganywhere/deps.posix
MACHINE		?= $(shell uname -m)
SYSTEM		= $(shell uname -s)
PACKAGES	= sdl2 sdl2_ttf yasm lame opus ogg vorbis theora vpx \
		  x264 x265 xvid ffmpeg live555
OSX_CLANG_PATCH	= echo '*** Not on OS X: not applied ***'

ifeq ($(SYSTEM),Darwin)
VPX_EXTRA_CONFIG =
HAS_BREW	?= $(shell if which -s brew; then echo 1; else echo 0; fi)
MACHINE		= $(shell uname -m)-apple-darwin
OSX_CLANG_PATCH = perl -p -i -e "s/-falign-loops=16//g" configure; \
		  perl -p -i -e "s/-fforce-addr//g"     configure
ifeq ($(HAS_BREW),1)
PACKAGES	= osx_brew live555
endif
else
VPX_EXTRA_CONFIG = --enable-shared
endif

TBZUNPACK	= tar xjf
TGZUNPACK	= tar xzf

include Makefile.packages

all: $(PACKAGES)

osx_brew:
	brew install sdl2 sdl2_ttf yasm lame opus libogg libvorbis theora \
		libvpx x264 x265 xvid
	brew install --with-libvorbis --with-libvpx --with-opus \
		--with-theora --with-x265 --with-tools ffmpeg

sdl2:
	$(TGZUNPACK) $(SDL2)
	mkdir -p $(SDL2DIR)/mybuild
	cd $(SDL2DIR)/mybuild; ../configure --prefix=$(GADEPS) --with-pic
	$(MAKE) -C $(SDL2DIR)/mybuild
	$(MAKE) -C $(SDL2DIR)/mybuild install
	rm -rf $(SDL2DIR)

sdl2_ttf: sdl2
	$(TGZUNPACK) $(SDL2_TTF)
	mkdir -p $(SDL2_TTFDIR)/mybuild
	cd $(SDL2_TTFDIR)/mybuild; ../configure --prefix=$(GADEPS) --with-pic
	$(MAKE) -C $(SDL2_TTFDIR)/mybuild
	$(MAKE) -C $(SDL2_TTFDIR)/mybuild install
	rm -rf $(SDL2_TTFDIR)

yasm:
	$(TGZUNPACK) $(YASM)
	cd $(YASMDIR); ./configure --prefix=$(GADEPS)
	$(MAKE) -C $(YASMDIR) $(PARALLEL_MAKE)
	$(MAKE) -C $(YASMDIR) install
	rm -rf $(YASMDIR)

lame:
	$(TGZUNPACK) $(LAME)
	cd $(LAMEDIR); ./configure --prefix=$(GADEPS) --with-pic
	$(MAKE) -C $(LAMEDIR)
	$(MAKE) -C $(LAMEDIR) install
	rm -rf $(LAMEDIR)

opus:
	$(TGZUNPACK) $(OPUS)
	#
	cd $(OPUSDIR); ./configure --prefix=$(GADEPS) --with-pic
	$(MAKE) -C $(OPUSDIR)
	$(MAKE) -C $(OPUSDIR) install
	rm -rf $(OPUSDIR)

ogg:
	$(TGZUNPACK) $(OGG)
	cd $(OGGDIR); ./configure --prefix=$(GADEPS) --with-pic
	$(MAKE) -C $(OGGDIR)
	$(MAKE) -C $(OGGDIR) install
	rm -rf $(OGGDIR)

vorbis: ogg
	$(TGZUNPACK) $(VORBIS)
	# Fix missing reference to ARM6 for Raspberry Pi compilation
	sed -i.bak 's/armv\[345\]/armv\[3456\]/g' $(VORBISDIR)/config.sub
	cd $(VORBISDIR); ./configure --prefix=$(GADEPS) --build=$(MACHINE) --with-pic
	$(MAKE) -C $(VORBISDIR)
	$(MAKE) -C $(VORBISDIR) install
	rm -rf $(VORBISDIR)

theora: ogg vorbis sdl2
	$(TBZUNPACK) $(THEORA)
	cd $(THEORADIR); $(OSX_CLANG_PATCH)
	cd $(THEORADIR); ./configure --prefix=$(GADEPS) --with-pic --disable-examples
	$(MAKE) -C $(THEORADIR)
	$(MAKE) -C $(THEORADIR) install
	rm -rf $(THEORADIR)

vpx: yasm
	$(TBZUNPACK) $(VPX)
	cd $(VPXDIR); ./configure --prefix=$(GADEPS) $(VPX_EXTRA_CONFIG) --enable-pic
	$(MAKE) -C $(VPXDIR)
	$(MAKE) -C $(VPXDIR) install
	rm -rf $(VPXDIR)

x264: yasm
	$(TBZUNPACK) $(X264)
	cd $(X264DIR); $(OSX_CLANG_PATCH)
	cd $(X264DIR); ./configure --prefix=$(GADEPS) --enable-shared --enable-pic
	$(MAKE) -C $(X264DIR)
	$(MAKE) -C $(X264DIR) install
	rm -rf $(X264DIR)

x265: yasm
	$(TBZUNPACK) $(X265)
	cd $(X265DIR)/build/linux; cmake \
		-D CMAKE_INSTALL_PREFIX:PATH=$(GADEPS) \
		-D CMAKE_CXX_FLAGS="-fPIC" -D CMAKE_C_FLAGS="-fPIC" \
		../../source/
	make -C $(X265DIR)/build/linux $(PARALLEL_MAKE)
	make -C $(X265DIR)/build/linux install
	rm -rf $(X265DIR)

xvid:
	$(TBZUNPACK) $(XVID)
	cd $(XVIDDIR)/build/generic; ./configure --prefix=$(GADEPS)
	$(MAKE) -C $(XVIDDIR)/build/generic
	$(MAKE) -C $(XVIDDIR)/build/generic install
	rm -rf $(XVIDDIR)

ffmpeg: lame vorbis theora vpx x264 x265 opus xvid
	$(TBZUNPACK) $(FFMPEG)
	cd $(FFMPEGDIR); ./configure --prefix=$(GADEPS) \
		--enable-shared --enable-pic \
		--enable-libmp3lame \
		--enable-libtheora --enable-libvorbis \
		--enable-libx264 --enable-libvpx \
		--enable-libx265 \
		--enable-libopus \
		--enable-libxvid \
		--enable-gpl --enable-nonfree --disable-outdev=sdl \
		--extra-cflags=-I$(GADEPS)/include \
		--extra-ldflags=-L$(GADEPS)/lib
	$(MAKE) -C $(FFMPEGDIR)
	$(MAKE) -C $(FFMPEGDIR) install
	rm -rf $(FFMPEGDIR)

live555:
	$(TGZUNPACK) $(LIVE555)
	# enable -fPIC for macosx and linux
	mv -f $(LIVE555DIR)/config.linux $(LIVE555DIR)/config.linux.old
	mv -f $(LIVE555DIR)/config.macosx $(LIVE555DIR)/config.macosx.old
	cat $(LIVE555DIR)/config.linux.old | sed -e 's,^COMPILE_OPTS.*$$,& -fPIC,' > $(LIVE555DIR)/config.linux
	cat $(LIVE555DIR)/config.macosx.old | sed -e 's,^COMPILE_OPTS.*$$,& -fPIC,' > $(LIVE555DIR)/config.macosx
	# need to apply the 4MB buffer patch?
	cd $(LIVE555DIR); cat ../patches/live555-* | patch -p1 
	#
	cd $(LIVE555DIR); ./genMakefiles $(shell uname -s | tr A-Z a-z | sed -e 's,darwin,macosx,' -e 's,cygwin.*,cygwin,')
	$(MAKE) -C $(LIVE555DIR)
	-mkdir -p $(GADEPS)/lib
	find $(LIVE555DIR) -name '*.a' -exec cp -f {} $(GADEPS)/lib \;
	-mkdir -p $(GADEPS)/include/live555
	find $(LIVE555DIR) -name '*.hh' -exec cp -f {} $(GADEPS)/include/live555 \;
	cp -f $(LIVE555DIR)/groupsock/include/NetCommon.h $(GADEPS)/include/live555
	rm -rf $(LIVE555DIR)

.PHONY: all $(PACKAGES)
