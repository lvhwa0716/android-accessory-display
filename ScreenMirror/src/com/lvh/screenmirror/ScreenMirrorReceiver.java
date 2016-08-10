package com.lvh.screenmirror;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.util.Log;

public class ScreenMirrorReceiver extends BroadcastReceiver {
	private static final String TAG = "ScreenMirrorReciver";
	
	public static final String ACTION_USB_ACCESSORY_PERMISSION =
            "com.android.accessorydisplay.source.ACTION_USB_ACCESSORY_PERMISSION";
	
    public ScreenMirrorReceiver() {
    }
    
	@Override
	public void onReceive(Context context, Intent intent) {
		Log.d(TAG,"onReceive: " + intent);
		UsbAccessory accessory = intent.<UsbAccessory> getParcelableExtra(UsbManager.EXTRA_ACCESSORY);
		if (accessory != null) {
			String action = intent.getAction();
			if (action.equals(UsbManager.ACTION_USB_ACCESSORY_ATTACHED)) {
				onAccessoryAttached(context,accessory);
			} else if (action.equals(UsbManager.ACTION_USB_ACCESSORY_DETACHED)) {
				onAccessoryDetached(context,accessory);
			} else if (action.equals(ACTION_USB_ACCESSORY_PERMISSION)) {
				if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
					Log.d(TAG,"Accessory permission granted: " + accessory);
					onAccessoryAttached(context,accessory);
				} else {
					Log.e(TAG,"Accessory permission denied: " + accessory);
				}
			}
		}
	}
	
	private void onAccessoryAttached(Context context,UsbAccessory accessory) {
        Log.d(TAG,"USB accessory attached: " + accessory);
        Intent service = new Intent(context,ScreenMirrorService.class);  
        service.putExtra(UsbManager.EXTRA_ACCESSORY, accessory);
        service.putExtra(ScreenMirrorService.CMD_NAME, ScreenMirrorService.CMD_ATTACHED);
        context.startService(service);
    }

    private void onAccessoryDetached(Context context,UsbAccessory accessory) {
    	Log.d(TAG,"USB accessory detached: " + accessory);
    	Intent service = new Intent(context,ScreenMirrorService.class); 
    	service.putExtra(UsbManager.EXTRA_ACCESSORY, accessory);
    	service.putExtra(ScreenMirrorService.CMD_NAME, ScreenMirrorService.CMD_DETACHED);
        context.startService(service);
    }
    
}
