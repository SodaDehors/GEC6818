#include "player.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
int main(void)
{
    Player p; int i,status; pid_t owned;
    const char *path="/tmp/a song 'quoted' $(must-not-run).mp3";
    setenv("MPLAYER_BIN","./build/tests/fake_player",1);
    setenv("TEST_MEDIA_PATH",path,1); player_init(&p);
    assert(player_start(&p,path,0)==0);
    for(i=0;i<100 && p.duration==0;i++) { player_update(&p); usleep(10000); }
    assert(p.pid>0 && p.duration==100.0 && p.position==12.5);
    assert(player_pause(&p)==0 && p.paused);
    for(i=0;i<70;i++) { player_update(&p); usleep(10000); }
    assert(p.pid>0 && !p.ended && p.position==12.5);
    assert(player_volume(&p,150)==0 && p.volume==100);
    assert(player_seek(&p,75)==0);
    assert(player_pause(&p)==0 && !p.paused);
    owned=p.pid; player_stop(&p);
    errno=0; assert(waitpid(owned,&status,WNOHANG)==-1 && errno==ECHILD);
    setenv("TEST_PLAYER_FAIL","1",1); assert(player_start(&p,path,1)==0);
    for(i=0;i<100 && !p.ended;i++) { player_update(&p); usleep(10000); }
    assert(p.ended && p.failed && p.pid==0); player_stop(&p);
    unsetenv("TEST_PLAYER_FAIL");
    setenv("MPLAYER_BIN","/not/a/player",1); assert(player_start(&p,path,0)==0);
    for(i=0;i<100 && !p.ended;i++) { player_update(&p); usleep(10000); }
    assert(p.ended && p.failed); player_stop(&p);
    puts("PASS player feedback/pause/seek/volume/argv/failure/process cleanup"); return 0;
}
