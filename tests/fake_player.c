#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
int main(int argc,char **argv)
{
    char line[256]; int paused=0;
    const char *expected=getenv("TEST_MEDIA_PATH");
    if(expected && (argc<2 || strcmp(argv[argc-1],expected)))return 23;
    setbuf(stdout,NULL);
    if(getenv("TEST_PLAYER_FAIL"))return 17;
    while(fgets(line,sizeof(line),stdin)) {
        if(!strcmp(line,"pause\n"))paused=!paused;
        else if(!strcmp(line,"get_time_pos\n")) {
            /* Paused queries are a regression for the course's older MPlayer. */
            if(paused)return 24;
            fputs("ANS_TIME_",stdout); usleep(2000); fputs("POSITION=12.5\n",stdout);
        } else if(!strcmp(line,"get_time_length\n"))fputs("ANS_LENGTH=100.0\n",stdout);
        else if(!strncmp(line,"pausing_keep seek ",18)) {
            if(strncmp(line,"pausing_keep seek 75 1",21))return 25;
        } else if(!strncmp(line,"pausing_keep volume ",20)) {
            if(strncmp(line,"pausing_keep volume 100 1",24))return 26;
        } else if(!strcmp(line,"quit\n"))return 0;
    }
    return 0;
}
