#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "id3read.h"
int main(int argc, char* argv[]){  FILE* infile;  if(argc<2){    printf("Usage: id3 filename.mp3\n");    return -1;  }    if(!strcmp(argv[1], "-v")){    // we have a list to fail :P    
	int i;    for(i=2; i<argc; i++){      infile=fopen(argv[i], "r");      if(infile){        if(GetID3Headers(infile, 1)>1)          printf("%s\n", argv[i]);        fclose(infile);      }    }    return 0;  }    // load the file  
infile = fopen(argv[1], "r");  if(!infile){    printf("Error opening file %s\n", argv[1]);    return -1;  }    GetID3Headers(infile, 0);    // all done    
fclose(infile);  return 0;}
