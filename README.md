# Parallel-Computing-Final-project
***link to short video of the project is also in About***


We were asked to write a program that receives input:
A file that contains encrypted words
The encryption operation was done with some key and simple xor.

The out put:
Decrypted text with the key that decoded the encrypted text.

A brief explanation of the actions:

MPI: 
First in the file parallelProject.c , I'm using MPI , i made two processes: ROOT(0),SLAVE(1)
ROOT: Handles all data reading, And of course takes care of calculating the amount of keys(exponent of two by the length that was given, In order to get all the possible permutations from the length).
Now that I have gathered all the data I need to continue I want to give half of the work to SLAVE.
You can see the sequence of SEND to slave.

After we have divided the work and left the if else the processes continue in the program and call
To function : calculatePossibleKeys.

OMP:
I start the calculation when I am in a function calculatePossibleKeys, 
The number of threads I chose is 4 (as the number of cores), thread 0 will perform the calculation with cuda while the rest will calculate the cpu.
I will now describe what happens to those who do not thread 0, Make a call to function decryption, there the calculation of xor with the possible key (Which is actually i).
then i save the results in an array possible_plaintext_str and call to the function checkPossiblePlainText .

CUDA:
When it comes to thread 0 , i fet into the if statement. 
A call to cudaDecryption is then made with conceptually identical parameters to that of a decoder in the CPU, adapted only for calculation in gpu.
cudaDecryption: First initializes and does all the calculations needed for example - number of blocks, number of threads depending on the length of the input or the length of the known words.
resIndex:An index that I use to summarize the possible results into cudaResults every time in another index that I promote through the function that calls cudaDecryption, in addition there is 
next_pow_of_2 - I use it to calculate a exponent of 2, because I noticed that when it is a number that is not a exponent of 2 it is wrong calculation,so this test is also done in kernalFindsWords.

