#ifndef _GETSERIALPORT_H_
#define _GETSERIALPORT_H_
extern  void *uart_thread(void *arg);
void set_speed(int fd, int speed);
int set_parity(int fd,int databits,int stopbits,int parity);
#endif