#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char * argv[], char * env[]){ 
    int i,j,t; 
    int length=0; 
    char line[500],*key,*value,*buffer; 

    printf("Processo: ho questo environment:\n"); 
    for(i=0;env[i];i++){ 
        key = env[i]; 
        for(j=0;env[i][j]!='=';j++); 
        env[i][j]=0; 
        value = env[i]+j+1; 
        if(!strcmp(key,"CONTENT_LENGTH")) 
            length = atoi(value); 
    } 
    fflush(stdout); 

    buffer=malloc(length); 

    for(i=0;i<length && (t=read(0,buffer+i,length-i));i+=t) ; 
    printf("Body received\n"); 

    for(i=0;i<length && (t=write(1,buffer+i,length-i));i+=t) ; 

    for(i=0;env[i];i++) 
        printf("%s---> %s\n",env[i],env[i]+strlen(env[i])+1); 

    return 0;
}
