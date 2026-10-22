#include <linux/input.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
static int channel[2];
static int mock_open(const char *path,int flags,...) { (void)path; (void)flags; return dup(channel[0]); }
static int mock_ioctl(int fd,unsigned long request,...)
{
    va_list ap; struct input_absinfo *axis; (void)fd;
    va_start(ap,request); axis=va_arg(ap,struct input_absinfo *); va_end(ap);
    memset(axis,0,sizeof(*axis));
    axis->maximum=request==EVIOCGABS(ABS_X) ? 1023 : 599; return 0;
}
#define open mock_open
#define ioctl mock_ioctl
#include "../src/event.c"
#undef open
#undef ioctl
static void send_event(unsigned short type,unsigned short code,int value)
{
    struct input_event e={0}; e.type=type; e.code=code; e.value=value;
    assert(write(channel[1],&e,sizeof(e))==(ssize_t)sizeof(e));
}
int main(void)
{
    Touch t;
    assert(pipe(channel)==0); assert(init_event()==0);
    send_event(EV_KEY,BTN_TOUCH,1); /* Key first, coordinates later in the same report. */
    send_event(EV_ABS,ABS_X,0); send_event(EV_ABS,ABS_Y,0); send_event(EV_SYN,SYN_REPORT,0);
    send_event(EV_ABS,ABS_X,1023); send_event(EV_ABS,ABS_Y,599);
    send_event(EV_KEY,BTN_TOUCH,0); send_event(EV_SYN,SYN_REPORT,0);
    assert(event_poll(&t,100)==1 && t.x==0 && t.y==0 && t.end_x==799 && t.end_y==479);
    /* Axes need not change on a second tap. */
    send_event(EV_KEY,BTN_TOUCH,1); send_event(EV_SYN,SYN_REPORT,0);
    send_event(EV_KEY,BTN_TOUCH,0); send_event(EV_SYN,SYN_REPORT,0);
    assert(event_poll(&t,100)==1 && t.x==799 && t.y==479);
    assert(event_poll(&t,1)==0);
    close_event(); close(channel[0]); close(channel[1]);
    assert(event_poll(&t,1)<0);
    puts("PASS touch report ordering/scaling/repeated taps/timeout/closed device"); return 0;
}
