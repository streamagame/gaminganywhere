
# Makefile for POSIX-based OSes

TARGET	= core module server client

.PHONY: all $(TARGET)

all: $(TARGET)

core:
	$(MAKE) -C core

module: core
	$(MAKE) -C module

server: core
	$(MAKE) -C server

client: core
	$(MAKE) -C client

install:
	mkdir -p ../bin
	$(MAKE) -C core install
	$(MAKE) -C module install
	$(MAKE) -C server install
	$(MAKE) -C client install
	cp -fr config ../bin/

clean:
	$(MAKE) -C core clean
	$(MAKE) -C module clean
	$(MAKE) -C server clean
	$(MAKE) -C client clean

