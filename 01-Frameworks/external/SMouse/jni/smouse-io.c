#include <fcntl.h>

#include "smouse-io.h"

#define TAG "SMouse"
#define PATH "/dev/smouse"

int openDevice(void)
{
	return open(PATH, O_RDWR | O_NONBLOCK);
}

int closeDevice(int fd)
{
	return close(fd);
}

int writeValues(int fd, int button, int Vscroll, int deltaX, int deltaY)
{
	struct smouse buf = { button, Vscroll, deltaX, deltaY };
	return write(fd, &buf, sizeof(buf));
}
