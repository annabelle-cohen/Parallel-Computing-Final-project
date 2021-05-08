build:
	mpicxx -c ParallelProject.c -fopenmp -lm
	mpicxx -c myFuncOmp.c -fopenmp -lm
	nvcc -c myFuncGpu.cu -o myFuncGpu.o
	mpicxx -o exec ParallelProject.o myFuncOmp.o myFuncGpu.o /usr/local/cuda-9.1/lib64/libcudart_static.a  -fopenmp -lm  -ldl -lrt

clean:
	rm *.o exec

run2:
	mpiexec -np 2 ./exec 8 input.txt 
