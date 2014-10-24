#ifndef __FAKETOOTH_AUDIO_H__
#define __FAKETOOTH_AUDIO_H__

#ifdef __cplusplus
    extern "C" {
#endif

int faketoothInit(void);        
int faketoothEnable(void);
int faketoothDo(void);
int faketoothDisable(void);

#ifdef __cplusplus
    }
#endif

#endif
