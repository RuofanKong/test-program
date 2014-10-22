//This program code simulates the Multi-thread communication problem: Producers and consumers orderly produce and consume the products.
//Please use g++ compiler, type the command: "g++ HW12.cc -o HW12 -lpthread -fpermissive -w" to compile the source code, and generate the
//file "HW12".
//This code is written by Ruofan Kong
/*---------------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <list>
#include <unistd.h>

int Num_Product; //The number of products to be generated provided  by the user
int product_count = 1; //count the id of the products
int consume_count = 1; //count the consumption number
int buffer_size; //The size of the buffer
int buffer_count = 0; //count the current buffer node
int select_algorithm; //select the schedule algorithm: "0" for First-Come-First-Serve and "1" for Round-Robin
int quantum; //the value of quantum used for round-robin scheduling

//mutex vriables to protect the global variables
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;

//condition variables to control full-queue for producer and empty-queue for consumer
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;

//Product for both producers and consumers
struct buffer{ 
  int product_id;  //Product id
  double produce_time; //The time when the product
  int life; //The life cycle of the product
} Product;

//Product queue for both producers and consumers
std::list<struct buffer> myqueue;

//Fibonacci element generation function
int fn(int i);

int main()
{
  int i,j; //i and j track the ith producer and the jth consumer, respectively
  int Num_Producer;  //The producer number provided by the user
  int Num_Consumer;  //The consumer number provided by the user
  int seed; //The seed for random number generator

  //Pre-allocate the memory for the pointers
  int *pro = malloc(0); //*pro includes id of all the producer threads
  int *cons = malloc(0); //*cons includes id of all the consumer threads
  pthread_t *pro_thread = malloc(0); //*pro includes all the producer threads
  pthread_t *cons_thread = malloc(0); //*cons includes all the consumer threads

  //Declaraion for both products producing and consuming functions, respectively.
  void *produce(int *);
  void *consume(int *);

  printf("Please enter the producer number:\n");
  scanf("%d", &Num_Producer);
  printf("%d\n", Num_Producer);

  printf("Please enter the consumer number:\n");
  scanf("%d", &Num_Consumer);
  printf("%d\n", Num_Consumer);

  printf("Please enter the number of products to be generated:\n");
  scanf("%d", &Num_Product);
  printf("%d\n", Num_Product);

  printf("Please enter the size of the buffer for both producers and consumers:\n");
  scanf("%d", &buffer_size);
  printf("%d\n", buffer_size);

  printf("Please select the type of scheduling algrithm (Press 0 to choose First-Come-first-serve algorithm; Press 1 to choose Round-Robin algorithm): \n");
  scanf("%d", &select_algorithm);
  printf("%d\n", select_algorithm);

  if(select_algorithm == 1){
    printf("Please enter the value of quantum used for round-robin scheduling: \n");
    scanf("%d", &quantum);
    printf("%d\n", quantum);
  }
  
  printf("Please enter the seed for random number generator:\n");
  scanf("%d", &seed);
  printf("%d\n", seed);

  srand(seed);  //Initialize the random number generator.

  //Allocate the memory for producer id arrays, consumer id arrays, producer thread arrays and consumer thread arrays, according to the user inputs.
  pro = realloc(pro, sizeof(int) * Num_Producer);
  cons = realloc(cons, sizeof(int) * Num_Consumer);
  pro_thread = realloc(pro_thread, sizeof(pthread_t) * Num_Producer);
  cons_thread = realloc(cons_thread, sizeof(pthread_t) * Num_Consumer);

  //Generate "Num_Producer" producer threads
  for(i=0;i<Num_Producer;i++){
    pro[i] = i + 1;
    pthread_create(&pro_thread[i],NULL,(void*)produce,&pro[i]);
  }

  //Generate "Num_Consumer" consumer threads
  for(j=0;j<Num_Consumer;j++){
    cons[j] = j + 1;
    pthread_create(&cons_thread[j],NULL,(void*)consume,&cons[j]);
  }
  
  for(i=0;i<Num_Producer;i++)
    pthread_join(pro_thread[i],NULL);

  for(j=0;j<Num_Consumer;j++)
    pthread_join(cons_thread[j],NULL);
  
  pthread_exit(0);

  return 0;
}

void *produce(int *i){
  //produce the products, since each thread accesses the global variable "product_count", the mutual exclusion is considered
  /*----------------------------------------------------------------------------------------------------------------------*/
  while(product_count<=Num_Product){ //produce products with number "Num_product" 
    pthread_mutex_lock(&mutex1);
    struct timeval t1; //The time when the product is produced
    if(product_count<=Num_Product){ //The condition guarantees the number of total product numbers to be produced
      if(buffer_size == 0){ //buffer_size == 0 means unlimited size of product queue
        Product.product_id = product_count; //name the product id
        gettimeofday(&t1,NULL); //Timing and begin to produce the product
        Product.produce_time = t1.tv_sec + ((double)(t1.tv_usec))/1000000; //stamp the produce_time on the product with unit "seconds".
        Product.life = rand()%1024; //randomly generate the life for each product
      }else if(buffer_size<0){ //when queue size is negative, program terminates
              printf("Please enter a non-negative number!");
              exit(0);
            }else{ //the case of product queue with limited size
               if(buffer_count >= buffer_size){ //when the queue is full, stop producing the products and wait
                 pthread_mutex_lock(&mutex3);
                 pthread_cond_wait(&not_full, &mutex3);
                 pthread_mutex_unlock(&mutex3);
               }
               Product.product_id = product_count; //name the product id
               gettimeofday(&t1,NULL); //Timing and begin to produce the product
               Product.produce_time = t1.tv_sec + ((double)(t1.tv_usec))/1000000; //stamp the produce_time on the product with unit "seconds".
               Product.life = rand()%1024; //randomly generate the life for each product
             }
     printf("Producer %d has produced product %d, at time %fs with life %d\n",*i,Product.product_id,Product.produce_time,Product.life); 
     myqueue.push_back(Product); //After one product is produced, it is put in the queue
     product_count++; //Update the number of produced products
     buffer_count++; //Update the number of non-empty queue elements 
    }
   pthread_cond_signal(&not_empty);
   pthread_mutex_unlock(&mutex1);
   usleep(100000); //sleep for 100 milliseconds
  }  
}

void *consume(int *i){
  if(select_algorithm == 0){ //"0" is for the First-Come-First-Serve algorithm
    while(consume_count<=Num_Product){ //consume products with number "Num_product"
       pthread_mutex_lock(&mutex2);      
       int j;
       if(consume_count<=Num_Product){ //The condition guarantees the number of total product numbers to be consumeds
	 if(buffer_count <= 0){ //when the queue is full, stop consuming the products and wait
           pthread_mutex_lock(&mutex3);
           pthread_cond_wait(&not_empty, &mutex3);
           pthread_mutex_unlock(&mutex3);
         }
         for(j=0;j<myqueue.front().life;j++) //Iterate "life" times to consume the product
           fn(10);
         printf("Consumer %d has consumed product %d, produced at time %fs with life %d\n",*i,myqueue.front().product_id,myqueue.front().produce_time,myqueue.front().life);
         if(!myqueue.empty())
         myqueue.pop_front(); //When the product is consumed, delete the product from the queue.
         consume_count++; //Update the number of consumed products
         buffer_count--; //Update the number of non-empty queue elements 
       }
       pthread_cond_signal(&not_full);
       pthread_mutex_unlock(&mutex2);
       usleep(100000); //sleep for 100 milliseconds
    }
  }else if(select_algorithm == 1){ //"1" for the Round-Robin algorithm
         while(consume_count<=Num_Product){ //consume products with number "Num_product"
           pthread_mutex_lock(&mutex2);    
           int j;
           if(consume_count<=Num_Product){ //The condition guarantees the number of total product numbers to be consumeds
	     if(buffer_count <= 0){ //when the queue is full, stop consuming the products and wait
               pthread_mutex_lock(&mutex3);
               pthread_cond_wait(&not_empty, &mutex3);
               pthread_mutex_unlock(&mutex3);
             }
             if(myqueue.front().life>=quantum){ //when life is greater than quantum, consumer iterates "quantum" times, and put the product to the end with the updated life: life - quantum
               for(j=0;j<quantum;j++)
                 fn(10); 
               struct buffer Temp;
               Temp.product_id = myqueue.front().product_id;
               Temp.produce_time = myqueue.front().produce_time;
               Temp.life = myqueue.front().life - quantum;
               myqueue.push_back(Temp);
             }else{ //Otherwise, consume the product by iterating "life" time
                for(j=0;j<myqueue.front().life;j++)
                  fn(10);
                printf("Consumer %d has consumed product %d, produced at time %fs with life %d\n",*i,myqueue.front().product_id,myqueue.front().produce_time,myqueue.front().life);
                consume_count++; //Update the number of consumed products
                buffer_count--; //Update the number of non-empty queue elements
                pthread_cond_signal(&not_full); 
              }
               if(!myqueue.empty())
                myqueue.pop_front(); //When the product is consumed, delete the product from the queue.
            }
            pthread_mutex_unlock(&mutex2);            
            usleep(100000); //sleep for 100 milliseconds
          }
        }else //Neither "0" nor "1", exit;
           exit(0);
}  

int fn(int i){  //generate the Fibonacci array
  if(i==1)
    return 0;
  else if(i==2)
         return 1;
       else
         return (fn(i-1) + fn(i-2));
}
