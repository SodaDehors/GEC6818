#include "event.h"
#include <linux/input.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
static int event_fd=-1,raw_x,raw_y,have_x,have_y,down,started;
static int x_code=ABS_X,y_code=ABS_Y;
static struct input_absinfo ax,ay;
static Touch pending;
static long long now_ms(void)
{
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts);
    return (long long)ts.tv_sec*1000+ts.tv_nsec/1000000;
}
static int scaled(int value,const struct input_absinfo *axis,int size)
{
    long long n=(long long)value-axis->minimum, span=(long long)axis->maximum-axis->minimum;
    if(n<0)n=0;
    if(n>span)n=span;
    return (int)(n*(size-1)/span);
}
int init_event(void)
{
    const char *path=getenv("TOUCH_DEVICE");
    event_fd=open(path ? path : "/dev/input/event0",O_RDONLY|O_NONBLOCK);
    if(event_fd<0) { perror("open touch"); return -1; }
    x_code=ABS_X; y_code=ABS_Y;
    if(ioctl(event_fd,EVIOCGABS(x_code),&ax)<0 || ioctl(event_fd,EVIOCGABS(y_code),&ay)<0) {
        x_code=ABS_MT_POSITION_X; y_code=ABS_MT_POSITION_Y;
        if(ioctl(event_fd,EVIOCGABS(x_code),&ax)<0 || ioctl(event_fd,EVIOCGABS(y_code),&ay)<0) {
            fprintf(stderr,"Touch axes unavailable\n"); close_event(); return -1;
        }
    }
    if(ax.maximum<=ax.minimum || ay.maximum<=ay.minimum) { close_event(); return -1; }
    raw_x=ax.value; raw_y=ay.value; have_x=have_y=1; down=started=0;
    return 0;
}
void close_event(void)
{
    if(event_fd>=0)close(event_fd);
    event_fd=-1; down=started=have_x=have_y=0;
}
int event_poll(Touch *touch,int timeout_ms)
{
    long long deadline=now_ms()+(timeout_ms<0 ? 0 : timeout_ms);
    if(event_fd<0 || !touch)return -1;
    for(;;) {
        struct pollfd p={event_fd,POLLIN,0}; struct input_event ev;
        long long remaining=deadline-now_ms();
        int wait=timeout_ms<0 ? -1 : remaining>0 ? (int)remaining : 0;
        int ret=poll(&p,1,wait); ssize_t count;
        if(ret<0 && errno==EINTR)continue;
        if(ret<0 || (p.revents&(POLLERR|POLLHUP|POLLNVAL)))return -1;
        if(ret==0)return 0;
        count=read(event_fd,&ev,sizeof(ev));
        if(count<0 && (errno==EINTR || errno==EAGAIN))continue;
        if(count!=(ssize_t)sizeof(ev))return -1;
        if(ev.type==EV_SYN && ev.code==SYN_DROPPED)down=started=have_x=have_y=0;
        if(ev.type==EV_ABS) {
            if(ev.code==x_code) { raw_x=ev.value; have_x=1; }
            if(ev.code==y_code) { raw_y=ev.value; have_y=1; }
            if(x_code==ABS_MT_POSITION_X && ev.code==ABS_MT_TRACKING_ID)down=ev.value>=0;
        }
        if(ev.type==EV_KEY && ev.code==BTN_TOUCH)down=ev.value!=0;
        /* Process a complete report, independent of coordinate/button event ordering. */
        if(ev.type==EV_SYN && ev.code==SYN_REPORT && have_x && have_y) {
            if(down && !started) {
                pending.x=scaled(raw_x,&ax,800); pending.y=scaled(raw_y,&ay,480); started=1;
            } else if(!down && started) {
                pending.end_x=scaled(raw_x,&ax,800); pending.end_y=scaled(raw_y,&ay,480);
                *touch=pending; started=0; return 1;
            }
        }
        if(timeout_ms>=0 && now_ms()>=deadline)return 0;
    }
}
int get_pos(int *x,int *y,int *ex,int *ey)
{
    Touch t;
    if(event_poll(&t,-1)!=1)return -1;
    *x=t.x; *y=t.y; *ex=t.end_x; *ey=t.end_y; return 0;
}
