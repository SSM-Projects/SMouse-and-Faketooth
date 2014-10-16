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
        int ret = nativeFaketoothEnable();
        if (ret != 0) {
            printRetVal(ret);
        } else {
            mThread = new FaketoothThread();
            if (mThread != null) {
                mThread.start();
                while (mThread.threadHandler == null)
                    ;
                mThread.threadHandler.sendEmptyMessage(0);
            }
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

    public void printRetVal(int ret) {
        String msg = "Nothing";

        switch (ret) {
            case 0: msg = "No error"; break;
            case 1: msg = "open error"; break;
            case 2: msg = "at_create error"; break;
            case 3: msg = "at_set error"; break;
            case 4: msg = "read < 0 error"; break;
            case 5: msg = "read = 0 error"; break;
            case 6: msg = "at_write error"; break;
            default: msg = "Unknown " + ret; break;
        }

        Log.e(TAG, msg);
    }

    class FaketoothThread extends Thread {
        public Handler threadHandler;
        public void run() {
            Looper.prepare();
            threadHandler = new Handler() {
                public void handleMessage(Message msg) {
                    int ret;
                    while (!(Thread.interrupted())) {
                        ret = nativeFaketoothDo();
                        if (ret != 0) {
                            printRetVal(ret);
                        }
                    }
                }
            };
            Looper.loop();
        }
    }

    static {
        System.loadLibrary("faketooth");
    }

    private native int nativeFaketoothEnable();
    private native int nativeFaketoothDo();
    private native int nativeFaketoothDisable();
}
