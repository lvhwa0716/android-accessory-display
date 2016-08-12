package com.lvh.screenmirror;

import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Handler;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import com.android.accessorydisplay.common.EventNotify;
import com.android.accessorydisplay.common.Logger;
import com.android.accessorydisplay.source.DisplaySourceService;
import com.android.accessorydisplay.source.UsbAccessoryStreamTransport;

public class ScreenMirrorService extends Service {

	private static final String TAG = "ScreenMirrorService";
	public static final String ACTION_NAME = "com.lvh.screenmirror.ACTION_MEDIAPROJECT_PERMISSION";
	public static final String CMD_NAME = "commandType";
	public static final String RESULT_CODE = "resultCode";
	public static final int CMD_ATTACHED = 1;
	public static final int CMD_DETACHED = 2;
	public static final int CMD_MEDIAPROJECT = 3;

	private UsbAccessoryStreamTransport mTransport = null;
	private DisplaySourceService mDisplaySourceService = null;
	private UsbAccessory mAccessory = null;
	private UsbAccessory mLastAccessory = null;
	private boolean mConnected = false;
	private MediaProjection mMediaProjection = null;

	// onCreate init these
	private UsbManager mUsbManager = null;
	private MediaProjectionManager mMediaProjectionManager = null;
	private Logger mLogger = new ScreenMirrorServiceLogger();
	private Handler mHandler = new Handler();
	private EventNotify mEventNotify = new ServiceEventNotify();
	private static final String MANUFACTURER = "Android";
	private static final String MODEL = "Accessory Display";

	private MediaProjection.Callback mMediaProjectionCallback = new MediaProjection.Callback() {
		@Override
		public void onStop() {

			mHandler.post(new Runnable() {
				@Override
				public void run() {
					mMediaProjection = null;
					mLogger.logError("MediaProjection Destroy, reconnect");
					reConnect();
				}
			});
		}
	};

	private static boolean isSink(UsbAccessory accessory) {
		return MANUFACTURER.equals(accessory.getManufacturer()) && MODEL.equals(accessory.getModel());
	}

	private void release() {

		if (mConnected == true) {
			mLogger.log("ScreenMirrorService release");
			mConnected = false;
			mAccessory = null;
			if (mMediaProjection != null) {
				mMediaProjection.stop();
				mMediaProjection = null;
			}
			if (mDisplaySourceService != null) {
				mDisplaySourceService.stop();
				mDisplaySourceService = null;
			}
			if (mTransport != null) {
				mTransport.close();
				mTransport = null;
			}
		}
	}

	public void accessoryAttached(UsbAccessory accessory) {
		if (accessory == null)
			return;
		mLogger.log("Attached:" + accessory.toString());
		mLogger.log("mMediaProjection:" + mMediaProjection);

		if (mAccessory != null) {
			if (mAccessory.equals(accessory)) {
				mLogger.log("Attached duplicate message already done, ignore it");
				return;
			}
		}
		if (isSink(accessory)) {
			release();

			if (!mUsbManager.hasPermission(accessory)) {
				mLogger.log("Prompting the user for access to the accessory.");
				Intent intent = new Intent(ScreenMirrorReceiver.ACTION_USB_ACCESSORY_PERMISSION);
				intent.setPackage(getPackageName());
				PendingIntent pendingIntent = PendingIntent.getBroadcast(this, 0, intent, PendingIntent.FLAG_ONE_SHOT);
				mUsbManager.requestPermission(accessory, pendingIntent);
				return;
			}
			if (mMediaProjection == null) {

				Intent intent = new Intent(getBaseContext(), MainActivity.class);
				intent.setAction(ACTION_NAME);
				intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
				intent.putExtra(Intent.EXTRA_INTENT, mMediaProjectionManager.createScreenCaptureIntent());
				getApplication().startActivity(intent);
				return;
			}
			// Open the accessory.
			ParcelFileDescriptor fd;
			try {
				fd = mUsbManager.openAccessory(accessory);
				if (fd == null) {
					mLogger.logError("Could not obtain accessory connection.");
					return;
				}
			} catch (Exception e) {
				mLogger.log(e.toString());
				return;
			}

			// All set.
			mLogger.log("Connected.");
			mConnected = true;
			mAccessory = accessory;
			mTransport = new UsbAccessoryStreamTransport(mLogger, fd, mEventNotify);
			mDisplaySourceService = new DisplaySourceService(this, mTransport, mMediaProjection);
			mMediaProjection.registerCallback(mMediaProjectionCallback, null);
			mDisplaySourceService.start();
			mTransport.startReading();
		}
	}

	public void accessoryDetached(UsbAccessory accessory) {
		mLogger.log("Detached:" + accessory.toString());
		if (mConnected == false)
			return;

		if (mAccessory.equals(accessory)) {
			release();
		}

	}

	@Override
	public void onCreate() {
		super.onCreate();
		mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
		mMediaProjectionManager = (MediaProjectionManager) getSystemService(Context.MEDIA_PROJECTION_SERVICE);
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		if (intent != null) {
			final UsbAccessory accessory = intent.<UsbAccessory> getParcelableExtra(UsbManager.EXTRA_ACCESSORY);
			int cmd = intent.getIntExtra(CMD_NAME, -1);
			mLogger.log("onStartCommand : CMD = " + cmd);

			switch (cmd) {
			case CMD_ATTACHED: {
				mLastAccessory = accessory;
				mHandler.post(new Runnable() {
					@Override
					public void run() {
						accessoryAttached(accessory);
					}
				});
			}
				break;
			case CMD_DETACHED: {
				mHandler.post(new Runnable() {
					@Override
					public void run() {
						accessoryDetached(accessory);
					}
				});
			}
				break;
			case CMD_MEDIAPROJECT: {
				final Intent data = intent.<Intent> getParcelableExtra(Intent.EXTRA_INTENT);
				final int resultCode = intent.getIntExtra(ScreenMirrorService.RESULT_CODE, 0);
				if (data != null) {
					mHandler.post(new Runnable() {
						@Override
						public void run() {
							if (mMediaProjection == null) {
								mMediaProjection = mMediaProjectionManager.getMediaProjection(resultCode, data);
								accessoryAttached(mLastAccessory);
							} else {
								mLogger.log("onStartCommand : Ignore new MediaProject" );
							}
						}
					});
				}
			}
				break;
			default:
				break;
			}
		}
		return super.onStartCommand(intent, flags, startId);
	}

	@Override
	public IBinder onBind(Intent intent) {
		return null;
	}

	private void reConnect() {
		if (mConnected) {
			accessoryDetached(mAccessory);
			// search accessory
			UsbAccessory[] accessories = mUsbManager.getAccessoryList();
			if (accessories != null) {
				for (UsbAccessory accessory : accessories) {
					if (isSink(accessory))
						accessoryAttached(accessory);
				}
			}
		}
	}

	private class ServiceEventNotify extends EventNotify {
		public ServiceEventNotify() {

		}

		@Override
		public void eventIOError(int error) {
			mLogger.logError("eventIOError : " + error);
			mHandler.post(new Runnable() {
				@Override
				public void run() {
					mLogger.logError("eventIOError reconnect");
					if ((mConnected) && (mAccessory != null)) {
						accessoryDetached(mAccessory);
					}
				}
			});
		}
	}

	private static class ScreenMirrorServiceLogger extends Logger {
		@Override
		public void log(final String message) {
			Log.d(TAG, message);
		}
	}
}