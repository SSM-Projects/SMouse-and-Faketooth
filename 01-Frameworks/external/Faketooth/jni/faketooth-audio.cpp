#include <poll.h>
#include <fcntl.h>
#include <android/log.h>
#include <media/AudioTrack.h>
#include <media/AudioRecord.h>
#include "faketooth-audio.h"

#define TAG "FaketoothAudioJni"

#define A2DP_BUFFER_SIZE  512
#define A2DP_SAMPLE_RATE  44100
#define A2DP_STREAM_TYPE  AUDIO_STREAM_FAKETOOTH
#define A2DP_CHANNEL_MASK AUDIO_CHANNEL_OUT_STEREO
#define A2DP_DEVICE_PATH  "/dev/faketooth_A2DP_speaker"

#define SCO_BUFFER_SIZE  512
#define SCO_SAMPLE_RATE  8000
#define SCO_STREAM_TYPE  AUDIO_STREAM_FAKETOOTH
#define SCO_CHANNEL_MASK AUDIO_CHANNEL_OUT_MONO
#define SCO_DEVICE_PATH  "/dev/faketooth_SCO_speaker"

#define SCO_MIC_BUFFER_SIZE  512
#define SCO_MIC_SAMPLE_RATE  8000
#define SCO_MIC_INPUT_SOURCE AUDIO_SOURCE_FAKETOOTH
#define SCO_MIC_CHANNEL_MASK AUDIO_CHANNEL_IN_MONO
#define SCO_MIC_DEVICE_PATH  "/dev/faketooth_SCO_mic"

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , TAG, __VA_ARGS__)

namespace android {

int fd;
int micfd;
AudioTrack *gat;
AudioRecord *gar;

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

/********************************************************************** AudioRecord Control */

AudioRecord* ar_create()
{
    AudioRecord *ar = new AudioRecord();
    if (ar == NULL)
        return NULL;

    return ar;
}

int ar_set(AudioRecord* ar, int inputSource, uint32_t sampleRate, uint32_t channelMask, int frameCount)
{
    status_t status = NO_ERROR;

    if (ar == NULL)
        return -1;

    status = ar->set(
            (audio_source_t) inputSource,
            sampleRate,
            (audio_format_t) AUDIO_FORMAT_PCM_16_BIT,
            (audio_channel_mask_t) channelMask,
            frameCount);

    if (status != NO_ERROR)
        return -1;

    return 0;
}

int ar_getMinFrameCount(AudioRecord* ar, size_t *frameCount, uint32_t sampleRate, uint32_t channelMask)
{
    status_t status = NO_ERROR;

    if (ar == NULL)
        return -1;

    status = ar->getMinFrameCount(
            frameCount,
            sampleRate,
            (audio_format_t) AUDIO_FORMAT_PCM_16_BIT,
            (audio_channel_mask_t) channelMask);

    if (status != NO_ERROR)
        return -1;

    return 0;
}

ssize_t ar_read(AudioRecord* ar, void *buf, size_t size)
{
    if (ar == NULL)
        return -1;

    return ar->read(buf, size);
}

void ar_start(AudioRecord* ar)
{
    if (ar != NULL)
        ar->start((AudioSystem::sync_event_t) 0 /* SYNC_EVENT_NONE */);
}

void ar_stop(AudioRecord* ar)
{
    if (ar != NULL)
        ar->stop();
}

/********************************************************************** Faketooth A2DP Control */

int _faketoothA2DPInit()
{
    long flag;

    fd = open(A2DP_DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        LOGE("[FAKETOOTH] %s open() error", A2DP_DEVICE_PATH);
        return -1;
    }

    flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);

    return 0;
}

int _faketoothA2DPEnable()
{
    int ret;
    AudioTrack *at;

    at = at_create();
    if (at == NULL) {
        LOGE("[FAKETOOTH] at_create() error");
        return -1;
    }

    ret = at_set(at, A2DP_STREAM_TYPE, A2DP_SAMPLE_RATE, A2DP_CHANNEL_MASK, A2DP_BUFFER_SIZE);
    if (ret == -1) {
        LOGE("[FAKETOOTH] at_set() error");
        return -2;
    }

    gat = at;

    at_start(at);

    return 0;
}

int _faketoothA2DPDo()
{
    AudioTrack* at = gat;

    static int cnt = 0;

    int ret, len = 0;
    char buf[A2DP_BUFFER_SIZE];
    struct pollfd pfd[1] = { {fd, POLLIN, 0} };

    ret = poll(pfd, 1, 5);
    if (pfd[0].revents & POLLIN) {
        len = read(fd, buf, A2DP_BUFFER_SIZE);
        if (len < 0) {
            LOGE("[FAKETOOTH] read() error %d", len);
            return -1;
        }
        if (len == 1 && cnt != 0) {
            cnt = 0;
            return 0;
        }
        if (len > 1 && ++cnt > 10) {
            ret = at_write(at, buf, len);
            if (ret < 0) {
                LOGE("[FAKETOOTH] at_write() error");
                return -2;
            }
        }
    }

    return 0;
}

int _faketoothA2DPDisable()
{
    AudioTrack* at = gat;

    if (fd > 0) {
        close(fd);
        fd = -1;
    }

    if (at != NULL) {
        at_stop(at);
        at = NULL;
        gat = NULL;
    }

    return 0;
}

/********************************************************************** Faketooth SCO Control */

int _faketoothSCOInit()
{
    long flag;

    fd = open(SCO_DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        LOGE("[FAKETOOTH] %s open() error", SCO_DEVICE_PATH);
        return -1;
    }

    flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);

    return 0;
}

int _faketoothSCOEnable()
{
    int ret;
    AudioTrack *at;

    at = at_create();
    if (at == NULL) {
        LOGE("[FAKETOOTH] at_create() error");
        return -1;
    }

    ret = at_set(at, SCO_STREAM_TYPE, SCO_SAMPLE_RATE, SCO_CHANNEL_MASK, SCO_BUFFER_SIZE);
    if (ret == -1) {
        LOGE("[FAKETOOTH] at_set() error");
        return -2;
    }

    gat = at;

    at_start(at);

    return 0;
}

int _faketoothSCODo()
{
    AudioTrack* at = gat;

    static int cnt = 0;

    int ret, len = 0;
    char buf[SCO_BUFFER_SIZE];
    struct pollfd pfd[1] = { {fd, POLLIN, 0} };

    ret = poll(pfd, 1, 5);
    if (pfd[0].revents & POLLIN) {
        len = read(fd, buf, SCO_BUFFER_SIZE);
        if (len < 0) {
            LOGE("[FAKETOOTH] read() error %d", len);
            return -1;
        }
        if (len == 1 && cnt != 0) {
            cnt = 0;
            return 0;
        }
        if (len > 1 && ++cnt > 10) {
            ret = at_write(at, buf, len);
            if (ret < 0) {
                LOGE("[FAKETOOTH] at_write() error");
                return -2;
            }
        }
    }

    return 0;
}

int _faketoothSCODisable()
{
    AudioTrack* at = gat;

    if (fd > 0) {
        close(fd);
        fd = -1;
    }

    if (at != NULL) {
        at_stop(at);
        at = NULL;
        gat = NULL;
    }

    return 0;
}

/********************************************************************** Faketooth SCO Mic Control */

int _faketoothSCOMicInit()
{
    micfd = open(SCO_MIC_DEVICE_PATH, O_WRONLY);
    if (micfd < 0) {
        LOGE("[FAKETOOTH] %s open() error", SCO_MIC_DEVICE_PATH);
        return -1;
    }

    return 0;
}

int _faketoothSCOMicEnable()
{
    int ret;
    size_t minFrameCount;
    AudioRecord *ar;

    ar = ar_create();
    if (ar == NULL) {
        LOGE("[FAKETOOTH] ar_create() error");
        return -1;
    }

    ret = ar_getMinFrameCount(ar, &minFrameCount, SCO_MIC_SAMPLE_RATE, SCO_MIC_CHANNEL_MASK);
    if (ret == -1) {
        LOGE("[FAKETOOTH] ar_getMinFrameCount() error");
        return -2;
    }

    ret = ar_set(ar, SCO_MIC_INPUT_SOURCE, SCO_MIC_SAMPLE_RATE, SCO_MIC_CHANNEL_MASK, minFrameCount);
    if (ret == -1) {
        LOGE("[FAKETOOTH] ar_set() error");
        return -3;
    }

    gar = ar;

    ar_start(ar);

    return 0;
}

int _faketoothSCOMicDo()
{
    AudioRecord* ar = gar;

    int ret, len = 0;
    char buf[SCO_MIC_BUFFER_SIZE];

    len = ar_read(ar, buf, SCO_MIC_BUFFER_SIZE);
    if (len < 0) {
        LOGE("[FAKETOOTH] ar_read() error");
        return -1;
    }
    if (len > 0) {
        ret = write(micfd, buf, len);
        if (ret < 0) {
            LOGE("[FAKETOOTH] write() error %d", ret);
            return -2;
        }
    }

    return 0;
}

int _faketoothSCOMicDisable()
{
    AudioRecord* ar = gar;

    if (micfd > 0) {
        close(micfd);
        micfd = -1;
    }

    if (ar != NULL) {
        ar_stop(ar);
        ar = NULL;
        gar = NULL;
    }

    return 0;
}

}

/********************************************************************** Bridge */

int faketoothA2DPInit()
{
    return android::_faketoothA2DPInit();
}

int faketoothA2DPEnable()
{
    return android::_faketoothA2DPEnable();
}

int faketoothA2DPDo()
{
    return android::_faketoothA2DPDo();
}

int faketoothA2DPDisable()
{
    return android::_faketoothA2DPDisable();
}

int faketoothSCOInit()
{
    return android::_faketoothSCOInit();
}

int faketoothSCOEnable()
{
    return android::_faketoothSCOEnable();
}

int faketoothSCODo()
{
    return android::_faketoothSCODo();
}

int faketoothSCODisable()
{
    return android::_faketoothSCODisable();
}

int faketoothSCOMicInit()
{
    return android::_faketoothSCOMicInit();
}

int faketoothSCOMicEnable()
{
    return android::_faketoothSCOMicEnable();
}

int faketoothSCOMicDo()
{
    return android::_faketoothSCOMicDo();
}

int faketoothSCOMicDisable()
{
    return android::_faketoothSCOMicDisable();
}
