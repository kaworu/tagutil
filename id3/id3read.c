#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "id3read.h"
int GetID3Headers(FILE* infile, int testfail){  id31* id1;  id32* id2;  int result;  result = GetID3HeadersFull(infile, testfail, &id1, &id2);  if(id1)    ID31Free(id1);  if(id2)    ID32Free(id2);  return result;   }int GetID3HeadersFull(FILE* infile, int testfail, id31** id31save, id32** id32save){  int result;  char* input;  id31* id3header;  id32* id32header;  int fail=0;  // seek to start of header
result = fseek(infile, 0-sizeof(id31), SEEK_END);  if(result){    printf("Error seeking to header\n");    return -1;  }    // read in to buffer
input = malloc(sizeof(id31));  result=fread(input, 1, sizeof(id31), infile);  if(result!=sizeof(id31)){    printf("Read fail: expected %d bytes but got %d\n", sizeof(id31), result);    return -1;  }    // now call id3print
id3header=ID31Detect(input);  // do we win?
if(id3header){    if(!testfail){      printf("Valid ID3v1 header detected\n");      ID31Print(id3header);    }  }  else{    if(!testfail)      printf("ID3v1 header not found\n");    else      fail=1;  }  id32header=ID32Detect(infile);  if(id32header){    if(!testfail){      printf("Valid ID3v2 header detected\n");      ID32Print(id32header);    }  }  else{    if(!testfail)      printf("ID3v2 header not found\n");    else      fail+=2;  }  *id31save=id3header;  *id32save=id32header;  return fail;}id32* ID32Detect(FILE* infile){  unsigned char* buffer;  int result;  int i=0;  id32* id32header;  int filepos=0;  int size=0;  id32frame* lastframe=NULL;  id322frame* lastframev2=NULL;  // seek to start
fseek(infile, 0, SEEK_SET);  // read in first 10 bytes  
buffer = calloc(1, 11);  id32header=calloc(1, sizeof(id32));  result = fread(buffer, 1, 10, infile);  filepos+=result;  // make sure we have 10 bytes  
if(result != 10){    printf("ID3v2 read failed, expected 10 bytes, read only %d\n", result);    return (NULL);  }  // do the comparison  
/*  An ID3v2 tag can be detected with the following pattern:  $49 44 33 yy yy xx zz zz zz zz   Where yy is less than $FF, xx is the 'flags' byte and zz is less than $80.   */  result=1;  for(i=6; i<10; i++)    if(buffer[i]>=0x80) result = 0;  if(buffer[0]==0x49 && buffer[1]==0x44 && buffer[2]==0x33 && buffer[3]<0xff && buffer[4]<0xff && result){    // its ID3v2 :D    
for(i=6; i<10; i++){      size=size<<7;      size+=buffer[i];    }    //printf("Header size: %d\n", size);
}  else{    // cleanup    
free(buffer);    free(id32header);    return NULL;  }  // set up header
id32header->version[0]=buffer[3];  id32header->version[1]=buffer[4];  id32header->flags=buffer[5];  id32header->size=size;  id32header->firstframe=NULL;    // done with buffer - be sure to free... 
free(buffer);    // test for flags - die on all for the time being    
if(id32header->flags){    //printf("Flags set, can't handle :(\n");
return NULL;  }  // make sure its version 3 
if(id32header->version[0]!=3){    //printf("Want version 3, have version %d\n", id32header->version[0]);    //return NULL;    
// this code is modified from version 3, ideally we should reuse some
while(filepos-10<id32header->size){      // make space for new frame      
int i;      id322frame* frame = calloc(1, sizeof(id322frame));      frame->next=NULL;      // populate from file      
result=fread(frame, 1, 6, infile);      if(result != 6){        printf("Expected to read 6 bytes, only got %d, from point %d in the file\n", result, filepos);        // not freeing this time, but we should deconstruct in future     
return NULL;      }      // make sure we haven't got a blank tag      
if(frame->ID[0]==0){        break;      }      // update file cursor      
filepos+=result;      // convert size to little endian      
frame->size=0;      for(i=0; i<3; i++){        frame->size = frame->size<<8;        frame->size += frame->sizebytes[i];      }      // debug      
buffer=calloc(1,4);      memcpy(buffer, frame->ID, 3);      //printf("Processing frame %s, size %d filepos %d\n", buffer, frame->size, filepos);      
free(buffer);      // read in the data      
frame->data = calloc(1, frame->size);      result = fread(frame->data, 1, frame->size, infile);      filepos+=result;      if(result != frame->size){        printf("Expected to read %d bytes, only got %d\n", frame->size, result);        return NULL;      }      // add to end of queue      
if(id32header->firstframe==NULL){        id32header->firstframe=(id32frame*)frame;      }      else{        lastframev2->next=frame;      }      lastframev2=frame;      // then loop until we've filled the struct    
}  }  else{    // start reading frames    
while(filepos-10<id32header->size){      // make space for new frame      
int i;      id32frame* frame = calloc(1, sizeof(id32frame));      frame->next=NULL;      // populate from file      
result=fread(frame, 1, 10, infile);      if(result != 10){        printf("Expected to read 10 bytes, only got %d, from point %d in the file\n", result, filepos);        // not freeing this time, but we should deconstruct in future        
return NULL;      }      // make sure we haven't got a blank tag      
if(frame->ID[0]==0){        break;      }      // update file cursor      
filepos+=result;      // convert size to little endian      
frame->size=0;      for(i=0; i<4; i++){        frame->size = frame->size<<8;        frame->size += frame->sizebytes[i];      }      // debug      
buffer=calloc(1,5);      memcpy(buffer, frame->ID, 4);      //printf("Processing frame %s, size %d filepos %d\n", buffer, frame->size, filepos);      
free(buffer);      // read in the data      
frame->data = calloc(1, frame->size);      result = fread(frame->data, 1, frame->size, infile);      filepos+=result;      if(result != frame->size){        printf("Expected to read %d bytes, only got %d\n", frame->size, result);        return NULL;      }      // add to end of queue      
if(id32header->firstframe==NULL){        id32header->firstframe=frame;      }      else{        lastframe->next=frame;      }      lastframe=frame;      // then loop until we've filled the struct    
}  }    return id32header;}void ID32Print(id32* id32header){  id32frame* thisframe;  int ver=id32header->version[0];  // loop through tags and process  
thisframe=id32header->firstframe;  while(thisframe!=NULL){    char* buffer;    if(id32header->version[0]==3){      buffer=calloc(5,1);      memcpy(buffer, thisframe->ID, 4);      if(!strcmp("TIT2", buffer) || !strcmp("TT2", buffer)){        //printf("TIT2 tag detected\n");        
printf("Title: %s\n", thisframe->data+1);      }      else if(!strcmp("TALB", buffer) || !strcmp("TAL", buffer)){        
//printf("TALB tag detected\n");        
printf("Album: %s\n", thisframe->data+1);      }      else if(!strcmp("TPE1", buffer) || !strcmp("TP1", buffer)){        //printf("TPE1 tag detected\n");        
printf("Artist: %s\n", thisframe->data+1);      }    }    else if(id32header->version[0]==2){      id322frame* tframe=(id322frame*) thisframe;      buffer=calloc(4,1);      memcpy(buffer, thisframe->ID, 3);      if(!strcmp("TIT2", buffer) || !strcmp("TT2", buffer)){        //printf("TIT2 tag detected\n");        
printf("Title: %s\n", tframe->data+1);      }      else if(!strcmp("TALB", buffer) || !strcmp("TAL", buffer)){        //printf("TALB tag detected\n");        
printf("Album: %s\n", tframe->data+1);      }      else if(!strcmp("TPE1", buffer) || !strcmp("TP1", buffer)){        //printf("TPE1 tag detected\n");        
printf("Artist: %s\n", tframe->data+1);      }    }    free(buffer);    thisframe=(ver==3)?thisframe->next:(id32frame*)((id322frame*)thisframe)->next;  }}void ID32Free(id32* id32header){  if(id32header->version[0]==3){    id32frame* bonar=id32header->firstframe;    while(bonar!=NULL){      id32frame* next = bonar->next;      free(bonar->data);      free(bonar);      bonar=next;    }  }  else if(id32header->version[0]==2){    id322frame* bonar=(id322frame*)id32header->firstframe;    while(bonar!=NULL){      id322frame* next = bonar->next;      free(bonar->data);      free(bonar);      bonar=next;    }  }  free(id32header);}id32flat* ID32Create(){  id32flat* gary = calloc(1, sizeof(id32flat));  // allocate 10 bytes for the main header  
gary->buffer=calloc(1, 10);  gary->size = 10;  return gary;}void ID32AddTag(id32flat* gary, char* ID, char* data, char* flags, int size){  // resize the buffer  
int i;  int killsize;  //printf("Resizing the buffer\n");  
gary->buffer = realloc(gary->buffer, gary->size+size+10);  // copy header in  
//printf("Copying the ID\n");  
for(i = 0; i<4; i++){    gary->buffer[gary->size+i]=ID[i];  }  killsize=size;  // convert to sizebytes - endian independent  
//printf("Copying size\n");  
for(i=7; i > 3; i--){    gary->buffer[gary->size+i]=killsize & 0xff;    killsize = killsize>>8;  }  // copy flags  
//printf("Setting flags\n");  
gary->buffer[gary->size+8]=flags[0];  gary->buffer[gary->size+9]=flags[1];  // then the data comes through  
memcpy(gary->buffer+gary->size+10, data, size);  // then update size  
gary->size+= size + 10;  // done :D
}void ID32Finalise(id32flat* gary){  int killsize;  int i;  // set tag  
gary->buffer[0]='I';  gary->buffer[1]='D';  gary->buffer[2]='3';  // set version  
gary->buffer[3]=3;  gary->buffer[4]=0;  // set flags  
gary->buffer[5]=0;  // get size  
killsize = gary->size;  // convert to bytes for header  
for(i=9; i > 5; i--){    gary->buffer[i]=killsize & 0x7f;    killsize = killsize>>7;  }  // done :D
}id32flat* ID3Copy1to2(id31* bonar){  // todo: get rid of spaces on the end of padded ID3v1 :/  
printf("Creating new ID3v2 header\n");  id32flat* final=ID32Create();  char* buffer;  char flags[2];  flags[0]=0;  flags[1]=0;  // copy title  
buffer = calloc(1, 31);  memcpy(buffer+1, bonar->title, 30);  printf("Adding title\n");  ID32AddTag(final, "TIT2", buffer, flags, strlen(buffer+1)+1);  free(buffer);  // copy artist  
buffer = calloc(1, 31);  memcpy(buffer+1, bonar->artist, 30);  printf("Adding artist\n");  ID32AddTag(final, "TALB", buffer, flags, strlen(buffer+1)+1);  free(buffer);  // copy album  
buffer = calloc(1, 31);  memcpy(buffer+1, bonar->album, 30);  printf("Adding album\n");  ID32AddTag(final, "TPE1", buffer, flags, strlen(buffer+1)+1);  free(buffer);  // then finalise  
printf("Finalising...\n");  ID32Finalise(final);  printf("done :D\n");  // all done :D  
return final;}id31* ID31Detect(char* header){  char test[4];  id31* toreturn = calloc(sizeof(id31), 1);  memcpy(test, header, 3);  // make sure TAG is present  
if(strcmp("TAG", test)){    // successrar    
memcpy(toreturn, header, sizeof(id31));    return toreturn;  }  // otherwise fail  
return NULL;}void ID31Print(id31* id31header){  char* buffer = calloc(1, 31);  memcpy(buffer, id31header->title, 30);  printf("Title: %s\n", buffer);  memcpy(buffer, id31header->artist, 30);  printf("Artist: %s\n", buffer);  memcpy(buffer, id31header->album, 30);  printf("Album: %s\n", buffer);  free(buffer);}void ID31Free(id31* id31header){  free(id31header);}
