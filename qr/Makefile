# Top-level Makefile for QR Tape

.PHONY: all clean test

all:
	$(MAKE) -C librs
	$(MAKE) -C src/qrtape

clean:
	$(MAKE) -C librs clean
	$(MAKE) -C src/qrtape clean

test: all
	cd src/qrtape && ./test_fec.sh
