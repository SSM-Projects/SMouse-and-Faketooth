package android.app;

import java.util.List;

import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHeadset;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.os.IBinder;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

public class FaketoothService extends Service {
    private final String TAG = "FaketoothService";

    private boolean mReceiverRegistered;
    private String mBluetoothProfile;
    
    private Context mContext;
    private AudioManager mAudioManager;
    private BluetoothAdapter mBluetoothAdapter;
    private BluetoothHeadset mBluetoothHeadset;

    private FaketoothA2DPThread mA2DPThread;
    private FaketoothSCOThread mSCOThread;
    private FaketoothSCOMicThread mSCOMicThread;

    private final BroadcastReceiver mSCOAudioStateReceiver = new BroadcastReceiver() {      
        @Override
        public void onReceive(Context context, Intent intent) {
            int state = intent.getIntExtra(AudioManager.EXTRA_SCO_AUDIO_STATE, -1);
            if (state == AudioManager.SCO_AUDIO_STATE_CONNECTED) {
                mSCOThread = new FaketoothSCOThread();
                if (mSCOThread != null) {
                    mSCOThread.start();
                    while (mSCOThread.threadHandler == null)
                        ;
                    mSCOThread.threadHandler.sendEmptyMessage(0);
                }               
                mSCOMicThread = new FaketoothSCOMicThread();
                if (mSCOMicThread != null) {
                    mSCOMicThread.start();
                    while (mSCOMicThread.threadHandler == null)
                        ;
                    mSCOMicThread.threadHandler.sendEmptyMessage(0);
                }               
            }
        }
    };

    private BluetoothProfile.ServiceListener mProfileListener = new BluetoothProfile.ServiceListener() {
        @Override
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            if (profile == BluetoothProfile.HEADSET) {
                mBluetoothHeadset = (BluetoothHeadset) proxy;
                List<BluetoothDevice> pairedDevices = mBluetoothHeadset.getConnectedDevices();
                if (pairedDevices.size() > 0) {                 
                    IntentFilter intentFilter = new IntentFilter();
                    intentFilter.addAction(AudioManager.ACTION_SCO_AUDIO_STATE_UPDATED);
                    mContext.registerReceiver(mSCOAudioStateReceiver, intentFilter);
                    
                    mReceiverRegistered = true;

                    mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
                    mAudioManager.startBluetoothSco();                  
                    mAudioManager.setBluetoothScoOn(true);
                    mAudioManager.setMode(AudioManager.MODE_IN_CALL);
                }
            }
        }       
        @Override
        public void onServiceDisconnected(int profile) {
            if (profile == BluetoothProfile.HEADSET) {
                mBluetoothHeadset = null;
            }
        }       
    };

    @Override
    public void onCreate() {
        super.onCreate();
        mContext = this;
        mReceiverRegistered = false;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        mBluetoothProfile = intent.getExtras().getString("BluetoothProfile");

        if (mBluetoothProfile.equals("A2DP")) {
            mA2DPThread = new FaketoothA2DPThread();
            if (mA2DPThread != null) {
                mA2DPThread.start();
                while (mA2DPThread.threadHandler == null)
                    ;
                mA2DPThread.threadHandler.sendEmptyMessage(0);
            }

        } else if (mBluetoothProfile.equals("SCO")) {
            mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();       
            mBluetoothAdapter.getProfileProxy(mContext, mProfileListener, BluetoothProfile.HEADSET);
        }

        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mA2DPThread != null)
            mA2DPThread.flag = false;
        if (mSCOThread != null)
            mSCOThread.flag = false;
        if (mSCOMicThread != null)
            mSCOMicThread.flag = false;
        if (mAudioManager != null) {
            mAudioManager.stopBluetoothSco();
            mAudioManager.setBluetoothScoOn(false);
            mAudioManager.setMode(AudioManager.MODE_NORMAL);
        }
        if (mReceiverRegistered)
            unregisterReceiver(mSCOAudioStateReceiver);
        if (mBluetoothAdapter != null && mBluetoothHeadset != null)
            mBluetoothAdapter.closeProfileProxy(BluetoothProfile.HEADSET, mBluetoothHeadset);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    class FaketoothA2DPThread extends Thread {
        public Handler threadHandler;
        public boolean flag = true;
        public void run() {
            Looper.prepare();
            threadHandler = new Handler() {
                public void handleMessage(Message msg) {
                    while (flag) {
                        while (flag && nativeFaketoothA2DPInit() < 0) {
                            try { Thread.sleep(100); }
                            catch (InterruptedException e) {}
                        }
                        while (flag && nativeFaketoothA2DPEnable() < 0) {
                            try { Thread.sleep(100); }
                            catch (InterruptedException e) {}
                        }
                        while (flag && nativeFaketoothA2DPDo() == 0) {

                        }
                        nativeFaketoothA2DPDisable();
                        try { Thread.sleep(100); }
                        catch (InterruptedException e) {}
                    }
                }
            };
            Looper.loop();
        }
    }

    class FaketoothSCOThread extends Thread {
        public Handler threadHandler;
        public boolean flag = true;
        public void run() {
            Looper.prepare();
            threadHandler = new Handler() {
                public void handleMessage(Message msg) {
                    while (flag) {
                        while (flag && nativeFaketoothSCOInit() < 0) {
                            try { Thread.sleep(100); }
                            catch (InterruptedException e) {}
                        }
                        while (flag && nativeFaketoothSCOEnable() < 0) {
                            try { Thread.sleep(100); }
                            catch (InterruptedException e) {}
                        }
                        while (flag && nativeFaketoothSCODo() == 0) {

                        }
                        nativeFaketoothSCODisable();
                        try { Thread.sleep(100); }
                        catch (InterruptedException e) {}
                    }
                }
            };
            Looper.loop();
        }
    }

    class FaketoothSCOMicThread extends Thread {
        public Handler threadHandler;
        public boolean flag = true;
        public void run() {
            Looper.prepare();
            threadHandler = new Handler() {
                public void handleMessage(Message msg) {
                    while (flag) {
                        while (flag && nativeFaketoothSCOMicInit() < 0) {
                            try { Thread.sleep(100); }
                            catch (InterruptedException e) {}
                        }
                        while (flag && nativeFaketoothSCOMicEnable() < 0) {
                            try { Thread.sleep(100); }
                            catch (InterruptedException e) {}
                        }
                        while (flag && nativeFaketoothSCOMicDo() == 0) {

                        }
                        nativeFaketoothSCOMicDisable();
                        try { Thread.sleep(100); }
                        catch (InterruptedException e) {}
                    }
                }
            };
            Looper.loop();
        }
    }

    static {
        System.loadLibrary("faketooth");
    }

    private native int nativeFaketoothA2DPInit();
    private native int nativeFaketoothA2DPEnable();
    private native int nativeFaketoothA2DPDo();
    private native int nativeFaketoothA2DPDisable();

    private native int nativeFaketoothSCOInit();
    private native int nativeFaketoothSCOEnable();
    private native int nativeFaketoothSCODo();
    private native int nativeFaketoothSCODisable();

    private native int nativeFaketoothSCOMicInit();
    private native int nativeFaketoothSCOMicEnable();
    private native int nativeFaketoothSCOMicDo();
    private native int nativeFaketoothSCOMicDisable();
}
