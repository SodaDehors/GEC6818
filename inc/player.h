#ifndef PLAYER_H
#define PLAYER_H
#include <sys/types.h>
typedef struct {
    pid_t pid;
    int input,output,paused,volume,ended,failed;
    double position,duration;
    long long queried;
    char line[2048]; size_t used;
} Player;
long long monotonic_ms(void);
void player_init(Player *p);
int player_start(Player *p,const char *path,int video);
void player_stop(Player *p);
int player_pause(Player *p);
int player_volume(Player *p,int value);
int player_seek(Player *p,int percent);
void player_update(Player *p);
#endif
