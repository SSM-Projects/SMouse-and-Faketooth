#include <fcntl.h>
#include <android/log.h>
#include <media/AudioTrack.h>
#include "faketooth-audio.h"

#define TAG "FaketoothAudioLib"

#define BUFFER_SIZE  8192 
#define SAMPLE_RATE  44100
#define STREAM_TYPE  AUDIO_STREAM_FAKETOOTH
#define CHANNEL_MASK AUDIO_CHANNEL_OUT_STEREO
#define DEVICE_PATH  "/dev/faketooth_speaker"

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , TAG, __VA_ARGS__)

namespace android {

int fd;
AudioTrack *gat;

/********************************************************************** AudioTrack Control */

AudioTrack* at_create()
{
    AudioTrack *at = new AudioTrack();
    if (at == NULL)
        return NULL;

    return at;
}

int at_set(AudioTrack* at, int streamType, uint32_t sampleRate, uint32_t channelMask, int frameCount)
{
    status_t status = NO_ERROR;

    if (at == NULL)
        return -1;

    status = at->set(
            (audio_stream_type_t) streamType,
            sampleRate,
            (audio_format_t) AUDIO_FORMAT_PCM_16_BIT,
            (audio_channel_mask_t) channelMask,
            frameCount);

    if (status != NO_ERROR)
        return -1;

    return 0;
}

int at_getMinFrameCount(AudioTrack* at, size_t *frameCount, int streamType, uint32_t sampleRate)
{
	status_t status = NO_ERROR;

	if (at == NULL)
		return -1;

	status = at->getMinFrameCount(
			frameCount,
			(audio_stream_type_t) streamType,
			sampleRate);

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

/********************************************************************** Faketooth Control */

int _faketoothEnable()
{
    int ret;
    long flag;
    size_t minFrameCount;
    AudioTrack *at;

    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        LOGE("[FAKETOOTH] open() error");
        return -1;
    }

    flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
    
    at = at_create();
    if (at == NULL) {
        LOGE("[FAKETOOTH] at_create() error");
        close(fd);
        return -1;
    }

    ret = at_getMinFrameCount(at, &minFrameCount, STREAM_TYPE, SAMPLE_RATE);
    if (ret == -1) {
        LOGE("[FAKETOOTH] at_getMinFrameCount() error");
        close(fd);
        at_release(at);
        return -1;
    }

    ret = at_set(at, STREAM_TYPE, SAMPLE_RATE, CHANNEL_MASK, BUFFER_SIZE);
    if (ret == -1) {
        LOGE("[FAKETOOTH] at_set() error");
        close(fd);
        at_release(at);
        return -1;
    }

    gat = at;

    at_start(at);

    return 0;
}

int _faketoothDo()
{
    AudioTrack* at = gat;

    int len, ret;
    char buf[BUFFER_SIZE];

    len = read(fd, buf, BUFFER_SIZE);
    if (len < 0) {
        LOGE("[FAKETOOTH] read() error");
        return -1;
    }
    if (len == 0) {
        LOGE("[FAKETOOTH] read() returned == 0");
        return -1;
    }

    ret = at_write(at, buf, len);
    if (ret < 0) {
        LOGE("[FAKETOOTH] at_write() error");
        return -1;
    }

    return 0;
}

int _faketoothDisable()
{
    AudioTrack* at = gat;

    if (fd > 0) {
        close(fd);
        fd = -1;
    }

    if (at != NULL) {
        at_stop(at);
        at_release(at);
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

