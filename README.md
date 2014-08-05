# SMouse

SMouse Project, Jun-Jul, 2014.<br>
Samsung Software Membership, Seoul, South Korea.<br>
Android part. Jeonghyeon Kim (kimjeongss@gmail.com)<br>
Windows part. Youngjun Lee (tgbnhy02@gmail.com)


#### Description

- To be updated


#### Working Demo

- To be updated


#### Target Environment

- Google Nexus 5 (hammerhead)
- Android 4.4.2 KitKat (r1, KOT49H)
- Kernel 3.4.0
- Windows 7 x86/x64


#### License and Copyright Notices

- To be updated


#### Added or Modified sources list
##### Kernel
```
- drivers/usb/gadget/android.c
- drivers/usb/gadget/f_mouse.c
```

##### Frameworks
```
- out/target/product/hammerhead/system/lib/libsmouse.so

- frameworks/base/packages/SystemUI/res/drawable-hdpi/ic_qs_mouse_on.png
- frameworks/base/packages/SystemUI/res/drawable-hdpi/ic_qs_mouse_off.png
- frameworks/base/packages/SystemUI/src/com/android/systemui/statusbar/policy/MouseController.java
- frameworks/base/packages/SystemUI/res/values/strings.xml
- frameworks/base/packages/SystemUI/src/com/android/systemui/statusbar/phone/QuickSettings.java
- frameworks/base/packages/SystemUI/src/com/android/systemui/statusbar/phone/QuickSettingsModel.java
- frameworks/base/packages/SystemUI/src/com/android/systemui/statusbar/phone/PhoneStatusBar.java
- frameworks/base/packages/SystemUI/src/com/android/systemui/statusbar/phone/SettingsPanelView.java

- frameworks/base/core/java/com/android/internal/widget/SMouseTouchView.java
- frameworks/base/core/java/android/hardware/usb/UsbManager.java
- frameworks/base/core/java/android/provider/Settings.java
- frameworks/base/core/java/android/view/SurfaceControl.java
- frameworks/base/core/jni/android_view_SurfaceControl.cpp

- frameworks/base/policy/src/com/android/internal/policy/impl/PhoneWindowManager.java

- frameworks/base/services/java/com/android/server/usb/UsbDeviceManager.java
- frameworks/base/services/java/com/android/server/power/PowerManagerService.java
- frameworks/base/services/java/com/android/server/power/Notifier.java
- frameworks/base/services/java/com/android/server/display/LocalDisplayAdapter.java
- frameworks/base/services/java/com/android/server/am/ActivityManagerService.java
- frameworks/base/services/java/com/android/server/wm/WindowManagerService.java
- frameworks/base/services/jni/com_android_server_power_PowerManagerService.cpp
- frameworks/base/services/jni/com_android_server_power_PowerManagerService.h
- frameworks/base/services/jni/com_android_server_input_InputManagerService.cpp

- frameworks/native/include/gui/SurfaceComposerClient.h
- frameworks/native/include/gui/ISurfaceComposer.h
- frameworks/native/libs/gui/SurfaceComposerClient.cpp
- frameworks/native/libs/gui/ISurfaceComposer.cpp
- frameworks/native/services/surfaceflinger/SurfaceFlinger.h
- frameworks/native/services/surfaceflinger/SurfaceFlinger.cpp
- frameworks/native/services/surfaceflinger/DisplayHardware/HWComposer.h
- frameworks/native/services/surfaceflinger/DisplayHardware/HWComposer.cpp
```


##### Unsolved Issues

- To be updated
