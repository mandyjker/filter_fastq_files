#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
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

int main(int argc,char **argv) {
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
  int bfnum=0;
  int j;
  for (j=0; j<linecount; j+=4) {
    // The number of nucleotides in the second line or equally in the last line
    int MaxLen=strlen(buffer[j+1])-1;
    printf("ROW %d Number of Nucelotides %d:\n",j, MaxLen);
    // length of line[j+1] and line[j+3] MUST be equally, if not trim them
    if ( strlen(buffer[j+1]) != strlen(buffer[j+3]) ) {
      int len_a = strlen(buffer[j+3]);
      int len_b = strlen(buffer[j+1]);
      if ( len_a > len_b ){
          buffer[j+3][len_b] = '\0';
      } else {
          buffer[j+1][len_a] = '\0';
      }
    }
    float Qual=0;
    // start and end position of the sliding window
    int start=0;
    int end=start+WinLen;
    Qual=WinThres+1;
    /* slide the window while: the end position has not reached the end of 
    the line and the mean quality score is above the minimum threshold*/
    while ((end<=MaxLen)&&Qual>WinThres){
    // calculate the mean quality score
      Qual=0;
      for (int k=start;k<end;k++) {
        Qual+=buffer[j+3][k]-33;
      }
      Qual/=WinLen;
      //slide the window by one position to the right
      start++;
      end=start+WinLen;
    }
    start--;
    printf("Nucelotides after position %d have mean window quality under %d\n", start, WinThres);

    // trim out the filter positions from
    // the second and the last lines up to start
    strncpy(buffer[j+1],buffer[j+1],start);
    buffer[j+1][start]='\0';
    strncpy(buffer[j+3],buffer[j+3],start);
    buffer[j+3][start]='\0';
    //write the filtered fastq to the output file
    for (int i=0; i<4; i++) {
      fprintf(Fout,"%s\n",buffer[j+i]);
    }
  }
  //close the files opened
  fclose(Fin);
  fclose(Fout);
  //calculate time for program
  clock_t end_time = clock();
  double time_spent = (double)(end_time - begin_time) / CLOCKS_PER_SEC;
  printf("Time for serial program is %f\n", time_spent);
  // free the allocated memory
  for (i=0;i<linecount;i++) {
    free(buffer[i]);
  }
  free(buffer);
  return 0;
}
