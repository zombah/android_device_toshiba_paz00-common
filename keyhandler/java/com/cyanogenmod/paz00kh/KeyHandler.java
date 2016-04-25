/*
 * Copyright (C) 2012 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.cyanogenmod.paz00kh;

import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.IBinder;
import android.os.IPowerManager;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.util.Log;
import android.view.KeyEvent;

import com.android.internal.os.DeviceKeyHandler;


public final class KeyHandler implements DeviceKeyHandler {
	private static final String TAG = "paz00_KeyHandler";

	private static final int MINIMUM_BACKLIGHT = 1;
	private static final int MAXIMUM_BACKLIGHT = 255;

	private final Context mContext;
	private final Handler mHandler;
	private final Intent mSettingsIntent;
	private IPowerManager mPowerManager;

	public KeyHandler(Context context) {
		mContext = context;
		mHandler = new Handler();

		mSettingsIntent = new Intent(Intent.ACTION_MAIN, null);
		mSettingsIntent.setAction(Settings.ACTION_SETTINGS);
		mSettingsIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK
				| Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
	}

	@Override
	public int handleKeyEvent(KeyEvent event) {
        	if (event.getAction() != KeyEvent.ACTION_DOWN
                	|| event.getRepeatCount() != 0) {
	            return KEYEVENT_UNCAUGHT;
	        }


		Log.d(TAG, "KeyEvent: action=" + event.getAction()
				+ ", flags=" + event.getFlags()
				+ ", canceled=" + event.isCanceled()
				+ ", keyCode=" + event.getKeyCode()
				+ ", deviceId=" + event.getDeviceId()
				+ ", scanCode=" + event.getScanCode()
				+ ", metaState=" + event.getMetaState()
				+ ", repeatCount=" + event.getRepeatCount());

		switch (event.getKeyCode()) {
			case KeyEvent.KEYCODE_BRIGHTNESS_DOWN:
				brightnessDown();
				break;
			case KeyEvent.KEYCODE_BRIGHTNESS_UP:
				brightnessUp();
				break;
			case KeyEvent.KEYCODE_SCREENSHOT:
				takeScreenshot();
				break;
			case KeyEvent.KEYCODE_SETTINGS:
				launchSettings();
				break;
			default:
				return KEYEVENT_UNCAUGHT;
		}
		return KEYEVENT_UNCAUGHT;
	}

	private void brightnessDown() {
		setBrightnessMode(Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL);

		int value = getBrightness(MINIMUM_BACKLIGHT);

		value -= 10;
		if (value < MINIMUM_BACKLIGHT) {
			value = MINIMUM_BACKLIGHT;
		}
		setBrightness(value);
	}

	private void brightnessUp() {
		setBrightnessMode(Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL);

		int value = getBrightness(MAXIMUM_BACKLIGHT);

		value += 10;
		if (value > MAXIMUM_BACKLIGHT) {
			value = MAXIMUM_BACKLIGHT;
		}
		setBrightness(value);
	}

	private void setBrightnessMode(int mode) {
		Settings.System.putInt(mContext.getContentResolver(),
				Settings.System.SCREEN_BRIGHTNESS_MODE, mode);
	}

	private void setBrightness(int value) {
		if (mPowerManager == null) {
			mPowerManager = IPowerManager.Stub.asInterface(
					ServiceManager.getService("power"));
		}
		try {
			mPowerManager.setBacklightBrightness(value);
		} catch (RemoteException ex) {
			Log.e(TAG, "Could not set backlight brightness", ex);
		}
		Settings.System.putInt(mContext.getContentResolver(),
				Settings.System.SCREEN_BRIGHTNESS, value);
	}

	private int getBrightness(int def) {
		int value = def;
		try {
			value = Settings.System.getInt(mContext.getContentResolver(),
					Settings.System.SCREEN_BRIGHTNESS);
		} catch (SettingNotFoundException ex) {
			Log.e(TAG, "Could not get backlight brightness", ex);
		}
		return value;
	}

	private void launchSettings() {
		try {
			mContext.startActivity(mSettingsIntent);
		} catch (ActivityNotFoundException ex) {
			Log.e(TAG, "Could not launch settings intent", ex);
		}
	}

	/*
	 * Screenshot stuff kanged from PhoneWindowManager
	 */

	final Object mScreenshotLock = new Object();
	ServiceConnection mScreenshotConnection = null;

	final Runnable mScreenshotTimeout = new Runnable() {
		@Override
		public void run() {
			synchronized (mScreenshotLock) {
				if (mScreenshotConnection != null) {
					mContext.unbindService(mScreenshotConnection);
					mScreenshotConnection = null;
				}
			}
		}
	};

	// Assume this is called from the Handler thread.
	private void takeScreenshot() {
		synchronized (mScreenshotLock) {
			if (mScreenshotConnection != null) {
				return;
			}
			ComponentName cn = new ComponentName("com.android.systemui",
					"com.android.systemui.screenshot.TakeScreenshotService");
			Intent intent = new Intent();
			intent.setComponent(cn);
			ServiceConnection conn = new ServiceConnection() {
				@Override
				public void onServiceConnected(ComponentName name, IBinder service) {
					synchronized (mScreenshotLock) {
						if (mScreenshotConnection != this) {
							return;
						}
						Messenger messenger = new Messenger(service);
						Message msg = Message.obtain(null, 1);
						final ServiceConnection myConn = this;
						Handler h = new Handler(mHandler.getLooper()) {
							@Override
							public void handleMessage(Message msg) {
								synchronized (mScreenshotLock) {
									if (mScreenshotConnection == myConn) {
										mContext.unbindService(mScreenshotConnection);
										mScreenshotConnection = null;
										mHandler.removeCallbacks(mScreenshotTimeout);
									}
								}
							}
						};
						msg.replyTo = new Messenger(h);
						msg.arg1 = msg.arg2 = 0;
						try {
							messenger.send(msg);
						} catch (RemoteException e) {
						}
					}
				}

				@Override
				public void onServiceDisconnected(ComponentName name) {
				}
			};
			if (mContext.bindService(intent, conn, Context.BIND_AUTO_CREATE)) {
				mScreenshotConnection = conn;
				mHandler.postDelayed(mScreenshotTimeout, 10000);
			}
		}
	}
}
