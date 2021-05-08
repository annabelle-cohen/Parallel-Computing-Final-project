#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
//#include "mpi.h"
#include <omp.h>

enum ranks{ROOT,SLAVE};

//my struct for the cipher
typedef struct cipher
{
    int input_length;
    int known_words_length;
    int key_len;
    long num_keys;
    char  *input_str;
    char **known_words_array; 
} Cipher;

#define NUM_ATTRIBUTES 6
#define START_SIZE 32
#define EXTEND_SIZE 32
#define MAX_STRING_LENGTH 32
#define MAX_KEY_SIZE 4
#define BITS_IN_BYTE 8
#define NUM_THREADS 4
#define NUM_WORDS_THRESH 2

//omp functions
void copyToBuffer(char **known_words_array,char *known_words_array_buff, int known_words_length);
char **copyFromBuffer(char *known_words_array_buff, int known_words_length);
char *readStringFromFile(FILE *fp, size_t allocated_size, size_t *input_length);
bool checkIfMatch(unsigned char ch1,unsigned char ch2);
bool checkWordAndCipher(int input_length,unsigned char *possible_plaintext_str,char *word);
int checkPossiblePlaintext(int known_words_length,char **known_words_array, int input_length,unsigned char *possible_plaintext_str);
int decryption(unsigned char *key, int input_length, size_t key_len,int known_words_length,char *input_str,char **known_words_array,unsigned char *possible_plaintext_str, unsigned char* tempForCalCulate);
void printDecryption(int input_length,int known_words_length,unsigned int key ,char **known_words_array, unsigned char *possible_plaintext_str);
long calculatesPossibleKeys(int pid, int num_procs, long min, long max, int known_words_length,int input_length, size_t key_len,char *input_str,char **known_words_array,char* known_words_pack);
void printHelp(char *argv);
char **readKnownWordFromFile(FILE *knownWords, int *length);
void freeMemory(char **known_words_array,int length);
long pad_key(long key,int  key_size);

//cuda functions
int cudaAllocateIntArray(int ** ptr,int num_elemnts);
int cudaAllocateString(char **str,int str_len);
int cudaCopyToDevice(char* cpu_mem,char* gpu_mem,int length);
int cudaCopyToHost(char* gpu_mem,char* cpu_mem,int length);
int cudaCopyIntToHost(int* cpu_mem,int* gpu_mem,int length);
void gpuDecryption(unsigned int key, char* known_words_array_cuda,char* input_str_cuda,unsigned char* possible_plaintext_str_cuda,int* cuda_results,int known_words_length,int input_length,int resIndex);
int nextPowerOf2( int n);
void cudaFreeFromHost(void* ptr);
