#include "app.h"
#include "event.h"
volatile sig_atomic_t app_running=1;
static void stop_app(int sig) { (void)sig; app_running=0; }
int main(void)
{
    struct sigaction action={0};
    action.sa_handler=stop_app;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT,&action,NULL); sigaction(SIGTERM,&action,NULL);
    if(init_lcd()<0)return EXIT_FAILURE;
    if(init_event()<0) { uinit_lcd(); return EXIT_FAILURE; }
    app_menu();
    close_event(); uinit_lcd(); return EXIT_SUCCESS;
}
