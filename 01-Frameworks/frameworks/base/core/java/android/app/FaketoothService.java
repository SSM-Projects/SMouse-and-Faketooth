package android.app;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

public class FaketoothService extends Service {
    private final String TAG = "FaketoothService";
    private FaketoothThread mThread;

    @Override
    public void onCreate() {
        super.onCreate();
        mThread = new FaketoothThread();
        if (mThread != null) {
            mThread.start();
            while (mThread.threadHandler == null)
                ;
            mThread.threadHandler.sendEmptyMessage(0);
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mThread != null) {
            mThread.interrupt();
        }
        nativeFaketoothDisable();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    class FaketoothThread extends Thread {
        public Handler threadHandler;
        public void run() {
            Looper.prepare();
            threadHandler = new Handler() {
                public void handleMessage(Message msg) {
                    while (!(Thread.interrupted())) {
                        while (nativeFaketoothInit() < 0) {
                            try { Thread.sleep(100); }
                            catch (InterruptedException e) {}
                        }
                        while (nativeFaketoothEnable() < 0) {
                            try { Thread.sleep(100); }
                            catch (InterruptedException e) {}
                        }
                        while (nativeFaketoothDo() == 0) {

                        }
                        nativeFaketoothDisable();
                    }
                }
            };
            Looper.loop();
        }
    }

    static {
        System.loadLibrary("faketooth");
    }

    private native int nativeFaketoothInit();
    private native int nativeFaketoothEnable();
    private native int nativeFaketoothDo();
    private native int nativeFaketoothDisable();
}
