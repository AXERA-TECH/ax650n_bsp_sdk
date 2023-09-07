include ../Makefile.inc
HOME_PATH               := ../../../../../..
INSTALL_BIN_PATH ?= $(HOME_PATH)/rootfs/rootfs/opt/data/dsp
BINDIR  = $(ROOTDIR)/out/dsp_algo
all : $(BINDIR)/dsp

$(BINDIR)/dsp : dsp_algo.c sample_add.c sample_tile_manager.c ax_dsp_trace.c sample_idma.c
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -Iinclude -DIDMA_APP_USE_XTOS -DIDMA_USE_XTOS $(LDFLAGS) -mlsp=$(LSP) $^ -o $@ -L$(LIBDIR)  -lax_tile_manager -lax650x -lax_cvlib -lxi_ref -lxi -lidma-xtos -lidma -lm
	$(DUMPELF) --base=0x20000000 --width=32 --little-endian --default=0x00000000 --full $(BINDIR)/dsp --size=0xF00000 >  $(BINDIR)/sram.txt
	$(DUMPELF) --base=0x00000000 --width=32 --little-endian --default=0x00000000 --full $(BINDIR)/dsp --size=0x8000 >  $(BINDIR)/itcm.txt
	python2 ../hex2bin.py $(BINDIR)/itcm.txt $(BINDIR)/itcm.bin
	python2 ../hex2bin.py $(BINDIR)/sram.txt $(BINDIR)/sram.bin

$(BINDIR)/%.o: %.S
	$(CC) $(ASFLAGS) $(FLAGS_$*) -o $@ -c $<

$(BINDIR)/%.o: %.c
	$(CC) $(CFLAGS)  $(FLAGS_$*) -o $@ -c $<
install: all
	@mkdir -p $(INSTALL_BIN_PATH)
	@cp -f $(BINDIR)/itcm.bin  $(INSTALL_BIN_PATH)/
	@cp -f $(BINDIR)/sram.bin  $(INSTALL_BIN_PATH)/

INSTALL_DATA            := $(BINDIR)

clean:
	-$(RM) -rf $(BINDIR)/*

.PHONY : all install clean

