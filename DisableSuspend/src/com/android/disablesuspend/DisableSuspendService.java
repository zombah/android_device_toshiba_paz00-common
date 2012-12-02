package com.android.disablesuspend;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.os.PowerManager;
import android.util.Log;

public class DisableSuspendService extends Service {

	private static final String TAG = "DisableSuspendService";

	private PowerManager pm;
	private PowerManager.WakeLock wl;

	private final IBinder mBinder = new LocalBinder();

	@Override
	public void onCreate() {
		super.onCreate();

		Log.d(TAG, "create");

		pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
		wl = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK, "DisableSuspend");
	}

	@Override
	public void onDestroy() {
		super.onDestroy();

		if (wl.isHeld()) {
			Log.d(TAG, "onDestroy release");
			wl.release();
		}
	}

	public class LocalBinder extends Binder {
		DisableSuspendService getService() {
			return DisableSuspendService.this;
		}
	}

	@Override
	public IBinder onBind(Intent intent) {
		return mBinder;
	}

	public void acquire() {
		wl.acquire();
	}

	public boolean isHeld() {
		return wl.isHeld();
	}

	public void release() {
		if (wl.isHeld()) {
			wl.release();
		}
	}
}