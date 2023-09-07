objects = sample_pcie_boot.o

CFLAGS = -g -Wall -O0 -I../include -I ./zlib/ -L ./zlib/lib_x86/

sample_pcie_boot : $(objects)
	gcc $(CFLAGS) -o sample_pcie_boot $(objects)  zlib/*.c -lpthread -lz

.PHONY : clean
clean :
	rm -f sample_pcie_boot *.o
	rm -f zlib/*.o
