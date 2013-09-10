#include <stdio.h>
#include <stdlib.h>
#include "id3read.h"
int main(int argc, char* argv[]){  FILE* infile;  int result;  id31* id1;  id32* id2;  // make sure files are present
	if(argc<2){    printf("Usage: id3fix filename.mp3\n");    return -1;  }  infile = fopen(argv[1], "r");  if(!infile){    printf("Error opening file %s\n", argv[1]);    return -1;  }    printf("Fixing file %s\n", argv[1]);  result = GetID3HeadersFull(infile, 1, &id1, &id2);  if(result & 1){    printf("No ID3v1 tag present\n");  }  if(result & 2){    printf("No ID3v2 tag present\n");  }  if(!result){    printf("File has both ID3v1 and ID3v2 tags\n");  }    if(result == 2){    char c;    printf("Do you want to copy ID3v2 info from the ID3v1 tag?");    c = getchar();    if(c=='y' || c=='Y'){      id32flat* gary = ID3Copy1to2(id1);      char* mp3;      unsigned long size;      printf("wee\n");      // then read in, and write to file :D
		fseek(infile, 0, SEEK_END);      size=ftell(infile);      printf("File size: %ld\n", size);      // this can only read in 2gb of file
		fseek(infile, 0, SEEK_SET);      mp3 = calloc(1, size);      fread(mp3, 1, size, infile);      // then close, reopen for reading
		fclose(infile);      infile = fopen(argv[1], "w");      //infile=NULL;
		if(!infile){        printf("Can't open file for writing :(\n");        return -1;      }      fwrite(gary->buffer, 1, gary->size, infile);      fwrite(mp3, 1, size, infile);      fclose(infile);      printf("done\n");            return 0;    }  }  // do our own ID3 tags from scratch;
	return 0;}
