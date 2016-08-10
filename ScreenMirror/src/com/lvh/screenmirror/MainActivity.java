package com.lvh.screenmirror;

import com.android.accessorydisplay.common.Logger;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity {
	private static final String TAG = "ScreenMirror-MainActivity";
	private TextView mLogTextView;
	private Logger mLogger;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		mLogTextView = (TextView) findViewById(R.id.logTextView);
        mLogTextView.setMovementMethod(ScrollingMovementMethod.getInstance());
        mLogger = new TextLogger();
        mLogger.log( "onCreate: " + getIntent());
		processIntent(getIntent());

	}

	@Override
	protected void onNewIntent(Intent intent) {
		super.onNewIntent(intent);
		mLogger.log( "onNewIntent: " + intent);
		setIntent(intent);
		processIntent(intent);

	}
	@Override
	public void onActivityResult(int requestCode, int resultCode, Intent data) {

		mLogger.log( "resultCode : " + resultCode + " request code: " + requestCode);
		if (requestCode != PERMISSION_CODE) {
			mLogger.log( "Unknown request code: " + requestCode);
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
	}
	private static final int PERMISSION_CODE = 0x1234;
	private void processIntent(Intent intent) {
		UsbManager mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
		if (intent.getAction().equals(ScreenMirrorService.ACTION_NAME)) {
			Intent prj_intent = intent.<Intent> getParcelableExtra(Intent.EXTRA_INTENT);
			mLogger.log( "Intent:" + prj_intent);
			startActivityForResult(prj_intent,PERMISSION_CODE);
		}
		else if (intent.getAction().equals(UsbManager.ACTION_USB_ACCESSORY_ATTACHED)) {
			UsbAccessory accessory = (UsbAccessory) intent
					.getParcelableExtra(UsbManager.EXTRA_ACCESSORY);
			if (accessory != null) {
				onAccessoryAttached(accessory);
			}
		} else {
			UsbAccessory[] accessories = mUsbManager.getAccessoryList();
			if (accessories != null) {
				for (UsbAccessory accessory : accessories) {
					onAccessoryAttached(accessory);
				}
			}
		}
	}
	private void onAccessoryAttached(UsbAccessory accessory) {
		mLogger.log("USB accessory attached: " + accessory);
		Intent service = new Intent(this, ScreenMirrorService.class);
		service.putExtra(UsbManager.EXTRA_ACCESSORY, accessory);
		service.putExtra(ScreenMirrorService.CMD_NAME,
				ScreenMirrorService.CMD_ATTACHED);
		startService(service);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		int id = item.getItemId();
		if (id == R.id.action_settings) {
			return true;
		}
		return super.onOptionsItemSelected(item);
	}
    class TextLogger extends Logger {
        @Override
        public void log(final String message) {
            Log.d(TAG, message);

            mLogTextView.post(new Runnable() {
                @Override
                public void run() {
                    mLogTextView.append(message);
                    mLogTextView.append("\n");
                }
            });
        }
    }
}
