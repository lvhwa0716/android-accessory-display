/*
 ============================================================================
 Name        : USBAndroid.h
 Author      : lvhwa0716@gmail.com
 Version     :
 Copyright   : 
 Description :  Ansi-style
 ============================================================================
 */
#ifndef __USBAndroid_h__
#define __USBAndroid_h__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include "libusb-1.0/libusb.h"

#define SUPPORT_HID

/* 
*  224.0.2.0～238.255.255.255 临时组地址
*  239.0.0.0～239.255.255.255 本地组播地址
* un define MULTICAST_UDP_ADDR use pipe in linux
*/
#if defined(_WIN32)
	#define H264_MULTICAST_UDP_ADDR	"239.70.37.29"
	#define H264_MULTICAST_UDP_PORT	6482
	#define H264_MULTICAST_TIME_PERIOD 5   /* second */
	#define H264_MULTICAST_TTL 2
	#define H264_MULTICAST_UDP_SIZE	64000
#else
	// Linux use pipe better then multicast udp	
#endif

#define _accessory_pid 0x2d01
#define _accessory_vid 0x18d1

#define _debug_accessory_pid 0xff68
#define _debug_accessory_vid 0x2717

#define DEFAULT_WIDTH 720
#define DEFAULT_HEIGHT 1280
typedef struct {
	libusb_context *ctx;
	libusb_device_handle *handle;
	int interface;
	uint16_t vid;
	uint16_t pid;
} usbandroid_handle;

struct _hid_data {
	int type; // 1 = key , 2 = mouse
	int status; // 1 = down , 0 = up
	int code;   // key sym
	int16_t x;
	int16_t y;
	int16_t dx;
	int16_t dy;
	// display info
	int video_x;
	int video_y;
	int video_w;
	int video_h;
};
int USBAndroidDevice_isSupport(libusb_device *dev, uint16_t *pVid, uint16_t *pPid);
int USBAndroidDevice_checkAll(libusb_context *ctx,uint16_t *pVid, uint16_t *pPid);
void USBAndroidDevice_Dump(usbandroid_handle *handle);
int USBAndroidDevice_isValid(usbandroid_handle *handle);
usbandroid_handle USBAndroidDevice_openClaim(libusb_context *ctx,uint16_t vid, uint16_t pid,
		int interface);
void USBAndroidDevice_closeRelease(usbandroid_handle *handle);
int USBAndroidDevice_setupUSBAcc(usbandroid_handle *_uhandle);
int USBAndroidDevice_enterUsbAcc(libusb_context *ctx,uint16_t vid, uint16_t pid, int interface);
void USBAndroidDevice_resetUsbAcc(libusb_context *ctx);
int USBAndroidDevice_checkAccExist(libusb_context *ctx, uint16_t *pVid, uint16_t *pPid);
int USBAndroidDevice_checkExistByVidPid(libusb_context *ctx, uint16_t vid, uint16_t pid);
void* USBAndroidDevice_getPool(void);
void USBAndroidDevice_freePool(void *p);
int USBAndroidDevice_sendToPeer(unsigned char *p, int size , int needfree);
void* USBAndroidDevice_thread_main(void *arg);

void MessageNotify_Init(void);
int MessageNotify_USBAccAttach(uint16_t vid, uint16_t pid);
int MessageNotify_WaitUSBAccAttach(uint16_t *vid, uint16_t *pid);
int MessageNotify_writeStream(void* pBuf, int size) ;
int MessageNotify_getReadFd(void);
void MessageNotify_setContext(libusb_device_handle* _handle);
void MessageNotify_setThread(pthread_t hotplug, pthread_t screen);
int MessageNotify_reportHID(struct _hid_data * d);
int MessageNotify_getHID(struct _hid_data * d);
int MessageNotify_PostData(void *p, int size);
int MessageNotify_getData(unsigned char **p, int *size);


int USBAndroidKeymap_getKeyMap(int key);

void USBAndroidHID_registerHID(libusb_device_handle* handle);
void USBAndroidHID_reportHIDEvent(libusb_device_handle* handle, struct _hid_data *_hid);

void *USBAndroidHotplug_thread_main(void *arg);


void *USBAndroidScreen_thread_main(void *arg);
int USBAndroidScreen_getWidth();
int USBAndroidScreen_getHeight();

void *USBAndroidMediaPlay_thread_main(void *arg);


#define __LOG_LEVEL_NONE__ 	0
#define __LOG_LEVEL_ERROR__	1
#define __LOG_LEVEL_DEBUG__	2
#define __LOG_LEVEL_INFO__	3
void USBAndroid_log(int level, const char * funcion_name, const char *format, ...);
void USBAndroid_SetLogLevel(int level);

#define _usbandroid_logerr(... ) USBAndroid_log(__LOG_LEVEL_ERROR__, __FUNCTION__, __VA_ARGS__)
#define _usbandroid_logdbg(... ) USBAndroid_log(__LOG_LEVEL_DEBUG__, __FUNCTION__, __VA_ARGS__)
#define _usbandroid_loginfo(... ) USBAndroid_log(__LOG_LEVEL_INFO__, __FUNCTION__, __VA_ARGS__)

#endif // __USBAndroid_h__
