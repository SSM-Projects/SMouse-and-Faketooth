package com.android.internal.widget;

import android.content.Context;
import android.hardware.input.InputManager;
import android.hardware.input.InputManager.InputDeviceListener;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.util.Log;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;

public class SMouseTouchView extends View implements InputDeviceListener, SensorEventListener {

	private static final String TAG = "SMouseTouchView";
	public static final boolean DEBUG = true;

	/* Device accessor */
	private SMouseNative mSMouseNative;

	/* Members related touches */
	private static final int SCREEN_WIDTH = 1080;	// Nexus 5
	private static final int SCREEN_HEIGHT = 1920;	// Nexus 5

	private final InputManager mIm;

	private int curWheel;
	private float curCoord;

	/* Members related sensors */
	private SensorManager mSensorManager;

	private Sensor mAccelerometer;
	private Sensor mGyroscope;
	private Sensor mGravity;

	private float mVx;
	private float mVy;

	private float[] mGravityData;
	private float[] mAccelData;
	private float[] mGyroData;

	private int mAccelChangedCnt;
	private int mGravityChangedCnt;

	private float mCalibrationX;
	private float mCalibrationY;
	private float mCalibrationG;

	private int mGyroMovementFlag;
	private int mGravityMovementFlag;

	private int mSavedReactiveTime;

	private int mEndCheckCnt;
	private int mEndCheckFlag;

	/**
	 * Constructor
	 */
	public SMouseTouchView(Context context) {
		super(context);

		/* Init device accessor */
		mSMouseNative = new SMouseNative();
		mSMouseNative.openDevice();	// Open device

		/* Init member variables */
		setFocusableInTouchMode(true);
		mIm = (InputManager)context.getSystemService(Context.INPUT_SERVICE);

		mVx = 0;
		mVy = 0;

		mGravityData = new float[3];
		mAccelData = new float[3];
		mGyroData = new float[3];

		mAccelChangedCnt = 0;
		mGravityChangedCnt = 0;

		mCalibrationX = 0.0f;
		mCalibrationY = 0.0f;
		mCalibrationG = 0.0f;

		mGyroMovementFlag = 0;
		mGravityMovementFlag = 0;

		mEndCheckCnt = 0;
		mEndCheckFlag = 0;

		/* Init sensors listener */
		mSensorManager = (SensorManager)context.getSystemService(Context.SENSOR_SERVICE);

		mAccelerometer = mSensorManager.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION);
		mGyroscope = mSensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
		mGravity = mSensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY);

		mSensorManager.registerListener(this, mAccelerometer, SensorManager.SENSOR_DELAY_FASTEST);
		mSensorManager.registerListener(this, mGyroscope, SensorManager.SENSOR_DELAY_FASTEST);
		mSensorManager.registerListener(this, mGravity, SensorManager.SENSOR_DELAY_FASTEST);	
	}

	/**
	 * Destructor (User-defined)
	 */
	public void destroyView() {
		mSMouseNative.closeDevice(); // Close device
		mSMouseNative = null;
		mSensorManager.unregisterListener(this);
	}

	/**
	 * Touches evnets handler
	 */
	public void addPointerEvent(MotionEvent event) {

		final int action = event.getAction();
		final float x = SCREEN_WIDTH - event.getRawX();
		final float y = SCREEN_HEIGHT - event.getRawY();

		if (action == MotionEvent.ACTION_DOWN) {

			if (y > SCREEN_HEIGHT / 4) {
				return;

			} else if (x < SCREEN_WIDTH / 2 - 100) {	// Left Button Down 
				if (DEBUG) {
					Log.d(TAG, "Left Button Down");
				}
				if (mSMouseNative != null && mSMouseNative.isOpened()) {
					mSMouseNative.writeValues(1, 0, 0, 0);
				}

			} else if (x > SCREEN_WIDTH / 2 + 100) {	// Right Button Down 
				if (DEBUG) {
					Log.d(TAG, "Right Button Down");
				}
				if (mSMouseNative != null && mSMouseNative.isOpened()) {
					mSMouseNative.writeValues(2, 0, 0, 0);
				}

			} else {									// Wheel Detect 
				curCoord = y;
				curWheel = 0;
				if (DEBUG) {
					Log.d(TAG, "Wheel Detect " + curWheel);
				}

			}

		} else if (action == MotionEvent.ACTION_UP) {

			if (y > SCREEN_HEIGHT / 4) {
				return;

			} else if (x < SCREEN_WIDTH / 2 - 100) {	// Left Button Up
				if (DEBUG) {
					Log.d(TAG, "Left Button Up");
				}
				if (mSMouseNative != null && mSMouseNative.isOpened()) {
					mSMouseNative.writeValues(4, 0, 0, 0);
				}

			} else if (x > SCREEN_WIDTH / 2 + 100) {	// Right Button Up
				if (DEBUG) {
					Log.d(TAG, "Right Button Up");
				}
				if (mSMouseNative != null && mSMouseNative.isOpened()) {
					mSMouseNative.writeValues(5, 0, 0, 0);
				}

			} else {
				return;
			}

		} else if (action == MotionEvent.ACTION_MOVE) {

			if (y > SCREEN_HEIGHT / 4)
				return;

			else if (x < SCREEN_WIDTH / 2 - 100)
				return;

			else if (x > SCREEN_WIDTH / 2 + 100)
				return;

			else {
				if (curCoord - y > SCREEN_HEIGHT / 4 / 10) {				// Wheel Up
					curCoord = y;
					curWheel++;
					if (DEBUG) {
						Log.d(TAG, "Wheel Up " + curWheel);
					}
					if (mSMouseNative != null && mSMouseNative.isOpened()) {
						mSMouseNative.writeValues(0, 1, 0, 0);
					}

				} else if (curCoord - y < -1 * (SCREEN_HEIGHT / 4 / 10)) {	// Wheel Down
					curCoord = y;
					curWheel--;
					if (DEBUG) {
						Log.d(TAG, "Wheel Down " + curWheel);
					}
					if (mSMouseNative != null && mSMouseNative.isOpened()) {
						mSMouseNative.writeValues(0, -1, 0, 0);
					}

				} else {
					return;

				}
			}

		} else {


		}
	}

	@Override
	public boolean onTouchEvent(MotionEvent event) {
		addPointerEvent(event);
		if (event.getAction() == MotionEvent.ACTION_DOWN && !isFocused())
			requestFocus();
		return true;
	}

	@Override
	public boolean onGenericMotionEvent(MotionEvent event) {
		final int source = event.getSource();
		if ((source & InputDevice.SOURCE_CLASS_POINTER) != 0)
			addPointerEvent(event);
		return true;
	}

	@Override
	protected void onAttachedToWindow() {
		super.onAttachedToWindow();
		mIm.registerInputDeviceListener(this, getHandler());
	}

	@Override
	protected void onDetachedFromWindow() {
		super.onDetachedFromWindow();
		mIm.unregisterInputDeviceListener(this);
	}

	@Override
	public void onInputDeviceAdded(int deviceId) {

	}

	@Override
	public void onInputDeviceChanged(int deviceId) {

	}

	@Override
	public void onInputDeviceRemoved(int deviceId) {

	}

	@Override
	public void onAccuracyChanged(Sensor sensor, int accuracy) {

	}

	@Override
	public void onSensorChanged(SensorEvent event) {

		/**
		 * LINEAR ACCELERATION Sensor
		 */
		if (event.sensor.getType() == Sensor.TYPE_LINEAR_ACCELERATION) {

			mAccelChangedCnt++;

			/* #1 Reactive Lock Filter */
			final int reactiveLockTerm = 30;

			if (mGravityMovementFlag == 3) {
				mSavedReactiveTime = mAccelChangedCnt;
				mGravityMovementFlag = 2;
				return;

			} else if (mGravityMovementFlag == 2 && mAccelChangedCnt < mSavedReactiveTime + reactiveLockTerm) {
				return;

			} else if (mGravityMovementFlag == 2 && mAccelChangedCnt == mSavedReactiveTime + reactiveLockTerm) {
				mGravityMovementFlag = 1;

			} else
				;

			/* #2 Get Values */
			mAccelData[0] = event.values[0];
			mAccelData[1] = event.values[1];

			/* #3 Calibration */
			final float calibrationInit = 50;
			final float calibrationTerm = 100;

			if (mAccelChangedCnt < calibrationInit) {
				return;

			} else if (mAccelChangedCnt < calibrationInit + calibrationTerm) {
				mCalibrationX += mAccelData[0];
				mCalibrationY += mAccelData[1];
				return;

			} else if (mAccelChangedCnt == calibrationInit + calibrationTerm) {
				mCalibrationX /= 100;
				mCalibrationY /= 100;
				return;

			} else {
				mAccelData[0] -= mCalibrationX;
				mAccelData[1] -= mCalibrationY;
			}

			/* #4 Mechanical Filtering Window */
			final float window = 0.06f;
			final int endCheckTime = 3;

			if ( (mAccelData[0] <= window && mAccelData[0] >= -window)
					&& (mAccelData[1] <= window && mAccelData[1] >= -window) ) {

				mAccelData[0] = 0.0f;
				mAccelData[1] = 0.0f;

				/* #5 Movement End Check */
				mEndCheckCnt++;

				if (mEndCheckFlag == 0) {
					mEndCheckFlag = 1;
					mEndCheckCnt = 1;

				} else	if (mEndCheckCnt == endCheckTime) {
					mVx = 0.0f;
					mVy = 0.0f;
				} else
					;

			} else if (mEndCheckFlag == 1) {
				mEndCheckFlag = 0;

			} else
				;

			/* #6 Positioning */
			final float sampleTime = 0.007f;
			final int scaleUp = 1000000;

			float moveX = (float) ((mVx * sampleTime) + (0.5 * mAccelData[0] * sampleTime * sampleTime));
			float moveY = (float) ((mVy * sampleTime) + (0.5 * mAccelData[1] * sampleTime * sampleTime));
			
			int xferX = (int) (moveX * scaleUp);
			int xferY = (int) (moveY * scaleUp);

			if (mGyroMovementFlag == 1 && mGravityMovementFlag == 1) {
				mVx = mVx + (mAccelData[0] * sampleTime);
				mVy = mVy + (mAccelData[1] * sampleTime);

				if (xferX != 0 && xferY != 0) {
					if (DEBUG) {
						Log.d(TAG, "Move X " + xferX + "\tY " + xferY);
					}
					if (mSMouseNative != null && mSMouseNative.isOpened()) {
						mSMouseNative.writeValues(0, 0, xferX, xferY);
					}
				}

			} else {
				mVx = 0.0f;
				mVy = 0.0f;
			}

		/**
		 * GYROSCOPE Sensor
		 */
		} else if (event.sensor.getType() == Sensor.TYPE_GYROSCOPE) {

			mGyroData[0] = event.values[0];
			mGyroData[1] = event.values[1];
			mGyroData[2] = event.values[2];

			/* Gyroscope Movement Check */
			if ( (Math.round(mGyroData[0] * 100d) == 0)
					&& (Math.round(mGyroData[1] * 100d) == 0)
					&& (Math.round(mGyroData[2] * 100d) == 0) )
				mGyroMovementFlag = 0;
			else
				mGyroMovementFlag = 1;

		/**
		 * GRAVITY Sensor
		 */
		} else if (event.sensor.getType() == Sensor.TYPE_GRAVITY) {

			mGravityChangedCnt++;

			mGravityData[0] = event.values[0];
			mGravityData[1] = event.values[1];
			mGravityData[2] = event.values[2];

			/* Gravity Movement Check */
			final float calibrationInit = 50;
			final float calibrationTerm = 100;
			final float window = 0.007f;

			if (mGravityChangedCnt < calibrationInit) {

			} else if (mGravityChangedCnt < calibrationInit + calibrationTerm) {
				mCalibrationG += mGravityData[2];

			} else if (mGravityChangedCnt == calibrationInit + calibrationTerm) {
				mCalibrationG /= 100;

			} else {
				if ( ((mGravityData[2] - mCalibrationG) <=  window)
						&& ((mGravityData[2] - mCalibrationG) >= -window) ) {
					if (mGravityMovementFlag == 0)	// Reactive
						mGravityMovementFlag = 3;
					else							// Keep Active or Keep Lock
						;
				} else
					mGravityMovementFlag = 0;
			}

		} else {

		}

	}

	/**
	 * Load native functions for access device
	 */
	static {
		System.loadLibrary("smouse");
	}

	private static native int nativeOpenDevice();
	private static native int nativeCloseDevice(int fd);
	private static native int nativeWriteValues(int fd, int btn, int wheel, int moveX, int moveY);

	/**
	 * Device access class
	 */
	class SMouseNative {

		private static final String TAG = "SMouseTouchView.SMouseNative";

		private int mFD;
		private boolean mIsOpened;

		public SMouseNative() {
			mFD = -1;
			mIsOpened = false;
		}

		public boolean openDevice() {
			if (mIsOpened) {
				Log.e(TAG, "Couldn't open device, device already opened.");
				return true;
			}

			mFD = nativeOpenDevice();
			if (mFD == -1) {
				Log.e(TAG, "Couldn't open device, device open failed.");
				return false;
			} 

			mIsOpened = true;
			if (DEBUG) {
				Log.d(TAG, "Device opened.");
			}
			return true;
		}

		public boolean closeDevice() {
			if (!mIsOpened) {
				Log.e(TAG, "Couldn't close device, device not opened.");
				return false;
			}

			nativeCloseDevice(mFD);
			mIsOpened = false;
			if (DEBUG) {
				Log.d(TAG, "Device closed. " + mFD);
			}
			return true;
		}

		public boolean writeValues(int btn, int wheel, int moveX, int moveY) {
			if (!mIsOpened) {
				Log.e(TAG, "Couldn't write values, device not opened.");
				return false;
			}

			nativeWriteValues(mFD, btn, wheel, moveX, moveY);
			if (DEBUG) {
				Log.d(TAG, "Write values. " + mFD + " " + btn + " " + wheel + " " + moveX + " " + moveY);
			}
			return true;
		}

		public boolean isOpened() {
			return mIsOpened;
		}
	}

}
