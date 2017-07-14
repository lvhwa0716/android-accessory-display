package com.lvh.usbhumaninterface;

import android.app.Application;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.media.projection.MediaProjection;
import android.os.Handler;

import java.nio.ByteBuffer;

public class MainApplication extends Application {
	private final static String TAG = "MainApplication";

	private USBVirtualDisplay mUSBVirtualDisplay = null;
	private UsbMessageDispatch mUsbMessageDispatch = null;
	private int mScreen_Width;
	private int mScreen_Height;
	
	private static Object mNotifySync = new Object();
	private Protocol.Callback mUiNotifyCb = null;

	private Handler mHandler = new Handler();

	@Override
	public void onCreate() {
		super.onCreate();
		IntentFilter filter = new IntentFilter();
		filter.addAction(Protocol.ACTION_USB_PERMISSION);
		filter.addAction(UsbManager.ACTION_USB_ACCESSORY_ATTACHED);
		filter.addAction(UsbManager.ACTION_USB_ACCESSORY_DETACHED);
		filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
		filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);

		registerReceiver(usbDeviceReceiver, filter);

	}

	@Override
	public void onTerminate() {
		super.onTerminate();
		unregisterReceiver(usbDeviceReceiver);
	}

	public void setUpVirtualDisplay(MediaProjection mpj, int style, Protocol.EncodecParameter p) {

		
		mUSBVirtualDisplay = USBVirtualDisplay.getInstance(this);
		mUSBVirtualDisplay.setDevice(mpj, style, p);
		mUSBVirtualDisplay.start();
		mHandler.postDelayed(new Runnable() {
			@Override
			public void run() {
				if (isDisplayActive()) {
					Logger.logDebug(TAG, "Virtual Setup OK");
					NotifyUI(Protocol.Internal.ID, Protocol.Internal.MSG_DISPLAY_OK, null);
				} else {
					Logger.logDebug(TAG, "Virtual Setup Failed");
					NotifyUI(Protocol.Internal.ID, Protocol.Internal.MSG_DISPLAY_FAIL, null);
				}
			}
		}, 2000);

	}

	public void tearDownVirtualDisplay() {
		Logger.logDebug(TAG, " Stop Virtual Display");
		if (mUSBVirtualDisplay != null) {
			mUSBVirtualDisplay.quit();
		}
		mUSBVirtualDisplay = null;
	}

	public boolean setUpUsbTransport(UsbManager usbMg, UsbAccessory accessory, int screen_w, int screen_h) {
		Logger.logDebug(TAG, "Setting up a Usb: ");
		if (UsbMessageDispatch.isActive()) {
			Logger.logDebug(TAG, "Already Done ");
			return true;
		}
		mUsbMessageDispatch = UsbMessageDispatch.getInstance(this);
		mScreen_Width = screen_w;
		mScreen_Height = screen_h;
		if (mUsbMessageDispatch.setDevice(usbMg, accessory) == true) {
			mUsbMessageDispatch.start();
			final int w = mScreen_Width;
			final int h = mScreen_Height;
			
			mHandler.postDelayed(new Runnable() {
				@Override
				public void run() {
					Logger.logDebug(TAG, "Query , UsbMessageDispatch.isActive()  " + UsbMessageDispatch.isActive());
					if (UsbMessageDispatch.isActive()) {
						// send screen w,h to target
						ByteBuffer mBuffer = ByteBuffer.allocate(8);
						mBuffer.clear();
		                mBuffer.putInt(w);
		                mBuffer.putInt(h);
		                mBuffer.flip();
						sendMessage(Protocol.MediaSource.ID, Protocol.MediaSource.MSG_QUERY, mBuffer);
					}
				}
			}, 1000);

			return true;
		} else {
			mUsbMessageDispatch.quit();
			mUsbMessageDispatch = null;
		}
		return false;

	}

	public void tearDownUsbTransport(UsbAccessory accessory) {
		// also stop virtual display capture
		tearDownVirtualDisplay();

		Logger.logDebug(TAG, "down Usb: ");
		if (mUsbMessageDispatch != null) {
			mUsbMessageDispatch.removeDevice(accessory);
		}
		mUsbMessageDispatch = null;

	}

	public boolean isDisplayActive() {
		return USBVirtualDisplay.isActive();
	}

	public void sendMessage(int service, int what, ByteBuffer content) {
		if (mUsbMessageDispatch != null)
			mUsbMessageDispatch.sendMessage(service, what, content);
	}

	public void registerUINotify(Protocol.Callback cb) {
		synchronized (mNotifySync) {
			mUiNotifyCb = cb;
		}
	}

	public void NotifyUI(int service, int what, ByteBuffer content) {
		if (Protocol.Internal.ID == service) {
			if (what < Protocol.Internal.DEVICE_ERROR_LIMIT) {
				tearDownUsbTransport(null);
			}
		}
		synchronized (mNotifySync) {
			Logger.logInfo(TAG, "id="+ service + " what=" + what );
			if (mUiNotifyCb != null) {
				mUiNotifyCb.onMessageReceived(service, what, content);
			}
		}
	}

	private void UsbDisconnect(UsbAccessory accessory) {
		tearDownUsbTransport(accessory);
		NotifyUI(Protocol.Internal.ID, Protocol.Internal.MSG_DISCONNECT, null);
	}

	private final BroadcastReceiver usbDeviceReceiver = new BroadcastReceiver() {

		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();
			Logger.logDebug(TAG, "usbDeviceReceiver : " + action);
			if (Protocol.ACTION_USB_PERMISSION.equals(action)) {
				int msg = 0;
				if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
					// permission OK
					Logger.logInfo(TAG, "Usb Permission agranted");
					msg = Protocol.Internal.MSG_PERMISSION_AGRANTED;
				} else {
					// permission denied
					Logger.logInfo(TAG, "Usb Permission denied");
					msg = Protocol.Internal.MSG_PERMISSION_DENIED;
				}
				final int _msg = msg;
				mHandler.post(new Runnable() {
					@Override
					public void run() {
						NotifyUI(Protocol.Internal.ID, _msg, null);

					}
				});

			} else if (UsbManager.ACTION_USB_ACCESSORY_DETACHED.equals(action)) {
				final UsbAccessory accessory = (UsbAccessory) intent.getParcelableExtra(UsbManager.EXTRA_ACCESSORY);
				;
				mHandler.post(new Runnable() {
					@Override
					public void run() {
						UsbDisconnect(accessory);
						Logger.logDebug(TAG, "ACTION_USB_ACCESSORY_DETACHED done");
					}
				});
			}
		}
	};
}
