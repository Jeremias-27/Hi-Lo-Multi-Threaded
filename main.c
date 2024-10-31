#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

// global variables
int guess[3];
int cmp[2];
int rdy[4];

//condition and mutex variables
pthread_cond_t cnd[3] = {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER};
pthread_mutex_t mtx[3] = {PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};

// function declarations
int checkThread(int, const char*);
void* startThread1(void*);
void* startThread2(void*);
void* startRefThread(void*);

int main(int argc, char* argv[])
{
   pthread_t thr[3]; // threads
   
   srand(time(NULL)); // seeding number generator
   
   for(int i = 0; i < 4; i++) {rdy[i] = 0;} // initializing rdy array

   // spawning threads
   checkThread(pthread_create(&thr[0], NULL, startThread1, NULL), "start Thread1");
   checkThread(pthread_create(&thr[1], NULL, startThread2, NULL), "start Thread2");
   checkThread(pthread_create(&thr[2], NULL, startRefThread, NULL), "start RefThread");

   checkThread(pthread_join(thr[2], NULL), "join RefThread"); // joining referee thread

   return 0;
}

// if val is greater than 0 output what went wrong and exit, else return
int checkThread(int val, const char* msg)
{
    if(val > 0)
    {
        errno = val;
        perror(msg);
        exit(EXIT_FAILURE);
    }
    return val;
}

//player 1 thread function
void* startThread1(void* arg)
{
    // bounds for guesses
    int min1;
    int max1;

    while(1) // game loop
    {
        // initialize bounds 
        min1 = 0;
        max1 = 100;
        
        checkThread(pthread_mutex_lock(&mtx[2]), "mutex_lock"); // lock referee mutex

        // wait for referee to signal we can start
        while(rdy[2] == 0)
        {
            pthread_cond_wait(&cnd[2], &mtx[2]);
        }
        
        rdy[2] = 0; // reset rdy flag
        
        checkThread(pthread_mutex_unlock(&mtx[2]), "mutex_unlock"); // unlock ref mutex

        while(1) // guess loop
        {
            // guess[0] is player 1's guess
            // player 1's strategy is to guess the average of its low and high bounds
            guess[0] = (min1 + max1) / 2;
            
            checkThread(pthread_mutex_lock(&mtx[0]), "mutex_lock"); // lock player 1 mutex

            while(rdy[0] == 0) // wait for ref to check guess
            {
                pthread_cond_wait(&cnd[0], &mtx[0]);
            }
            
            rdy[0] = 0; // reset flag indicating guess has been checked
            
            checkThread(pthread_mutex_unlock(&mtx[0]), "mutex_unlock"); // unlock player 1 mutex

            if(cmp[0] == 0 || cmp[1] == 0) {break;} // if player 1 or player 2 won break out of guess loop
            else if(cmp[0] < 0) {min1 = guess[0];} // if guess was too low set low bound to current guess
            else if(cmp[0] > 0) {max1 = guess[0];} // if guess was too high set high bound to current guess
        }
    }
    
    pthread_exit(NULL); // exit player 1 thread
}

// player 2 thread function
void* startThread2(void* arg)
{
    // bounds for guesses
    int min2;
    int max2;

    while(1) // game loop
    {
        // initialize bounds
        min2 = 1;
        max2 = 100;
        
        checkThread(pthread_mutex_lock(&mtx[2]), "mutex_lock"); // lock referee mutex

        while(rdy[3] == 0) // wait for ref to signal we can start
        {
            pthread_cond_wait(&cnd[2], &mtx[2]);
        }
        
        rdy[3] = 0; // reset ready flag
        
        checkThread(pthread_mutex_unlock(&mtx[2]), "mutex_unlock"); // unlock ref mutex

        while(1) // guess loop
        {
            // guess[1] is player 2's guess
            // player 2's strategy is to guess a random number between its bounds(inclusive)
            guess[1] = rand() % (max2 + 1 - min2) + min2;
            
            checkThread(pthread_mutex_lock(&mtx[1]), "mutex_lock"); // lock player 2 mutex

            while(rdy[1] == 0)// wait for ref to check guess 
            {
                pthread_cond_wait(&cnd[1], &mtx[1]);
            }
            
            rdy[1] = 0; // reset flag indicating guess has been checked 
            
            checkThread(pthread_mutex_unlock(&mtx[1]), "mutex_unlock"); // unlock player 2 mutex

            if(cmp[0] == 0 || cmp[1] == 0) {break;} // if player 1 or player 2 won break out of guess loop
            else if(cmp[1] < 0) {min2 = guess[1];} // if guess was too low set low bound to current guess
            else if(cmp[1] > 0) {max2 = guess[1];} // if guess was too high set high bound to current guess
        }
    }

    pthread_exit(NULL); // exit player 2 thread
}

// referee thread function
void* startRefThread(void* arg)
{
    int p1cnt = 0; // player 1 win counter
    int p2cnt = 0; // player 2 win counter
    int tiecnt = 0; // tie counter
    
    for(int i = 0; i < 10; i++) // game loop
    {
        checkThread(pthread_mutex_lock(&mtx[2]), "mutex_lock"); // lock ref mutex 
        
        // guess[2] is the target of the game; generate random number(1-100) 3 times for randomness
        for(int i = 0; i < 3; i++) {guess[2] = rand() % (100 + 1 - 1) + 1;}
        
        // signal player 1 and 2 to start
        rdy[2] = 1;
        rdy[3] = 1;

        checkThread(pthread_cond_broadcast(&cnd[2]), "broadcast"); // wake up player 1 and player 2 threads
        checkThread(pthread_mutex_unlock(&mtx[2]), "mutex_unlock"); // unlock parent mutex

        printf("Game %d\n", i+1); // print current game being played

        while(1) // guess loop
        {
            sleep(1); // sleep for a second
            
            checkThread(pthread_mutex_lock(&mtx[0]), "mutex_lock"); // lock player 1 mutex
            checkThread(pthread_mutex_lock(&mtx[1]), "mutex_lock"); // lock player 2 mutex

            cmp[0] = guess[0] - guess[2]; // cmp[0] is player 1's feedback 
            cmp[1] = guess[1] - guess[2]; // cmp[1] is player 2's feedback

            // guess[0] is player 1's guess, guess[1] is player 2's guess, and guess[2] is the target.
            // output current guesses and target guess to track each guess
            printf("%d %d %d\n", guess[0], guess[1], guess[2]);

            // signal player 1 and player 2 that the ref has given feedback
            rdy[0] = 1;
            rdy[1] = 1;

            checkThread(pthread_cond_broadcast(&cnd[0]), "broadcast"); // wake up player 1
            checkThread(pthread_cond_broadcast(&cnd[1]), "broadcast"); // wake up player 2

            // unlock player 1 and player 2 mutexes
            checkThread(pthread_mutex_unlock(&mtx[0]), "mutex_unlock");
            checkThread(pthread_mutex_unlock(&mtx[1]), "mutex_unlock");

            // if a player guessed correctly output who won the current round, update respective counter, and break
            if(cmp[0] == 0 && cmp[1] == 0) 
            {
                tiecnt++; 
                printf("It was a tie!\n\n"); 
                break;
            }
            else if(cmp[0] == 0) 
            {
                p1cnt++; 
                printf("Player 1 won!\n\n"); 
                break;
            }
            else if(cmp[1] == 0) 
            {
                p2cnt++; 
                printf("Player 2 won!\n\n"); 
                break;
            }
            
            if(cmp[0] < 0)  // output if player 1 guessed low
            {
                printf("Player 1 guessed low!\n");
            } 
            else // if guess was not low output it was high
            {
                printf("Player 1 guessed high!\n");
            }

            if(cmp[1] < 0) // output if player 2 guessed low
            {
                printf("Player 2 guessed low!\n");
            }
            else // if guess was not low output it was high
            {
                printf("Player 2 guessed high!\n");
            }
        }
    }

    // output how many ties there were
    printf("\nNumber of ties: %d", tiecnt);
    
    // output how many times player 1 won
    printf("\nPlayer 1 won %d", p1cnt);
    printf(" times!\n");
    
    // output how many times player 2 won
    printf("Player 2 won %d", p2cnt);
    printf(" times!\n");

    // output the overall winner of the guessing game
    if(p1cnt > p2cnt) 
    {
        printf("\nPlayer 1 is the winner!\n");
    }
    else if (p1cnt < p2cnt) 
    {
        printf("\nPlayer 2 is the winner!\n");
    }
    else 
    {
        printf("\nThe game resulted in a tie!\n");
    }

    pthread_exit(NULL); // exit ref thread
}