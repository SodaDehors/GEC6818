#include "player.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
long long monotonic_ms(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return (long long)t.tv_sec*1000+t.tv_nsec/1000000;
}
void player_init(Player *p)
{
    memset(p,0,sizeof(*p)); p->input=p->output=-1; p->volume=60;
    signal(SIGPIPE,SIG_IGN);
}
static void close_pipes(Player *p)
{
    if(p->input>=0)close(p->input);
    if(p->output>=0)close(p->output);
    p->input=p->output=-1;
}
static int command(Player *p,const char *cmd)
{
    ssize_t n;
    if(p->pid<=0 || p->input<0)return -1;
    do { n=write(p->input,cmd,strlen(cmd)); } while(n<0 && errno==EINTR);
    return n==(ssize_t)strlen(cmd) ? 0 : -1;
}
void player_stop(Player *p)
{
    if(p->pid>0) {
        long long until=monotonic_ms()+400;
        int status; pid_t ret;
        command(p,"quit\n");
        do {
            ret=waitpid(p->pid,&status,WNOHANG);
            if(ret==p->pid || (ret<0 && errno==ECHILD))break;
            usleep(10000);
        } while(monotonic_ms()<until);
        if(ret==0) {
            kill(p->pid,SIGTERM);
            until=monotonic_ms()+200;
            do { ret=waitpid(p->pid,&status,WNOHANG); if(ret!=0)break; usleep(10000); }
            while(monotonic_ms()<until);
            if(ret==0) { kill(p->pid,SIGKILL); while(waitpid(p->pid,&status,0)<0 && errno==EINTR){} }
        }
    }
    close_pipes(p); p->pid=0; p->paused=0; p->position=0; p->used=0; p->ended=0;
}
int player_start(Player *p,const char *path,int video)
{
    int to[2],from[2]; char volume[16]; pid_t pid;
    const char *binary=getenv("MPLAYER_BIN");
    player_stop(p); p->failed=0; p->duration=0; p->queried=0;
    if(pipe(to)<0)return -1;
    if(pipe(from)<0) { close(to[0]); close(to[1]); return -1; }
    snprintf(volume,sizeof(volume),"%d",p->volume);
    pid=fork();
    if(pid==0) {
        const char *vo=getenv("MPLAYER_VO");
        close(to[1]); close(from[0]);
        if(dup2(to[0],STDIN_FILENO)<0 || dup2(from[1],STDOUT_FILENO)<0)_exit(126);
        close(to[0]); close(from[1]);
        /* argv prevents shell expansion, including quotes/spaces in filenames. */
        if(video)execlp(binary ? binary : "mplayer","mplayer","-slave","-quiet",
            "-nolirc","-nomouseinput","-osdlevel","0",
            "-softvol","-volume",volume,"-vo",vo ? vo : "fbdev2","-geometry","0:64",
            "-zoom","-x","800","-y","288","-vf","scale=800:288","--",path,(char *)NULL);
        else execlp(binary ? binary : "mplayer","mplayer","-slave","-quiet",
            "-nolirc","-nomouseinput","-softvol",
            "-volume",volume,"-novideo","--",path,(char *)NULL);
        perror("exec mplayer"); _exit(127);
    }
    close(to[0]); close(from[1]);
    if(pid<0) { close(to[1]); close(from[0]); return -1; }
    p->pid=pid; p->input=to[1]; p->output=from[0];
    fcntl(p->input,F_SETFL,fcntl(p->input,F_GETFL)|O_NONBLOCK);
    fcntl(p->output,F_SETFL,fcntl(p->output,F_GETFL)|O_NONBLOCK);
    /* Prevent unrelated subsequently spawned children from retaining these pipes. */
    fcntl(p->input,F_SETFD,FD_CLOEXEC); fcntl(p->output,F_SETFD,FD_CLOEXEC);
    return 0;
}
int player_pause(Player *p)
{
    if(command(p,"pause\n")<0)return -1;
    p->paused=!p->paused; return 0;
}
int player_volume(Player *p,int value)
{
    char cmd[80]; if(value<0)value=0; if(value>100)value=100;
    if(p->pid>0) {
        snprintf(cmd,sizeof(cmd),"pausing_keep volume %d 1\n",value);
        if(command(p,cmd)<0)return -1;
    }
    p->volume=value; return 0;
}
int player_seek(Player *p,int percent)
{
    char cmd[80]; if(percent<0)percent=0; if(percent>100)percent=100;
    snprintf(cmd,sizeof(cmd),"pausing_keep seek %d 1\n",percent);
    return command(p,cmd);
}
static void answer(Player *p)
{
    double v;
    if(sscanf(p->line,"ANS_TIME_POSITION=%lf",&v)==1 && isfinite(v) && v>=0 && v<1e9)p->position=v;
    if(sscanf(p->line,"ANS_LENGTH=%lf",&v)==1 && isfinite(v) && v>=0 && v<1e9)p->duration=v;
}
void player_update(Player *p)
{
    char data[1024]; ssize_t n; int status; pid_t ret;
    if(p->pid<=0)return;
    while((n=read(p->output,data,sizeof(data)))>0) {
        ssize_t i;
        for(i=0;i<n;i++) {
            if(data[i]=='\n' || data[i]=='\r') { p->line[p->used]=0; answer(p); p->used=0; }
            else if(p->used<sizeof(p->line)-1)p->line[p->used++]=data[i];
            else p->used=0;
        }
    }
    ret=waitpid(p->pid,&status,WNOHANG);
    if(ret==p->pid) {
        p->failed=!WIFEXITED(status) || WEXITSTATUS(status)!=0;
        p->ended=1; p->pid=0; p->paused=0; close_pipes(p); return;
    }
    if(!p->paused && monotonic_ms()-p->queried>=500) {
        /* Course MPlayer 1.0rc2 lacks pausing_keep_force. Freeze UI while paused
           rather than sending queries that would resume the old player. */
        command(p,"get_time_pos\nget_time_length\n");
        p->queried=monotonic_ms();
    }
}
