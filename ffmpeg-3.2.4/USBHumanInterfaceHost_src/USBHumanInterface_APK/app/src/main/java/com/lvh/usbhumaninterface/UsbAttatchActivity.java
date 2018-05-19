package com.lvh.usbhumaninterface;

import java.nio.ByteBuffer;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.hardware.display.DisplayManager;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.DisplayMetrics;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.view.View;

public class UsbAttatchActivity extends Activity implements View.OnClickListener {
	private final static String TAG = "UsbAttatchActivity";
	private static final int REQUEST_MEDIA_PROJECTION = 1;

	private Handler mHandler = new Handler();

	private MediaProjectionManager mMediaProjectionManager = null;
	private UsbManager mUsbManager = null;
	private UsbAccessory mUsbAccessory = null;
	private MediaProjection mMediaProjection = null;

	private int mResultCode = 0;
	private Intent mResultData = null;
	private int mScreenDensity;
	private int mScreen_Width;
	private int mScreen_Height;

	private static final int STYLE_MIRROR = DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR;
	private static final int STYLE_PRESENTAION = DisplayManager.VIRTUAL_DISPLAY_FLAG_PRESENTATION
			| DisplayManager.VIRTUAL_DISPLAY_FLAG_PUBLIC;
	private int mStyle = STYLE_PRESENTAION;

	private final static int STATUS_DISABLE = 0; // 0: device not connect
	private final static int STATUS_CONNECTED = 1; // 1: connect , can start
													// display
	private final static int STATUS_RENDERING = 2; // 2: output for rendering
	private final static int STATUS_FAILED_TRY_AGAIN = 3;
	private int mStatus = STATUS_DISABLE;
	// start

	private TextView mInfoView = null;
	private Button mButton = null;

	private Protocol.EncodecParameter mEncodec = Protocol.H264Profile.get(1);

	private MainApplication getApp() {
		return ((MainApplication) getApplicationContext());
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Logger.logDebug(TAG, "onCreate ");
		setContentView(R.layout.activity_usb_attatch);
		mInfoView = (TextView) findViewById(R.id.infoTextView);
		mButton = (Button) findViewById(R.id.actionButton);
		mButton.setOnClickListener(this);

		// Style
		RadioGroup group = (RadioGroup) this.findViewById(R.id.styleRadioGroup);
		if(mStyle == STYLE_MIRROR)
			group.check(R.id.styleRadio_mirror);
		else
			group.check(R.id.styleRadio_extend);
		group.setOnCheckedChangeListener(mRadioGroupCheckedListener);

		// Resolution
		group = (RadioGroup) this.findViewById(R.id.h264RadioGroup);
		if(mEncodec.equals(Protocol.H264Profile.get(0)))
			group.check(R.id.h264Radio_480p);
		else if(mEncodec.equals(Protocol.H264Profile.get(1)))
			group.check(R.id.h264Radio_720p);
		else
			group.check(R.id.h264Radio_1080p);
		group.setOnCheckedChangeListener(mRadioGroupCheckedListener);

		Logger.logDebug(TAG, "onCreate: " + getIntent());

		DisplayMetrics metrics = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(metrics);
		mScreenDensity = metrics.densityDpi;
		mScreen_Width = metrics.widthPixels;
		mScreen_Height = metrics.heightPixels;
		Logger.logDebug(TAG, "metrics: " + metrics.toString());
		
		mMediaProjectionManager = (MediaProjectionManager) getSystemService(Context.MEDIA_PROJECTION_SERVICE);
		mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);

		processIntent(getIntent());
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		Logger.logDebug(TAG, "onConfigurationChanged: " + newConfig.toString());
	}

	@Override
	protected void onNewIntent(Intent intent) {
		super.onNewIntent(intent);
		Logger.logDebug(TAG, "onNewIntent: " + intent);
		processIntent(intent);

	}

	@Override
	protected void onResume() {
		super.onResume();
		getApp().registerUINotify(new UsbAttachCallback());
	}

	@Override
	protected void onPause() {
		super.onPause();
		getApp().registerUINotify(null);
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		Logger.logDebug(TAG, "onDestroy ");

	}

	private void setActionButtonMode(int mode) {
		mStatus = mode;
		switch (mode) {
		case STATUS_CONNECTED:
			mButton.setEnabled(true);
			mInfoView.setText(R.string.info_connected);
			return;
		case STATUS_RENDERING:
			mButton.setEnabled(false);
			mInfoView.setText(R.string.info_rendering);
			return;
		case STATUS_DISABLE:
			mInfoView.setText(R.string.info_connecting);
			break;
		case STATUS_FAILED_TRY_AGAIN:
			mInfoView.setText(R.string.info_tryagain);
			break;
		default:
			mInfoView.setText(R.string.info_unknown_error);
			break;
		}
		mStatus = 0;
		mButton.setEnabled(false);
	}

	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		switch (v.getId()) {
		case R.id.actionButton:
			switch (mStatus) {
			case STATUS_CONNECTED: // connect
				// mEncodec = Protocol.H264Profile.get(0);
				if (!mUsbManager.hasPermission(mUsbAccessory)) {
					Toast.makeText(this, "Permission Denied", Toast.LENGTH_SHORT).show();
					return;
				}

				if (mMediaProjection == null) {
					Logger.logDebug(TAG, "Requesting confirmation");
					startActivityForResult(mMediaProjectionManager.createScreenCaptureIntent(),
							REQUEST_MEDIA_PROJECTION);
				} else {
					getApp().setUpVirtualDisplay(mMediaProjection, mStyle, mEncodec);
				}
				break;

			case STATUS_RENDERING: // transfer
				getApp().tearDownVirtualDisplay();
				tearDownMediaProject();
				break;
			case STATUS_DISABLE:// not connect
			default:
				break;
			}
			break;
		}
	}

	@Override
	public void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (requestCode == REQUEST_MEDIA_PROJECTION) {
			if (resultCode != Activity.RESULT_OK) {
				Logger.logInfo(TAG, "User cancelled");
				Toast.makeText(this, "User denied capture screen", Toast.LENGTH_SHORT).show();
				return;
			}

			Logger.logInfo(TAG, "Starting screen capture");
			mResultCode = resultCode;
			mResultData = data;
			mMediaProjection = mMediaProjectionManager.getMediaProjection(mResultCode, mResultData);
			mMediaProjection.registerCallback(mMediaProjectionCallback, null);
			getApp().setUpVirtualDisplay(mMediaProjection, mStyle, mEncodec);
		}
	}

	private void processIntent(Intent intent) {
		setActionButtonMode(STATUS_DISABLE);
		if (getApp().isDisplayActive()) {
			setActionButtonMode(STATUS_RENDERING);
		}
		if (intent.getAction().equals(UsbManager.ACTION_USB_ACCESSORY_ATTACHED)) {
			UsbAccessory accessory = (UsbAccessory) intent.getParcelableExtra(UsbManager.EXTRA_ACCESSORY);
			if (accessory != null) {
				mUsbAccessory = accessory;
				if (!mUsbManager.hasPermission(accessory)) {
					Logger.logDebug(TAG, "Prompting the user for access to the accessory.");
					Intent _intent = new Intent(Protocol.ACTION_USB_PERMISSION);
					intent.setPackage(getPackageName());
					PendingIntent pendingIntent = PendingIntent.getBroadcast(this, 0, _intent,
							PendingIntent.FLAG_ONE_SHOT);
					mUsbManager.requestPermission(accessory, pendingIntent);
				} else {
					getApp().setUpUsbTransport(mUsbManager, mUsbAccessory,mScreen_Width, mScreen_Height);
				}

			}
		} else if (intent.getAction().equals(UsbManager.ACTION_USB_ACCESSORY_DETACHED)) {
			UsbAccessory accessory = (UsbAccessory) intent.getParcelableExtra(UsbManager.EXTRA_ACCESSORY);
			if (accessory != null) {
				getApp().tearDownUsbTransport(accessory);
				mUsbAccessory = null;
			}
		}
	}

	private final MediaProjection.Callback mMediaProjectionCallback = new MediaProjection.Callback() {
		@Override
		public void onStop() {
			mHandler.post(new Runnable() {
				@Override
				public void run() {
					if (mMediaProjection != null) {
						mMediaProjection.unregisterCallback(mMediaProjectionCallback);
						mMediaProjection = null;
					}
					Logger.logDebug(TAG, "MediaProjection Destroy");
				}
			});

		}
	};

	private final RadioGroup.OnCheckedChangeListener mRadioGroupCheckedListener = new RadioGroup.OnCheckedChangeListener() {

		@Override
		public void onCheckedChanged(RadioGroup arg0, int arg1) {

			switch (arg0.getCheckedRadioButtonId()) {
			case R.id.styleRadio_mirror:
				mStyle = STYLE_MIRROR;
				break;
			case R.id.styleRadio_extend:
				mStyle = STYLE_PRESENTAION;
				break;
			case R.id.h264Radio_480p:
				mEncodec = Protocol.H264Profile.get(0);
				break;
			case R.id.h264Radio_720p:
				mEncodec = Protocol.H264Profile.get(1);
				break;
			case R.id.h264Radio_1080p:
				mEncodec = Protocol.H264Profile.get(2);
				break;
			default:
				break;
			}
		}
	};

	private void handleAvailable() {
		setActionButtonMode(STATUS_CONNECTED);
	}

	private void handleNotAvailable() {
		setActionButtonMode(STATUS_DISABLE);
	}
	
	private void tearDownMediaProject() {
		Logger.logDebug(TAG, "MediaProjection tear down");
		if (mMediaProjection != null) {
			mMediaProjection.unregisterCallback(mMediaProjectionCallback);
			mMediaProjection.stop();
			mMediaProjection = null;
		}

	}
	
	private class UsbAttachCallback implements Protocol.Callback {
		public UsbAttachCallback() {
		}

		@Override
		public void onMessageReceived(int service, int what, ByteBuffer content) {
			Logger.logDebug(TAG, "onMessageReceived :  " + "id=" + service + "  what=" + what);
			if (service == Protocol.Internal.ID) {
				switch (what) {
				case Protocol.Internal.MSG_DISPLAY_OK:
					setActionButtonMode(STATUS_RENDERING);
					break;
				case Protocol.Internal.MSG_DISPLAY_FAIL:
					setActionButtonMode(STATUS_FAILED_TRY_AGAIN);
					break;
				case Protocol.Internal.MSG_DISCONNECT:
					mHandler.post(new Runnable() {
						@Override
						public void run() {
							Logger.logDebug(TAG, "cable disconnect");
							tearDownMediaProject();
							// finish();
						}
					});
					break;
				case Protocol.Internal.MSG_PERMISSION_AGRANTED:
					getApp().setUpUsbTransport(mUsbManager, mUsbAccessory,mScreen_Width, mScreen_Height);
					break;
				case Protocol.Internal.MSG_PERMISSION_DENIED:

					break;
				}
			} else if (service == Protocol.Render.ID) {
				switch (what) {
				case Protocol.Render.MSG_SINK_AVAILABLE: {
					Logger.logInfo(TAG, "Received MSG_SINK_AVAILABLE");
					if (content.remaining() >= 12) {
						final int width = content.getInt();
						final int height = content.getInt();
						final int densityDpi = content.getInt();
						if (width >= 0 && width <= 4096 && height >= 0 && height <= 4096 && densityDpi >= 60
								&& densityDpi <= 640) {
							handleAvailable();
							return;
						}
					}
					break;
				}

				case Protocol.Render.MSG_SINK_NOT_AVAILABLE: {
					Logger.logInfo(TAG, "Received MSG_SINK_NOT_AVAILABLE");
					handleNotAvailable();
					break;
				}
				}
			}
		}

	}
}
