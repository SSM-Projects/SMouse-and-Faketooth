#include <fcntl.h>

#include "smouse-io.h"

#define TAG	"SMouse"
#define PATH	"/dev/android_mouse"

int openDevice(void)
{
	return open(PATH, O_RDWR | O_NONBLOCK);
}

int closeDevice(int fd)
{
	return close(fd);
}

int writeValues(int fd, int btn, int wheel, int moveX, int moveY)
{
	struct smouse buf = { btn, wheel, moveX, moveY };
	return write(fd, &buf, sizeof(buf));
}
