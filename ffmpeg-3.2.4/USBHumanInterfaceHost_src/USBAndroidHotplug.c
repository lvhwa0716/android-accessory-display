/*
 ============================================================================
 Name        : USBAndroidHotplug.c
 Author      : lvhwa0716@gmail.com
 Version     :
 Copyright   : 
 Description :  Ansi-style
 ============================================================================
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "USBAndroid.h"

static int LIBUSB_CALL hotplug_callback(libusb_context *ctx, libusb_device *dev,
		libusb_hotplug_event event, void *user_data) {
	uint16_t vid;
	uint16_t pid;
	int rc;
	struct libusb_device_descriptor desc;
	_usbandroid_loginfo("    hotplug_callback \n");
	rc = libusb_get_device_descriptor(dev, &desc);
	if (LIBUSB_SUCCESS != rc) {
		_usbandroid_logerr( "Error getting device descriptor\n");
		return 0;
	}

	if ((desc.idVendor == _accessory_vid)
			&& (desc.idProduct == _accessory_pid)) {

		int rc = MessageNotify_USBAccAttach(_accessory_vid, _accessory_pid);
		_usbandroid_loginfo(
				"    USB Accessory Detect , post this message to data dispatch :%d \n",
				rc);

		return 0;
	}
	if (0 == USBAndroidDevice_isSupport(dev, &vid, &pid)) {
		_usbandroid_loginfo("    Found it \n");
		USBAndroidDevice_enterUsbAcc(NULL, vid, pid, 0);
	} else {
		_usbandroid_loginfo("    Not Support !! \n");
	}

	return 0;
}

static int LIBUSB_CALL hotplug_callback_detach(libusb_context *ctx,
		libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	_usbandroid_loginfo("    hotplug_callback_detach \n");
	struct libusb_device_descriptor desc;
	int rc;
	rc = libusb_get_device_descriptor(dev, &desc);
	if (LIBUSB_SUCCESS != rc) {
		return 0;
	}
	_usbandroid_loginfo("Device Removed: %04x:%04x\n", desc.idVendor, desc.idProduct);
	if ((desc.idVendor == _accessory_vid)
			&& (desc.idProduct == _accessory_pid)) {
		_usbandroid_loginfo(" USB Accessory device removed");

	}
	return 0;
}
void *USBAndroidHotplug_thread_main(void *arg) {
	libusb_context *g_libusb_ctx = NULL;
	libusb_hotplug_callback_handle hp[2];
	int rc;
	int hotplug_ok = 0; // -1 : force use poll , window does not support hot plug
	int isInAccessoryMode = 0; // 0 : not detect , 1 : setup ok , wait , 2: ok

	// init hotplug
	rc = libusb_init(&g_libusb_ctx);
	if (rc < 0) {
		_usbandroid_logerr("failed to initialise libusb: %s\n", libusb_error_name(rc));
		return NULL;
	}

	_usbandroid_loginfo("Hotplug Context : %p\n", g_libusb_ctx);
	
	{
		// if alread connected
		uint16_t last_vid;
		uint16_t last_pid;
		if( 0 == USBAndroidDevice_checkExistByVidPid(g_libusb_ctx, _accessory_vid, _accessory_pid)) {
			USBAndroidDevice_resetUsbAcc(g_libusb_ctx);
		}else if( 0 == USBAndroidDevice_checkAll(g_libusb_ctx, &last_vid,&last_pid)) {
			USBAndroidDevice_enterUsbAcc(g_libusb_ctx, last_vid,	last_pid, 0);
			isInAccessoryMode = 1;
		}
	}

	if (!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
		_usbandroid_loginfo("Hotplug capabilites are not supported on this platform\n");
		hotplug_ok--;
	}
	if(hotplug_ok >= 0) {
		rc = libusb_hotplug_register_callback(g_libusb_ctx,
				LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0, -1, -1,
				LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &hp[0]);
		if (LIBUSB_SUCCESS != rc) {
			_usbandroid_logerr( "Error registering callback 0\n");
			hotplug_ok--;
		}
	}
	
	if(hotplug_ok >= 0) {
		rc = libusb_hotplug_register_callback(g_libusb_ctx,
				LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0, -1, -1,
				LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback_detach, NULL, &hp[1]);
		if (LIBUSB_SUCCESS != rc) {
			libusb_hotplug_deregister_callback(g_libusb_ctx,hp[0]);
			_usbandroid_logerr( "Error registering callback 1\n");
			hotplug_ok--;
		}
	}
	
	if(hotplug_ok >= 0) {


		while (1) {
			rc = libusb_handle_events(g_libusb_ctx);
			if (rc < 0) {
				_usbandroid_logerr("libusb_handle_events() failed: %s\n",
						libusb_error_name(rc));
				sleep(1);
			}
		}
	} else {
		// poll mode , windows shit
		
		int checkFlags = 0;
		int try_count = 0;
		//libusb_set_debug(g_libusb_ctx,LIBUSB_LOG_LEVEL_DEBUG);
		while(1) {
			uint16_t last_vid;
			uint16_t last_pid;
			
			if (isInAccessoryMode == 1 ) {
				
				if( 0 == USBAndroidDevice_checkExistByVidPid(g_libusb_ctx, _accessory_vid, _accessory_pid)) {
					_usbandroid_loginfo("in USB Accessory mode \n");
					isInAccessoryMode = 2;
					MessageNotify_USBAccAttach(_accessory_vid, _accessory_pid);
				} else if(0 == USBAndroidDevice_checkExistByVidPid(g_libusb_ctx, last_vid, last_pid)) {
					isInAccessoryMode = 0;
				} else {
					try_count++;
					if(try_count >= 5) {
						isInAccessoryMode = 0;
					}
				}
			} else if(isInAccessoryMode == 0 ){
				if( 0 == USBAndroidDevice_checkAll(g_libusb_ctx, &last_vid,&last_pid)) {
					USBAndroidDevice_enterUsbAcc(g_libusb_ctx, last_vid,	last_pid, 0);
					isInAccessoryMode = 1;
					try_count = 0;
				}
			} else {
				if( checkFlags == 0) {
					sleep(5);
					checkFlags = 1;
					continue;
				} else {
					checkFlags = 0;
					uint16_t vid;
					uint16_t pid;
					if(0 != USBAndroidDevice_checkAccExist(g_libusb_ctx, &vid, &pid)) {
						isInAccessoryMode = 0;
					}
				}
			}
			sleep(1);
			
		}
		
	}


	libusb_exit(g_libusb_ctx);
	_usbandroid_loginfo("libusb_handle_events() closed\n");
	return NULL;
}
