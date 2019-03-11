#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <mpi.h>
#include <time.h>

#define MAXLINE 300
#define WinLen 20
#define WinThres 19
#define min(X, Y) (((X) < (Y)) ? (X) : (Y))
#define TAG_SEND_DATA 0
#define TAG_DONE 1


int countLinesInFile(FILE *fp) { 
  int count = 0;  // Line counter (result) 
  char c;  // To store a character read from file 
  if (fp == NULL) { 
    printf("Could not open file"); 
    return 0; 
  } 
  // Extract characters from file and store in character c 
  for (c = getc(fp); c != EOF; c = getc(fp)) {
    if (c == '\n')
      count = count + 1; 
    }
  printf("The file has %d lines\n", count); 
  return count; 
} 

int main(int argc,char **argv) {
  //start time of program
  clock_t begin_time = clock();
  //open files for input and output
  FILE * Fin= fopen(argv[1], "r");
  int linecount = countLinesInFile(Fin);
  fseek(Fin, 0, SEEK_SET);
  FILE * Fout= fopen(argv[2], "w");
  int i;
  int Line;
  //create buffer array to store data from input file
  char ** buffer;
  buffer=(char**)malloc(sizeof(char*)*linecount);
  for(i=0;i<linecount;i++) {
    buffer[i]=(char*)malloc(sizeof(char)*MAXLINE );
  }
  // read line-by-line the lines of the file and store each in the array named buffer
  size_t len = 0;
  for(Line=0;Line<linecount;Line++) {
    getline(&buffer[Line], &len, Fin);
  }

  int rank, size;
  char message[MAXLINE];
  //create MPI processes
  MPI_Init (&argc, &argv);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Comm_size (MPI_COMM_WORLD, &size);
  //calculate how many sequences are per process
  int sequence_number = linecount/4;
  int sequence_per_rank = sequence_number/(size-1);
  int extra_sequences = sequence_number%(size-1);
  printf("Each thread has %d out of %d sequences\n", sequence_per_rank, sequence_number);
  if (rank == 0) { //MASTER
    int recipient = 1;
    for (int i=0; i<linecount; i+=4){
      //for every sequence, send the next 4 lines to one slave and expect his reply
      printf("Master: %d Sending to process %d lines %d - %d\n", rank, recipient, i, (i+3));
      MPI_Send(buffer[i], MAXLINE, MPI_CHAR, recipient, TAG_SEND_DATA, MPI_COMM_WORLD);
      MPI_Recv(message, 4, MPI_CHAR, recipient, TAG_DONE, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Send(buffer[i+1], MAXLINE, MPI_CHAR, recipient, TAG_SEND_DATA, MPI_COMM_WORLD);
      MPI_Recv(message, 4, MPI_CHAR, recipient, TAG_DONE, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Send(buffer[i+2], MAXLINE, MPI_CHAR, recipient, TAG_SEND_DATA, MPI_COMM_WORLD);
      MPI_Recv(message, 4, MPI_CHAR, recipient, TAG_DONE, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Send(buffer[i+3], MAXLINE, MPI_CHAR, recipient, TAG_SEND_DATA, MPI_COMM_WORLD);
      MPI_Recv(message, 4, MPI_CHAR, recipient, TAG_DONE, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      recipient++;
      //if previous recipient was the last slave, start again with slave with rank=1
      if (recipient > size-1) {
        recipient = 1;
      }
    }
    printf("Master: %d has FINISHED\n", rank);
  } else { //SLAVES
    //create a buffer to store sequence
    char slavebuf[4][MAXLINE];
    //if there are extra sequences, then get one extra
    if (rank<=extra_sequences){
      sequence_per_rank++;
    }
    for (int i=0; i<sequence_per_rank; i++){
      //receive sequence from MASTER and reply with "done"
      printf("Thread: %d Waiting to receive from MASTER on loop %d out of %d\n", rank, (i+1), sequence_per_rank);
      MPI_Recv(slavebuf[0], MAXLINE, MPI_CHAR, 0, TAG_SEND_DATA, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Send("done", 4, MPI_CHAR, 0, TAG_DONE, MPI_COMM_WORLD);
      MPI_Recv(slavebuf[1], MAXLINE, MPI_CHAR, 0, TAG_SEND_DATA, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Send("done", 4, MPI_CHAR, 0, TAG_DONE, MPI_COMM_WORLD);
      MPI_Recv(slavebuf[2], MAXLINE, MPI_CHAR, 0, TAG_SEND_DATA, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Send("done", 4, MPI_CHAR, 0, TAG_DONE, MPI_COMM_WORLD);
      MPI_Recv(slavebuf[3], MAXLINE, MPI_CHAR, 0, TAG_SEND_DATA, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Send("done", 4, MPI_CHAR, 0, TAG_DONE, MPI_COMM_WORLD);
      //sliding window method
      int MaxLen=strlen(slavebuf[1])-1;
      printf("Thread: %d Number of Nucelotides %d:\n", rank, MaxLen);
      //if lines 2 and 4 have different lengths, trim them
      if (strlen(slavebuf[3])!=strlen(slavebuf[1])) {
        int len_a = strlen(slavebuf[3]);
        int len_b = strlen(slavebuf[1]);
        if ( len_a > len_b ){
            slavebuf[3][len_b] = '\0';
        } else {
            slavebuf[1][len_a] = '\0';
        }
      }
      //calculate average quality in window
      float Qual=0;
      int start=0;
      int end=start+WinLen;
      Qual=WinThres+1;
      while ((end<=MaxLen)&&Qual>WinThres) {
        Qual=0;
        for (int k=start;k<end;k++){
          Qual+=slavebuf[3][k]-33;
        }
        Qual/=WinLen;
        //slide window
        start++;
        end=start+WinLen;
      }
      start--;
      printf("Thread: %d Nucelotides after position %d have mean window quality under %d\n",rank,start,WinThres);
      //trim lines according to point where quality<threshold
      strncpy(slavebuf[1],slavebuf[1],start);
      slavebuf[1][start]='\0';
      strncpy(slavebuf[3],slavebuf[3],start);
      slavebuf[3][start]='\0';
      //store lines in output file
      fprintf(Fout,"%s",slavebuf[0] );
      fprintf(Fout,"%s\n",slavebuf[1] );
      fprintf(Fout,"%s",slavebuf[2] );
      fprintf(Fout,"%s\n",slavebuf[3] );
    }
    printf("Thread: %d has FINISHED\n", rank);
  }
  MPI_Finalize();
  //close files opened
  fclose(Fin);
  fclose(Fout);
  //calculate time for program
  clock_t end_time = clock();
  double time_spent = (double)(end_time - begin_time) / CLOCKS_PER_SEC;
  printf("Time for MPI program is %f\n", time_spent);
  //free memory allocated for buffer
  for (i=0;i<4;i++){
    free(buffer[i]);
  }
  free(buffer);
  exit(0);
}