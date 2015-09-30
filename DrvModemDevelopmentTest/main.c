/* 
 * File:   main.c
 * Author: ingeniero02
 *
 * Created on 30 de septiembre de 2015, 09:11 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>

#include "DrvModem.h"

timer_start(int signo, void (*handler)(int), double period){
    timer_t fd;
    struct sigevent sigconf;
    struct itimerspec tmrtime;
    sigconf.sigev_notify = SIGEV_SIGNAL;
    sigconf.sigev_signo = signo;
    tmrtime.it_interval.tv_sec = (int)period;
    tmrtime.it_interval.tv_nsec = (period-tmrtime.it_interval.tv_sec)*1E9;
    tmrtime.it_value = tmrtime.it_interval;
    signal(signo, handler);
    timer_create(CLOCK_REALTIME, &sigconf, &fd);
    timer_settime(fd, 0 , &tmrtime, NULL);
}

void tmr_sim(int id){
    CellularModemISR_1msTimeOutHandler();
}

void modem_handler_task(int id){
    if (!CellularModem_TurnOn()) return; //if operation unsuccess, return and retry next time task gets triggered
    if (!CellularModem_StartSetup(3)) return; //if operation unsuccess, return an try next time (If retries > 3, release previous flags)
    if (!CellularModem_ConnectToCellularNetwork(4)) return; //if operation unsuccess, return an try next time (If retries > 4, release previous flags)
}

void putcmodem(char c){ putc(c, stdout);}
void putcdegub(char c){ putc(c, stdout);}
void modem_pwrctrl(unsigned char x) {puts("PWR Pin Maniputaed");}
void modem_rstctrl(unsigned char x) {puts("RST Pin Maniputaed");}

int main(void) {
    pthread_t th;
    timer_start(SIGALRM, tmr_sim, 0.001);
    
    CellularModem_InitDrv(putcmodem, putcdegub, modem_pwrctrl, modem_rstctrl);
    CellularModem_AddOperator( "movistar", "internet.movistar.com.co", "movistar", "movistar");
    CellularModem_AddOperator( "claro", "internet.comcel.com.co", "COMCELWEB", "COMCELWEB");
    CellularModem_AddOperator( "uff", "web.uffmovil.com.co", "", "");
    CellularModem_AddOperator( "tigo", "web.colombiamovil.com.co", "", "");
    CellularModem_AddOperator( "virgin mobile", "web.vmc.net.co", "", "");
    timer_start(SIGUSR1, modem_handler_task, 0.2); 
    for(;;){
        pause();
    }
    
    return (EXIT_SUCCESS);
}

