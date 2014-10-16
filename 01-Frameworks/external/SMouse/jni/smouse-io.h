#ifndef _SMOUSE_IO_H_
#define _SMOUSE_IO_H_

struct smouse {
	int button;
	int Vscroll;
	int deltaX;
	int deltaY;
};

int openDevice(void);
int closeDevice(int);
int writeValues(int, int, int, int, int);

#endif
