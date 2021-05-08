#include "myFuncOmp.h"
#include "mpi.h"

char *readStringFromFile(FILE *fp, size_t allocated_size, size_t *input_length)
{
    char *string;
    int ch;
    *input_length = 0;
    string = (char *)realloc(NULL, sizeof(char) * allocated_size);
    if (!string)
        return string;
    while (EOF != (ch = fgetc(fp)))
    {
        if (ch == EOF)
            break;
        string[*input_length] = ch;
        *input_length += 1;
        if (*input_length == allocated_size)
        {
            string = (char *)realloc(string, sizeof(char) * (allocated_size += EXTEND_SIZE));
            if (!string)
                return string;
        }
    }
    return (char *)realloc(string, sizeof(char) * (*input_length));
}

bool checkIfMatch(unsigned char ch1, unsigned char ch2)
{   //if there is a match then return true else return false.
    return ch1 == ch2 ? true : false;
}

bool checkWordAndCipher(int input_length, unsigned char *possible_plaintext_str, char *word)
{

    int i = 0;
    int k = 0;
    int numOfSuccesses = 0;
    int wordLength = strlen(word);
    int in_word = 0;

    for (i; i < input_length; i++)
    {   //we check until we get to the word length 
        if (k < wordLength)
        {   //if i have found space i want to continue and now count it.
            if (possible_plaintext_str[i] == ' ')
            {
                in_word = 0;
                continue;
            }
            //taking care of the left or right edge of the word.
            if (in_word == 0 || k > 0)
            {   //check it there is a match bewteen two characters.
                if (checkIfMatch((unsigned char)word[k], possible_plaintext_str[i]))
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

int checkPossiblePlaintext(int known_words_length, char **known_words_array, int input_length, unsigned char *possible_plaintext_str)
{

    //initizalize the values for the calculation
    int maxNumSuccess = 0;
    int i = 0;
    for (i = 0; i < known_words_length; i++)
    {
        //check if word match
        if (checkWordAndCipher(input_length, possible_plaintext_str, known_words_array[i]))
            maxNumSuccess += 1;
    }
    
    return maxNumSuccess;
}

int decryption(unsigned char *key, int input_length, size_t key_len, int known_words_length, char *input_str, char **known_words_array, unsigned char *possible_plaintext_str, unsigned char *tempForCalCulate)
{
    //initizalize the values for the calculation
    int i, j = 0;
    //my flag
    int num_found;
    //divide the byte into bits as we asked to in the project
    int key_len_in_bytes = key_len / BITS_IN_BYTE;

    //i prefer working with unsigned char for disapear the fff at the begining.
    memcpy(tempForCalCulate, input_str, input_length + 1);

    for (i = 0; i < input_length; i++, j++)
    { //calculate xor and save it at possible plaintext.
        if (j == key_len_in_bytes)
            j = 0;
        //the xor calculation.
        possible_plaintext_str[i] = tempForCalCulate[i] ^ key[j];
    }

    //send the possible plaintext and the known words array to function that will return boolean if is it the solution.
    num_found = checkPossiblePlaintext(known_words_length, known_words_array, input_length, possible_plaintext_str);
    //return the number of words that i have found
    return num_found;
}

void printDecryption(int input_length, int known_words_length, unsigned int key, char **known_words_array, unsigned char *possible_plaintext_str)
{
    printf("possible :  %s\n", possible_plaintext_str);
    printf("Plaintext was found!\n");
    printf("The key is: %x\n", key);
    printf("The words was found: \n");
    for (int i = 0; i < known_words_length; i++)
    {
        if (checkWordAndCipher(input_length, possible_plaintext_str, known_words_array[i]))
            printf("%s \n", known_words_array[i]);
    }
    printf("\n");
}

long calculatesPossibleKeys(int pid, int num_procs, long min, long max, int known_words_length, int input_length, size_t key_len, char *input_str, char **known_words_array, char *known_words_pack)
{   //initalizing the variable for the calculation as you can see it is outside the pragma - cus' i want it to be shared between all the threads.
    int j;
    long max_words_found, best_key;
    bool finalCalculation = false;
    char *known_words_array_cuda = 0;
    char *input_str_cuda = 0;
    long *cpu_thread_results;
    long *proccess_results;
    unsigned char *possible_plaintext_str_cuda = 0;
    int *cuda_results = 0;
    int *cuda_results_cpu;
    int cuda_work = (max - min) / NUM_THREADS;

    //allocating memory for cuda computetion
    cudaAllocateString(&known_words_array_cuda, known_words_length * (MAX_STRING_LENGTH));
    cudaAllocateString(&input_str_cuda, input_length);
    cudaAllocateString((char **)&possible_plaintext_str_cuda, input_length);
    cudaAllocateIntArray(&cuda_results, cuda_work);
    cuda_results_cpu = (int *)malloc(sizeof(int) * cuda_work);
    cpu_thread_results = (long *)malloc(sizeof(long) * NUM_THREADS * 2);
    //copy the known words and the input string that have giving from the user.
    cudaCopyToDevice(known_words_pack, known_words_array_cuda, known_words_length * (MAX_STRING_LENGTH));
    cudaCopyToDevice(input_str, input_str_cuda, input_length);

#pragma omp parallel num_threads(NUM_THREADS)
    {   //each of the varibales here is private for each threads.
        long i, padded_key;
        unsigned char *possible_plaintext_str = (unsigned char *)malloc(input_length * sizeof(unsigned char));
        unsigned char *tempForCalCulate = (unsigned char *)malloc(input_length * sizeof(unsigned char));
        int plainTextFound;
        const int thread_id = omp_get_thread_num();
        int resIndex = 0;
        int thread_work = cuda_work;
        int my_min = min + thread_id * thread_work;
        int my_max = my_min + thread_work;
        cpu_thread_results[thread_id * 2 + 1] = -1;
        if (thread_id == NUM_THREADS - 1)
            my_max = max;
        if (thread_id == 0)//only if the thread id is 0 we want the calculation for him in cuda.
        {
            for (i = my_min; i < my_max; i++)
            {   //create the padded key for the calculation.
                padded_key = pad_key(i, key_len / BITS_IN_BYTE);
                //i send it to the calculate the possible key on the gpu.
                gpuDecryption(padded_key, known_words_array_cuda, input_str_cuda, possible_plaintext_str_cuda, cuda_results, known_words_length, input_length, resIndex);
                resIndex++;
            }
            //we copy to the cpu.
            cudaCopyIntToHost(cuda_results_cpu, cuda_results, cuda_work);
            for (i = 0; i < cuda_work; i++)
            {   //cpu_thread_results holding the key and the result.
                if (cuda_results_cpu[i] > cpu_thread_results[thread_id * 2 + 1])
                {
                    padded_key = pad_key(min + i, key_len / BITS_IN_BYTE);
                    //at the odd places in the array we save the results.
                    cpu_thread_results[thread_id * 2 + 1] = cuda_results_cpu[i];
                    //at the even places in the array we save the keys.
                    cpu_thread_results[thread_id * 2] = padded_key;
                }
            }
        }
        else
        {
            for (i = my_min; i < my_max; i++)
            {
                //send the key for xor with the cipher text
                padded_key = pad_key(i, key_len / BITS_IN_BYTE);
                //plain text found hold the number of words that have been discovered by the possible key.
                plainTextFound = decryption((unsigned char *)&padded_key, input_length, key_len, known_words_length, input_str, known_words_array, possible_plaintext_str, tempForCalCulate);
                //if there is another key the giving us better result, we want to update the maximum.
                if (plainTextFound > cpu_thread_results[thread_id * 2 + 1])
                {   //we save the results at the odd place.
                    cpu_thread_results[thread_id * 2 + 1] = plainTextFound;
                    //we save the key at the even place
                    cpu_thread_results[thread_id * 2] = padded_key;
                }
            }
        }
        //no longer need to ise the arrays we can free the allocation
        free(possible_plaintext_str);
        free(tempForCalCulate);
    }
    //finding the maximum - we want the best result that the best key has found.
    max_words_found = -1;
    for (j = 0; j < NUM_THREADS; j++)
    {
        if (cpu_thread_results[j * 2 + 1] > max_words_found)
        {   //save it in variables with appropriate name.
            max_words_found = cpu_thread_results[j * 2 + 1];
            best_key = cpu_thread_results[j * 2];
        }
    }
    //if the process is 0 , then i want him to create an long array for the best key and the max words found.
    if (pid == ROOT)
    {
        proccess_results = (long *)malloc(sizeof(long) * num_procs * 2);
    }
    //process 1 will send the best key and the max words found, process 0 will gathering the them into the process result that we have created.
    MPI_Gather(&best_key, 1, MPI_LONG, proccess_results, 1, MPI_LONG, ROOT, MPI_COMM_WORLD);
    MPI_Gather(&max_words_found, 1, MPI_LONG, proccess_results + num_procs, 1, MPI_LONG, ROOT, MPI_COMM_WORLD);
    //the root is take the data that he gathering and put it into the variables.
    if (pid == ROOT)
    {

        for (j = 0; j < num_procs; j++)
        {
            if (proccess_results[num_procs + j] > max_words_found)
            {
                max_words_found = proccess_results[num_procs + j];
                best_key = proccess_results[j];
            }
        }
        //we do the same calculation for saving them and print them - it cost nothing according to the heavy calculation that i have already done above.
        unsigned char *possible_plaintext_str = (unsigned char *)malloc(input_length * sizeof(unsigned char));
        unsigned char *tempForCalCulate = (unsigned char *)malloc(input_length * sizeof(unsigned char));
        decryption((unsigned char *)&best_key, input_length, key_len, known_words_length, input_str, known_words_array, possible_plaintext_str, tempForCalCulate);
        printDecryption(input_length, known_words_length, (unsigned int)best_key, known_words_array, possible_plaintext_str);
        free(possible_plaintext_str);
        free(tempForCalCulate);
    }

    free(cpu_thread_results);
    free(cuda_results_cpu);
    cudaFreeFromHost(known_words_array_cuda);
    cudaFreeFromHost(cuda_results);
    cudaFreeFromHost(input_str_cuda);
    cudaFreeFromHost(possible_plaintext_str_cuda);
    //copy and sum cuda results
    return best_key;
}

long pad_key(long key, int key_size)
{
    int i;
    long padded_key = 0;
    int num_iterations = MAX_KEY_SIZE / key_size;
    for (i = 0; i < num_iterations; i++)
    {
        padded_key = padded_key << key_size * BITS_IN_BYTE;
        padded_key |= key;
    }
    return padded_key;
}

void printHelp(char *argv)
{
    fprintf(stdout, "usage: %s KEY KEY_LENGTH [options]...\nEncrypt a file using xor cipher (key length in bytes)\n", argv);
    fprintf(stdout, "    -i, --input             specify input file\n");
    fprintf(stdout, "    -o, --output            specify output file\n");
    fprintf(stdout, "    -b, --binary            read key as binary\n");
}

char **readKnownWordFromFile(FILE *knownWords, int *length)
{
    int i = 0, allocated_memory = START_SIZE;
    char **known_words_array;

    known_words_array = (char **)malloc(START_SIZE * sizeof(char *));

    known_words_array[0] = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));

    while (fscanf(knownWords, "%s", known_words_array[i]) == 1)
    {
        i++;
        if (i == allocated_memory)
        {
            allocated_memory += EXTEND_SIZE;
            known_words_array = (char **)realloc(known_words_array, allocated_memory * sizeof(char *));
        }
        known_words_array[i] = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
    }

    //subtract 1 because of the initizalize and then i++ it count unnecessary one.
    *length = i - 1;

    return known_words_array;
}

void copyToBuffer(char **known_words_array, char *known_words_array_buff, int known_words_length)
{
    
    int i = 0;
    //the for loop to fill the buffer - i transfer the known words from char** to one long array of char* , it's more easy sending char* in mpi.
    for (i; i < known_words_length; i++)
    {

        memcpy(known_words_array_buff + i * MAX_STRING_LENGTH, known_words_array[i], MAX_STRING_LENGTH * sizeof(char));
    }
}

char **copyFromBuffer(char *known_words_array_buff, int known_words_length)
{

    char **returnedArray = (char **)malloc(sizeof(char *) * known_words_length);
    //the for loop that i use to for copy from the buffer that i have created for the sending in mpi , now we return it to be as the original.
    for (int i = 0; i < known_words_length; i++)
    {
        returnedArray[i] = (char *)malloc(sizeof(char) * MAX_STRING_LENGTH);
        memcpy(returnedArray[i], known_words_array_buff + i * MAX_STRING_LENGTH, MAX_STRING_LENGTH * sizeof(char));
    }

    return returnedArray;
}

void freeMemory(char **known_words_array, int length)
{   //free allocation for the known words array - is the only array of char**
    for (int i = 0; i < length; i++)
        free(known_words_array[i]);
    free(known_words_array);
}