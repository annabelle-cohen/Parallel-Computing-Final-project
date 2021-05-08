#include "myFuncOmp.h"
#include <time.h>
#include "mpi.h"



int main(int argc, char *argv[])
{
    //variables
    FILE *input, *known_words;
    MPI_Status status;
    Cipher cipher;
    char* buff;
    int my_rank, num_procs,buff_size,isNotFound=1;
    long min = 0;
    long best_key;
    double start;



    //initialize
    MPI_Init(&argc, &argv);

    /* find out process rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    /* find out number of processes */
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (my_rank == ROOT)
    {   
        //i prefer define local variable of size_t for count the input length.     
        size_t input_length;

        if (argc < 3 || argc > 4)
        {
            printHelp(argv[0]);
             MPI_Abort(MPI_COMM_WORLD,-1);
        }
        
        //reading the length of key
        cipher.key_len = atoi(argv[1]);
        
        //number of possible keys 2^length
        cipher.num_keys = (long)(pow(2,cipher.key_len));

        printf("key len: %ld\n",cipher.num_keys);

        //open the input file that include the ciphertext
        input = fopen(argv[2], "r");


        //if the input does'nt exist or any other problem we stop immediately
        if (!input)
        {
            fprintf(stderr, "Error opening file\n");
             MPI_Abort(MPI_COMM_WORLD,-1);
        }
        
        //if there a file that include the known words
        if (argc == 4)
        {
            known_words = fopen(argv[3], "r");
        }
        else //if not than i gave default path for known words
        {   
            known_words = fopen("/usr/share/dict/words", "r");
        }

        //if the known_words does'nt exist or any other problem we stop immediately
        if (!known_words)
        {
            fprintf(stderr, "Error opening file\n");
            MPI_Abort(MPI_COMM_WORLD,-1);
        }

        //reading all the known words from the file into array.
        cipher.known_words_array = readKnownWordFromFile(known_words,&cipher.known_words_length);
        //reading the cipher text into value
        cipher.input_str = readStringFromFile(input, START_SIZE,&input_length);

        //i copy the size_t to variable that his type is int.
        cipher.input_length = input_length;

        buff_size = (cipher.known_words_length*MAX_STRING_LENGTH);

        buff = (char*)malloc(sizeof(char)*buff_size);

        //my own pack
        copyToBuffer(cipher.known_words_array,buff,cipher.known_words_length);
        
        
        //sending input cipher text
         MPI_Send(&cipher.input_length,1,MPI_INT,SLAVE,0,MPI_COMM_WORLD);
         MPI_Send(cipher.input_str,cipher.input_length,MPI_CHAR,SLAVE,0,MPI_COMM_WORLD);

        //send keys
        long otherHalf = cipher.num_keys/2;
         MPI_Send(&cipher.key_len,1,MPI_INT,SLAVE,0,MPI_COMM_WORLD);
         MPI_Send(&otherHalf,1,MPI_LONG,SLAVE,0,MPI_COMM_WORLD);

        //send the buff - our pack
         MPI_Send(&buff_size,1,MPI_INT,SLAVE,0,MPI_COMM_WORLD);
         MPI_Send(buff,buff_size,MPI_CHAR,SLAVE,0,MPI_COMM_WORLD);

        min = cipher.num_keys/2;

      fclose(input);//close the input file.
      fclose(known_words);//close the known word file.

      start = MPI_Wtime();

    }
    else
    {
    //recieve input with cipher text
     MPI_Recv(&cipher.input_length,1,MPI_INT,ROOT,MPI_ANY_TAG,MPI_COMM_WORLD,&status);   
     cipher.input_str = (char*)malloc(sizeof(char)*cipher.input_length*MAX_STRING_LENGTH);
     MPI_Recv(cipher.input_str,cipher.input_length,MPI_CHAR,ROOT,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
  
    //recieve keys
     MPI_Recv(&cipher.key_len,1,MPI_INT,ROOT,MPI_ANY_TAG,MPI_COMM_WORLD,&status); 
     MPI_Recv(&cipher.num_keys,1,MPI_LONG,ROOT,MPI_ANY_TAG,MPI_COMM_WORLD,&status); 

    //recieve the pack
     MPI_Recv(&buff_size,1,MPI_INT,ROOT,MPI_ANY_TAG,MPI_COMM_WORLD,&status);   
     buff = (char*)malloc(sizeof(char)*buff_size);

     MPI_Recv(buff,buff_size,MPI_CHAR,ROOT,MPI_ANY_TAG,MPI_COMM_WORLD,&status);

  
     //unpack
     cipher.known_words_length = buff_size/MAX_STRING_LENGTH; 
     cipher.known_words_array = copyFromBuffer(buff,cipher.known_words_length);


     min = 0;


    }

    //sending for finding possibe key
    best_key = calculatesPossibleKeys(my_rank,num_procs,min,cipher.num_keys,cipher.known_words_length,cipher.input_length,cipher.key_len,cipher.input_str,cipher.known_words_array,buff);
    
    //we need to free the allocate memory at the end.
    freeMemory(cipher.known_words_array,cipher.known_words_length);
    free(buff);//i dont need the buff anymore.
    free(cipher.input_str);

    MPI_Finalize();
    return 0;
}
