#include "myFuncOmp.h"
#include <cuda_runtime.h>



//allocate memory for string
int cudaAllocateString(char **str,int str_len){
    cudaError_t cudaStatus;
    *str = 0;
    cudaStatus = cudaMalloc((void**)str,str_len*sizeof(char));
    //if there is an error then we will get into the if statement
    if(cudaStatus != cudaSuccess){
        printf("Failed allocating cuda\n");
        goto error;
    }
    return 1;
error:
    cudaFree(*str);
    return 0;

}
//allocate memory for int
int cudaAllocateIntArray(int ** ptr,int num_elemnts)
{
    cudaError_t cudaStatus;
    *ptr = 0;
    cudaStatus = cudaMalloc((void**)ptr,num_elemnts*sizeof(int));
    //if there is an error then we will get into the if statement
    if(cudaStatus != cudaSuccess){
        printf("Failed allocating cuda\n");
        goto error;
    }
    return 1;
error:
    cudaFree(*ptr);
    return 0;

}

//copy to the gpu
int cudaCopyToDevice(char* cpu_mem,char* gpu_mem,int length){
    cudaError_t cudaStatus;
    cudaStatus = cudaMemcpy(gpu_mem,cpu_mem,length*sizeof(char),cudaMemcpyHostToDevice); 
    //if there is an error then we will get into the if statement 
    if(cudaStatus != cudaSuccess){
        printf("Failed copy to device cuda\n");
        return 0;
    }
    return 1;

}

//copy to the cpu
int cudaCopyToHost(char* cpu_mem,char* gpu_mem,int length){
    cudaError_t cudaStatus;
    cudaStatus = cudaMemcpy(cpu_mem,gpu_mem,length*sizeof(char),cudaMemcpyDeviceToHost); 
    //if there is an error then we will get into the if statement   
    if(cudaStatus != cudaSuccess){
        printf("Failed copy to host cuda\n");
        return 0;
    }
    return 1;
}

int cudaCopyIntToHost(int* cpu_mem,int* gpu_mem,int length){
    cudaError_t cudaStatus;
    cudaStatus = cudaMemcpy(cpu_mem,gpu_mem,length*sizeof(int),cudaMemcpyDeviceToHost); 
    //if there is an error then we will get into the if statement    
    if(cudaStatus != cudaSuccess){
        printf("Failed copy to host cuda\n");
        return 0;
    }
    return 1;
}
//cuda free allocation
void cudaFreeFromHost(void* ptr)
{
    cudaFree(ptr);
}

__device__ int strLenCuda(char* str){
    int length = 0;
    while(*str != 0){
        length++;
        str++;
    }
    return length;
}

__device__ bool checkIfMatchCuda(unsigned char ch1,unsigned char ch2){
    return ch1 == ch2? true : false;
}

__device__ bool checkWordAndCipherCuda(int input_length, unsigned char *possible_plaintext_str, char *word)
{
    //initalization
    int i = 0;
    int k = 0;
    int numOfSuccesses = 0;
    int wordLength = strLenCuda(word);
    int in_word = 0;
    //same algorithm as myFuncOmp but here we calculate in the gpu.
    for (i=0; i < input_length; i++)
    {
        if (k < wordLength)
        {
            if (possible_plaintext_str[i] == ' ')
            {
                in_word = 0;
                continue;
            }
            if (in_word == 0 || k > 0)
            {
                if (checkIfMatchCuda((unsigned char)word[k], possible_plaintext_str[i]))
                {
                    numOfSuccesses += 1;
                    k++;
                }
                else
                {
                    k = 0;
                    numOfSuccesses = 0;
                }
            }
            in_word = 1;
        }
        else
        {
            if (possible_plaintext_str[i] != ' ')
            {
                k = 0;
                numOfSuccesses = 0;
            }
            else
            {
                break;
            }
        }
    }

    return numOfSuccesses == wordLength ? true : false;
}


__global__ void kernelXor(unsigned int key,char* input_str_cuda,unsigned char* possible_plaintext_str_cuda,int input_length){
    //initalization
    int id = threadIdx.x + blockDim.x*blockIdx.x;
    //if the id is the same ar the input length then we return.
    if(id >= input_length)
        return;
    int keyIndex = id%4;
    char* keyCharPtr = ((char*)&key);
    char keyChar = keyCharPtr[keyIndex];
    //calculate the xor and save the result at the possible plaintext array of cuda.
    possible_plaintext_str_cuda[id] = keyChar ^ input_str_cuda[id];
}

__global__ void kernelFindWords(unsigned char* possible_plaintext_str_cuda,int input_length,char* known_words_array_cuda,int known_words_length,int* cuda_results,int resIndex,int pow_of_2){
    //initalization
    int id = threadIdx.x + blockDim.x*blockIdx.x;
    unsigned int s;
    int sum;
    //we need the shared memory for gathering all the results from each block.
    __shared__ int wordsFound[1024];
    if(id >= known_words_length)
        return;
    wordsFound[threadIdx.x]=0;
    char* myWord = known_words_array_cuda+id*MAX_STRING_LENGTH;
    //if we find somthing we want to increase with 1 at the index of the threadId
    if(checkWordAndCipherCuda(input_length,possible_plaintext_str_cuda,myWord)){
        wordsFound[threadIdx.x]=1;
    }
    __syncthreads();
	//here i take care of extreme case - if the number of block isn't pow of 2(if i dont take care of this case then it will not work at all).
    if(pow_of_2/2 != blockDim.x)
    {
        s = pow_of_2/2;
        if (threadIdx.x < s) 
        { 
            if(threadIdx.x + s < blockDim.x)
                wordsFound[threadIdx.x] += wordsFound[threadIdx.x + s];
        }
        __syncthreads();
    }
    else
        s = blockDim.x;
    for(s=s/2; s>0; s=s/2) 
    {
        if (threadIdx.x < s) 
        {
            wordsFound[threadIdx.x] += wordsFound[threadIdx.x + s];
        }
        __syncthreads();
    }
    //kind of reduction i want to amount all the result into cuda_results at the res index(that we send from myFunOmp.c) .
    if(threadIdx.x == 0)
    {
        sum = wordsFound[0];
        atomicAdd(&cuda_results[resIndex],sum);
    }
    __syncthreads();
    
    
    
}

int nextPowerOf2( int n)  
{  
   	int count = 0;  
      
    // First n in the below condition  
    // is for the case where n is 0  
    if (n && !(n & (n - 1)))  
        return n;  
      
    while( n != 0)  
    {  
        n >>= 1;  
        count += 1;  
    }  
      
    return 1 << count;  
}




void gpuDecryption(unsigned int key, char* known_words_array_cuda,char* input_str_cuda,unsigned char* possible_plaintext_str_cuda,int* cuda_results,int known_words_length,int input_length,int resIndex){
    //initalization
    int numThreads;
    int numBlocks;
    int extraBlock;
    
    cudaError_t cudaStatus;
    cudaDeviceProp props;

    //Properties for the specified device
    cudaGetDeviceProperties(&props,0);

    //calculate num threads needed according to input length
    numThreads = props.maxThreadsPerBlock < input_length ? props.maxThreadsPerBlock : input_length;
    numBlocks = input_length/numThreads;
    extraBlock = input_length%numThreads != 0;

    //sending the paramters with the for calculate the xor with the possible key that we get from the function that call us (calculatePossibleKey at myFuncOmp.c)
    kernelXor<<<numBlocks+extraBlock,numThreads>>>(key,input_str_cuda,possible_plaintext_str_cuda,input_length);

    //wait for the kernel
    cudaStatus = cudaDeviceSynchronize();

    //if there is any problem i print it into the terminal.
    if(cudaStatus != cudaSuccess){
        printf("Error in kernelxor\n");
        return ;
    }

    //calculate the number of threads needed for finding the words same way as i calculate for the input length.
    numThreads = props.maxThreadsPerBlock < known_words_length ? props.maxThreadsPerBlock : known_words_length;

    //calculate the number of block needed.
    numBlocks = known_words_length/numThreads;
    extraBlock = known_words_length%numThreads != 0;
    int pow_of_2 = nextPowerOf2(numThreads);
    //calling the find words at the known words array of cuda.
    kernelFindWords<<<numBlocks+extraBlock,numThreads>>>(possible_plaintext_str_cuda,input_length,known_words_array_cuda,known_words_length,cuda_results,resIndex,pow_of_2*2);

    //wait for the kernel
    cudaStatus = cudaDeviceSynchronize();
    if(cudaStatus != cudaSuccess){
        printf("Error in kernelFindWords\n");
        return ;
    }
}


