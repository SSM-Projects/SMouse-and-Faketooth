#include <fcntl.h>
#include <media/AudioTrack.h>
#include "faketooth-audio.h"

#define BUF_SIZE 8192
#define DEV_PATH "/data/g.wav"

namespace android {

int fd;
AudioTrack *gat;

/********************************************************************** AudioTrack Control */

AudioTrack* at_create()
{
    AudioTrack *at = new AudioTrack();
    if (at == NULL) {
        return NULL;
    }
    return at;
}

int at_set(AudioTrack* at, int streamType, uint32_t sampleRate, int channelCount, int bufSize)
{
    status_t status = NO_ERROR;
    audio_format_t format = AUDIO_FORMAT_PCM_16_BIT;

    if (at == NULL)
        return -1;

    status = at->set((audio_stream_type_t)streamType, sampleRate, format, channelCount, bufSize);
    if (status != NO_ERROR)
        return -1;

    return 0;
}

ssize_t at_write(AudioTrack* at, const void *buf, size_t size)
{
    if (at == NULL)
        return -1;

    return at->write(buf, size);
}

void at_pause(AudioTrack* at)
{
    if (at != NULL)
        at->pause();
}

void at_start(AudioTrack* at)
{
    if (at != NULL)
        at->start();
}

void at_stop(AudioTrack* at)
{
    if (at != NULL)
        at->stop();
}

void at_flush(AudioTrack* at)
{
    if (at != NULL)
        at->flush();
}

void at_release(AudioTrack* at)
{
    if (at != NULL)
        at = NULL;
}

/********************************************************************** Faketotoh Control */

void removeWavHeader()
{
    char buf[44] = {0,};
    at_pause(gat);
    read(fd, buf, sizeof(buf));
}

int _faketoothEnable()
{
    int ret;
    long flag;
    AudioTrack* at;

    fd = open(DEV_PATH, O_RDONLY);
    if (fd < 0) {
        return 1;
    }

    flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
    
    at = at_create();
    if (at == NULL) {
        close(fd);
        return 2;
    }

    ret = at_set(at, 10/* AUDIO_STREAM_FAKETOOTH */, 88200, 2, BUF_SIZE);
    if (ret == -1) {
        close(fd);
        at_release(at);
        return 3;
    }

    gat = at;

    removeWavHeader();      // TODO Remove after

    at_start(at);

    return 0;
}

int _faketoothDo()
{
    int len, ret;
    char buf[BUF_SIZE];
    AudioTrack* at = gat;

    len = read(fd, buf, sizeof(buf));

    if (len < 0) {          // TODO Modify after
        return 4;

    } else if (len == 0) {  // TODO Modify after
        return 5;

    } else {
        ret = at_write(at, buf, len);
        if (ret <= 0) {
            return 6;
        }
    }

    return 0;
}

int _faketoothDisable()
{
    if (fd > 0) {
        close(fd);
        fd = 0;
    }
    if (gat != NULL) {
        at_stop(gat);
        at_release(gat);
    }
    return 0;
}

}

/********************************************************************** Bridge */

int faketoothEnable()
{
    return android::_faketoothEnable();
}

int faketoothDo()
{
    return android::_faketoothDo();
}

int faketoothDisable()
{
    return android::_faketoothDisable();
}

