package com.android.disablesuspend;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.CheckBox;

import com.android.disablesuspend.DisableSuspendService.LocalBinder;

public class DisableSuspendActivity extends Activity {
	private static final String TAG = "DisableSuspendActivity";

	DisableSuspendService mService;

	ServiceConnection mConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName className, IBinder service) {
			Log.d(TAG, "onServiceConnected");

			LocalBinder binder = (LocalBinder) service;
			mService = binder.getService();

			final CheckBox checkBox = (CheckBox) findViewById(R.id.CheckBox1);

			checkBox.setChecked(mService.isHeld());

			checkBox.setOnClickListener(new OnClickListener() {
				@Override
				public void onClick(View v) {
					if (checkBox.isChecked()) {
						Log.d(TAG, "acquire");
						mService.acquire();
					} else {
						Log.d(TAG, "release");
						mService.release();
					}
				}
			});

		}

		@Override
		public void onServiceDisconnected(ComponentName arg0) {
		}
	};

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		Log.d(TAG, "Create");
		setContentView(R.layout.main);

		Intent intent = new Intent(this, DisableSuspendService.class);
		startService(new Intent(this, DisableSuspendService.class));
		bindService(intent, mConnection, Context.BIND_AUTO_CREATE);
	}

	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();

		unbindService(mConnection);
	}

}