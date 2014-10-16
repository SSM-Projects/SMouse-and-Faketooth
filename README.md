# SMouse & Faketooth

SMouse Project, Jun-Jul, 2014.<br>
Faketooth Project, Sep-Oct, 2014.<br>
Android part. Junghyun Kim (kimjeongss@gmail.com)<br>
Windows part. Youngjun Lee (tgbnhy02@gmail.com)


#### Description

- To be updated


#### Working Demo

- To be updated


#### Target Environment

- Google Nexus 5
- Android 4.4.2 KitKat
- Android Linux Kernel 3.4.0
- Windows 7 x86/x64


#### License and Copyright Notices

- To be updated


#### Unsolved Issues

- To be updated


#### Added or Modified Android Sources List
##### 01-Frameworks/
```
+ external/SMouse/
+ external/Faketooth/

- packages/apps/Settings/res/values/strings.xml
- packages/apps/Settings/res/values/arrays.xml
- packages/apps/Settings/src/com/android/settings/bluetooth/BluetoothSettings.java
+ packages/apps/Settings/src/com/android/settings/bluetooth/BluetoothSelectHostFragment.java

+ frameworks/base/packages/SystemUI/res/drawable-hdpi/ic_qs_mouse_on.png
+ frameworks/base/packages/SystemUI/res/drawable-hdpi/ic_qs_mouse_off.png
- frameworks/base/packages/SystemUI/res/values/strings.xml
+ frameworks/base/packages/SystemUI/src/com/android/systemui/statusbar/policy/MouseController.java
- frameworks/base/packages/SystemUI/src/com/android/systemui/statusbar/phone/QuickSettings.java
- frameworks/base/packages/SystemUI/src/com/android/systemui/statusbar/phone/QuickSettingsModel.java
- frameworks/base/packages/SystemUI/src/com/android/systemui/statusbar/phone/PhoneStatusBar.java
- frameworks/base/packages/SystemUI/src/com/android/systemui/statusbar/phone/SettingsPanelView.java

+ frameworks/base/core/java/com/android/internal/widget/SMouseTouchView.java
- frameworks/base/core/java/android/hardware/usb/UsbManager.java
- frameworks/base/core/java/android/provider/Settings.java
- frameworks/base/core/java/android/view/SurfaceControl.java
- frameworks/base/core/jni/android_view_SurfaceControl.cpp
- frameworks/base/core/jni/android_media_AudioSystem.cpp

- frameworks/base/media/java/android/media/AudioService.java
- frameworks/base/media/java/android/media/AudioSystem.java

- frameworks/base/policy/src/com/android/internal/policy/impl/PhoneWindowManager.java

- frameworks/base/services/java/com/android/server/usb/UsbDeviceManager.java
- frameworks/base/services/java/com/android/server/power/PowerManagerService.java
- frameworks/base/services/java/com/android/server/power/Notifier.java
- frameworks/base/services/java/com/android/server/display/LocalDisplayAdapter.java
- frameworks/base/services/java/com/android/server/am/ActivityManagerService.java
- frameworks/base/services/java/com/android/server/wm/WindowManagerService.java
- frameworks/base/services/java/com/android/server/input/InputManagerService.java
- frameworks/base/services/jni/com_android_server_power_PowerManagerService.h
- frameworks/base/services/jni/com_android_server_power_PowerManagerService.cpp
- frameworks/base/services/jni/com_android_server_input_InputManagerService.cpp
- frameworks/base/services/input/InputReader.h
- frameworks/base/services/input/InputReader.cpp

- frameworks/native/include/gui/SurfaceComposerClient.h
- frameworks/native/include/gui/ISurfaceComposer.h
- frameworks/native/libs/gui/SurfaceComposerClient.cpp
- frameworks/native/libs/gui/ISurfaceComposer.cpp
- frameworks/native/services/surfaceflinger/SurfaceFlinger.h
- frameworks/native/services/surfaceflinger/SurfaceFlinger.cpp
- frameworks/native/services/surfaceflinger/DisplayHardware/HWComposer.h
- frameworks/native/services/surfaceflinger/DisplayHardware/HWComposer.cpp

- frameworks/av/include/media/AudioSystem.h
- frameworks/av/include/media/IAudioPolicyService.h
- frameworks/av/media/libmedia/AudioSystem.cpp
- frameworks/av/media/libmedia/IAudioPolicyService.cpp
- frameworks/av/services/audioflinger/AudioPolicyService.h
- frameworks/av/services/audioflinger/AudioPolicyService.cpp

- hardware/libhardware/include/hardware/audio_policy.h
- hardware/libhardware/modules/audio/audio_policy.c
- hardware/libhardware_legacy/audio/audio_policy_hal.cpp
- hardware/libhardware_legacy/include/hardware_legacy/AudioSystemLegacy.h
- hardware/libhardware_legacy/include/hardware_legacy/AudioPolicyInterface.h
- hardware/libhardware_legacy/include/hardware_legacy/AudioPolicyManagerBase.h
- hardware/libhardware_legacy/audio/AudioPolicyManagerBase.cpp

- system/core/include/system/audio.h

```


##### 02-Kernel/
```
- drivers/usb/gadget/android.c
- drivers/usb/gadget/f_smouse.c
- drivers/usb/gadget/f_faketooth_mouse.c
- drivers/usb/gadget/f_faketooth_keyboard.c
- drivers/usb/gadget/f_faketooth_audio.c
```
