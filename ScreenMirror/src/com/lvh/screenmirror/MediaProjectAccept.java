package com.lvh.screenmirror;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;

public class MediaProjectAccept extends Activity {
	private static final int PERMISSION_CODE = 0x1234;
	private static final String TAG = "ScreenMirror-MediaProjectAccept";

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		Log.d(TAG, "Intent:" + getIntent());
		Intent intent = getIntent().<Intent> getParcelableExtra(Intent.EXTRA_INTENT);
		Log.d(TAG, "Intent:" + intent);
		startActivityForResult(intent,PERMISSION_CODE);
	}

	@Override
	public void onActivityResult(int requestCode, int resultCode, Intent data) {

		Log.d(TAG, "resultCode : " + resultCode + " request code: " + requestCode);
		if (requestCode != PERMISSION_CODE) {
			Log.e(TAG, "Unknown request code: " + requestCode);
			return;
		}

		if (resultCode != RESULT_OK) {
			Toast.makeText(this, "User denied screen sharing permission", Toast.LENGTH_SHORT).show();
			return;
		}
		Intent service = new Intent(this,ScreenMirrorService.class); 
		service.putExtra(ScreenMirrorService.CMD_NAME, ScreenMirrorService.CMD_MEDIAPROJECT);
		service.putExtra(Intent.EXTRA_INTENT, data);
		service.putExtra(ScreenMirrorService.RESULT_CODE, resultCode);
		startService(service);
		finish();
	}

}
