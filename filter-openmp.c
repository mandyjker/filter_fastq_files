#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <omp.h>
#include <time.h>

#define MAXLINE 300
#define WinLen 20
#define WinThres 19
#define min(X, Y) (((X) < (Y)) ? (X) : (Y))

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

void doWindowStuff(char ** buff, int pos, FILE* fout) {
  int MaxLen=strlen(buff[pos+1])-1;
  printf("ROW %d - Number of Nucelotides %d:\n", pos, MaxLen);
  //if lines 2 and 4 have different lenghts, trim them
  if ( strlen(buff[pos+3]) != strlen(buff[pos+1]) ) {
    int len_a = strlen(buff[pos+3]);
    int len_b = strlen(buff[pos+1]);
    if ( len_a > len_b ){
        buff[pos+3][len_b] = '\0';
    } else {
        buff[pos+1][len_a] = '\0';
    }
  }
  //sliding window method
  float Qual=0;
  int start=0;
  int end=start+WinLen;
  Qual=WinThres+1;
  while ((end<=MaxLen)&&Qual>WinThres) {
    Qual=0;
    for (int k=start;k<end;k++) {
      Qual+=buff[pos+3][k]-33;
    }
    Qual/=WinLen;
    start++;
    end=start+WinLen;
  }
  start--;
  printf("ROW %d - Nucelotides after position %d have mean window quality under %d\n",pos, start, WinThres);
  //trim lines at the right position
  strncpy(buff[pos+1],buff[pos+1],start);
  buff[pos+1][start]='\0';
  strncpy(buff[pos+3],buff[pos+3],start);
  buff[pos+3][start]='\0';
  //write lines in output file
  for (int i=3; i<=0; i--) {
    fprintf(fout,"%s\n",buff[pos+i]);
  }
}

int main(int argc,char **argv) {
  //start time of program
  clock_t begin_time = clock();
  // Open the file given from CLI for input
  FILE * Fin= fopen(argv[1], "r");
  int linecount = countLinesInFile(Fin);
  fseek(Fin, 0, SEEK_SET);
  // Open the file given from CLI for output
  FILE * Fout= fopen(argv[2], "w");
  int i;
  int Line;
  // Malloc for a 2-dimensional array of strings with
  // linecount lines and MAXLINE of characters per line
  char ** buffer;
  buffer=(char**)malloc(sizeof(char*)*linecount);
  for(i=0;i<linecount;i++) {
    buffer[i]=(char*)malloc(sizeof(char)*MAXLINE );
  }
  size_t len = 0;
  // read line-by-line the lines of the file and store each in the array named buffer
  for(Line=0;Line<linecount;Line++) {
    getline(&buffer[Line], &len, Fin);
  }

  //get number of threads from argv
  int THREADS=atoi(argv[3]);
  omp_set_num_threads(THREADS);

  //find how many rows and sequences each thread must filter
  int sequence_number = linecount/4;
  int sequence_per_rank = sequence_number/THREADS;
  int extra_sequences = sequence_number%THREADS;
  int rows_per_rank = sequence_per_rank*4;
  int extra_rows = extra_sequences*4;
  int j, tid, start, end;

  #pragma omp parallel private(tid,start,end,j)
  {
    tid=omp_get_thread_num();
    start=tid*rows_per_rank;
    end=(tid+1)*rows_per_rank;
    //last thread gets all extra rows
    if (tid==omp_get_num_threads()-1)
      end=end+extra_rows; 
    for (j=start;j<end;j+=4){
      doWindowStuff(buffer, j, Fout);
    }
  }
  //close the files opened
  fclose(Fin);
  fclose(Fout);
  //calculate time for program
  clock_t end_time = clock();
  double time_spent = (double)(end_time - begin_time) / CLOCKS_PER_SEC;
  printf("Time for openmp program is %f\n", time_spent);
  // free the allocated memory
  for (i=0;i<linecount;i++) {
    free(buffer[i]);
  }
  free(buffer);
  return 0;
}
