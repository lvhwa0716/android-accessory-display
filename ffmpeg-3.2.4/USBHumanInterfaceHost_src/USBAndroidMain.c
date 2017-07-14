/*
 ============================================================================
 Name        : USBAndroidMain.c
 Author      : lvhwa0716@gmail.com
 Version     :
 Copyright   : 
 Description :  Ansi-style
 ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>

#include "USBAndroid.h"
int main(int argc, char *argv[]);
int main(int argc, char *argv[]) {
	pthread_t _USBAndroidScreen_thread;
	pthread_t _USBAndroidDevice_thread;
	pthread_t _USBAndroidHotplug_thread;

	int rc;

	USBAndroid_SetLogLevel(3);// only for debug
	// init Message
	MessageNotify_Init();
	// init Usb Screen Thread
	rc = pthread_create(&_USBAndroidScreen_thread, NULL, USBAndroidScreen_thread_main, NULL);
	if (rc) {
		_usbandroid_logerr(  "creating USBAndroidScreen_thread_main event thread (%d)", rc);
		return -1;
	}

	// init Usb Accessory Thread
	rc = pthread_create(&_USBAndroidDevice_thread, NULL, USBAndroidDevice_thread_main, NULL);
	if (rc) {
		_usbandroid_logerr(  "creating USBAndroidScreen_thread_main event thread (%d)", rc);
		return -1;
	}

	// init Hotplug Thread
	rc = pthread_create(&_USBAndroidHotplug_thread, NULL, USBAndroidHotplug_thread_main, NULL);
	if (rc) {
		_usbandroid_logerr(  "creating _USBAndroidMediaPlay_thread event thread (%d)", rc);
		return -2;
	}



	MessageNotify_setThread(_USBAndroidDevice_thread, _USBAndroidScreen_thread);
	
	USBAndroidMediaPlay_thread_main(NULL) ;

	_usbandroid_loginfo("USBAndroidMain Exit\n");
	return EXIT_SUCCESS;

}
