objects = sample_pcie_dma_host.o ax_pcie_dma.o mm_buffer.o ../msg/ax_pcie_msg.o

CFLAGS = -g -Wall -O0 -I../include

sample_pcie_msg_host : $(objects)
	gcc $(CFLAGS) -o sample_pcie_dma_host $(objects) -lpthread

.PHONY : clean
clean :
	rm -f sample_pcie_dma_host *.o