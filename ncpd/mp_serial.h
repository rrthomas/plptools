#ifndef _mp_serial_h
#define _mp_serial_h

int init_serial(const char *dev, int speed, int debug);
void ser_exit(int fd);

#endif
