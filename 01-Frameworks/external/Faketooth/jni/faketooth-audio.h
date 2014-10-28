#ifndef __FAKETOOTH_AUDIO_H__
#define __FAKETOOTH_AUDIO_H__

#ifdef __cplusplus
    extern "C" {
#endif

int faketoothA2DPInit(void);        
int faketoothA2DPEnable(void);
int faketoothA2DPDo(void);
int faketoothA2DPDisable(void);

int faketoothSCOInit(void);        
int faketoothSCOEnable(void);
int faketoothSCODo(void);
int faketoothSCODisable(void);

int faketoothSCOMicInit(void);        
int faketoothSCOMicEnable(void);
int faketoothSCOMicDo(void);
int faketoothSCOMicDisable(void);

#ifdef __cplusplus
    }
#endif

#endif
