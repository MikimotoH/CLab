#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char** argv){
#define MAX_VOLUME_NAME (64)

#define __STRINGIFY__(x) #x
#define STRINGIFY(x) __STRINGIFY__(x)   // unable to remove parantheses 
#define __STRINGIFYX__   STRINGIFY 
#define STRINGIFYX(x) __STRINGIFYX__ x  //remove parantheses

    char pi_vid[]="Pi-Coral";
    char volume_name[MAX_VOLUME_NAME]="volume01";
    char vid[8+1+MAX_VOLUME_NAME+1];
    memset(vid, ' ', sizeof(vid));
    vid[sizeof(vid)-1] = 0;
    int k = sprintf(vid, "%.8s:%-.*s", pi_vid, MAX_VOLUME_NAME, volume_name );
    vid[k] = ' ';

    printf("vid=\"%s\"\n", vid);
    printf("vidlen=%d\n", (int)strlen(vid));
    return 0;
}
