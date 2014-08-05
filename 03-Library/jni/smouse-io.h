#ifndef _SMOUSE_IO_H_
#define _SMOUSE_IO_H_

struct smouse {
	int btn;
	int wheel;
	int moveX;
	int moveY;
};

int openDevice(void);
int closeDevice(int);
int writeValues(int, int, int, int, int);

#endif
