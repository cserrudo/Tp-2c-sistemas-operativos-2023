make all:
	make -C Memoria
	make -C Filesystem
	make -C Cpu
	make -C Kernel
	make -C Utils
make clean:
	make clean -C Memoria
	make clean -C Filesystem
	make clean -C Cpu
	make clean -C Kernel
	make clean -C Utils
