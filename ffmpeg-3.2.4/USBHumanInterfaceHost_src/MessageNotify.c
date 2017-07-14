/*
 ============================================================================
 Name        : MessageNotify.c
 Author      : lvhwa0716@gmail.com
 Version     :
 Copyright   : 
 Description :  Ansi-style
 ============================================================================
 */

#include<unistd.h>
#include <sys/types.h>
#include <string.h>
#if defined(_WIN32)
	#include <winsock2.h>
	#include <windows.h>
	#include <ws2tcpip.h>
#else
	#include <sys/ipc.h>
	#include <sys/msg.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "USBAndroid.h"

#if defined(_WIN32)
	#define MTYPE_BASE		WM_USER
#else
	#define MTYPE_BASE		0x400
#endif 

#define MTYPE_ACCESSORY (MTYPE_BASE + 1)
#define MTYPE_DATA (MTYPE_BASE + 2)

struct _vid_pid {
	uint16_t vid;
	uint16_t pid;
};

struct _buffer_size {
	void*  pBuf;
	int size;
};

struct _msg_data {
	long int mtype; // msg reserved
	union {
		struct _vid_pid _vp;
		struct _buffer_size _buf;
	};
};
#define _msg_data_len (sizeof(struct _msg_data) - sizeof(long int))

static libusb_device_handle* owner_handle = NULL;
static pthread_t mDeviceThread = (pthread_t)(0);
static pthread_t mScreenThread = (pthread_t)(0);

static int id_MessageNotify_USBEvent = -1; // hotplug => device
static int id_MessageNotify_DataEvent = -1; // device => screen
static int pipe_fd[2]; // media stream

#if defined(H264_MULTICAST_UDP_ADDR)
	static int multicast_socket = 0;
	static struct sockaddr_in multicast_addr;
	static int multicast_addrlen = sizeof(multicast_addr);
#endif

#if defined(_WIN32)
	/*
	 * 0 = Success
	 * */
	int MessageNotify_USBAccAttach(uint16_t vid, uint16_t pid) {
		void *hnd = pthread_gethandle (mDeviceThread);
		if( hnd != NULL ) {
			DWORD idThread = GetThreadId(hnd);
			BOOL result = PostThreadMessage(idThread,
								MTYPE_ACCESSORY,
								vid,
								pid
							);
			_usbandroid_loginfo("PostThreadMessage : %d" , result);
		}
		_usbandroid_loginfo("GetLastError : %d, Handle : %p" , GetLastError(), hnd);
		return 0;
	}

	/*
	 * 0 = Success
	 * */
	int MessageNotify_WaitUSBAccAttach(uint16_t *vid, uint16_t *pid) {
		MSG msg;
		_usbandroid_loginfo("MessageNotify_WaitUSBAccAttach : %d \n " , GetLastError());
		if(GetMessage(&msg,0,0,0) > 0 ) {
			switch(msg.message) {
				case MTYPE_ACCESSORY:
					*vid = (uint16_t)msg.wParam;
					*pid = (uint16_t)msg.lParam;
					_usbandroid_loginfo("MessageNotify_WaitUSBAccAttach : OK \n" );
					return 0;
				default:
					break;
			}
			
		}
		_usbandroid_logerr("MessageNotify_WaitUSBAccAttach : error \n" );
		return -1;
	}

	int MessageNotify_PostData(void *p, int size) {
		void *hnd = pthread_gethandle (mScreenThread);
		if( hnd != NULL ) {
			DWORD idThread = GetThreadId(hnd);
			BOOL result = PostThreadMessage(idThread,
								MTYPE_DATA,
								size,
								p
							);
		}
		return 0;
	}
	int MessageNotify_getData(unsigned char **p, int *size) {
		MSG msg;
		if(GetMessage(&msg,0,0,0) > 0 ) {
			switch(msg.message) {
				case MTYPE_ACCESSORY:
					*size = (int)msg.wParam;
					*p = (void*)msg.lParam;
					return 0;
				default:
					break;
			}
			
		}
		_usbandroid_logerr("MessageNotify_getData : error \n" );
		return -1;
	}
#else
	// Linux

	/*
	 * 0 = Success
	 * */
	int MessageNotify_USBAccAttach(uint16_t vid, uint16_t pid) {
		struct _msg_data _msg_vp;
		_msg_vp.mtype = MTYPE_ACCESSORY;
		_msg_vp._vp.vid = vid;
		_msg_vp._vp.pid = pid;
		return msgsnd(id_MessageNotify_USBEvent, &_msg_vp, _msg_data_len, 0);
	}

	/*
	 * 0 = Success
	 * */
	int MessageNotify_WaitUSBAccAttach(uint16_t *vid, uint16_t *pid) {
		struct _msg_data _msg_vp;
		int result;
		result = msgrcv(id_MessageNotify_USBEvent, &_msg_vp,
		_msg_data_len, MTYPE_ACCESSORY, 0);
		_usbandroid_loginfo("mtype = %ld , %04x,%04x, result = %d\n", _msg_vp.mtype,
				_msg_vp._vp.vid, _msg_vp._vp.pid, result);
		*vid = _msg_vp._vp.vid;
		*pid = _msg_vp._vp.pid;
		return result > 0 ? 0 : (-1);
	}

	int MessageNotify_PostData(void *p, int size) {
		struct _msg_data _msg_data;
		_msg_data.mtype = MTYPE_DATA;
		_msg_data._buf.pBuf = p;
		_msg_data._buf.size = size;
		return msgsnd(id_MessageNotify_DataEvent, &_msg_data, _msg_data_len, 0);
	}
	int MessageNotify_getData(unsigned char **p, int *size) {
		struct _msg_data _msg_data;
		int result;
		result = msgrcv(id_MessageNotify_DataEvent, &_msg_data,_msg_data_len, MTYPE_DATA, 0);

		*p = _msg_data._buf.pBuf;
		*size = _msg_data._buf.size;
		return result > 0 ? 0 : (-1);
	}
#endif 


void MessageNotify_Init() {
#if !defined(_WIN32)
	id_MessageNotify_USBEvent = msgget(ftok(".", 1),IPC_CREAT | 0644);
	id_MessageNotify_DataEvent = msgget(ftok(".", 2),IPC_CREAT | 0644);
#endif

	_usbandroid_loginfo("id_MessageNotify_USBEvent=%d\n", id_MessageNotify_USBEvent);
	#if defined(H264_MULTICAST_UDP_ADDR) 
	{
		#if defined(_WIN32)
		{
			WSADATA wsaData;
			if( WSAStartup(MAKEWORD(2,2),&wsaData) != 0){
				_usbandroid_logerr(" WSAStartup: error %d \n" , GetLastError()); 
				return;
			}
		}
		#endif
		multicast_socket = socket(AF_INET, SOCK_DGRAM, 0);
		if (multicast_socket < 0) {
			_usbandroid_logerr(" socket create: error \n"); 
			return;
		}
		memset((char *)&multicast_addr, 0, sizeof(multicast_addr));
		multicast_addr.sin_family = AF_INET;
		multicast_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		multicast_addr.sin_port = htons(H264_MULTICAST_UDP_PORT);
		multicast_addr.sin_addr.s_addr = inet_addr(H264_MULTICAST_UDP_ADDR);
		
		int reuse = 1;
		if (setsockopt (multicast_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0) {
			_usbandroid_logerr("setsockopt: SO_REUSEADDR error \n"); 
		}


		/*set TTL, default is 1*/
		int ttl = H264_MULTICAST_TTL;
		if (setsockopt(multicast_socket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl))) {
			_usbandroid_logerr("setsockopt:IP_MULTICAST_TTL error\n"); 
		}
	}
	#else
		pipe(pipe_fd);
	#endif
	
}
int MessageNotify_writeStream(void* pBuf, int size) {
	#if defined(H264_MULTICAST_UDP_ADDR)
		int sent_size = 0;
		while(sent_size < size) {
			int bulk_size = (size - sent_size) > H264_MULTICAST_UDP_SIZE ? H264_MULTICAST_UDP_SIZE : (size -sent_size);
			int ret = sendto(multicast_socket, pBuf+sent_size, bulk_size, 0,
						(struct sockaddr *) &multicast_addr, multicast_addrlen);
			if (ret < 0) {
				_usbandroid_logerr("sendto error : %d\n", ret);
				return ret;
			}
			sent_size = sent_size + ret;
		}
		return 0;
	#else
		return write(pipe_fd[1], pBuf, size);
	#endif
}

int MessageNotify_getReadFd(void) {
	#if defined(H264_MULTICAST_UDP_ADDR)
		// not used in this mode
		return 0;
	#else
		return pipe_fd[0];
	#endif
}

void MessageNotify_setContext(libusb_device_handle* _handle) {
	owner_handle = _handle;
}

void MessageNotify_setThread(pthread_t device, pthread_t screen) {
	mDeviceThread = device;
	mScreenThread = screen;
}

int MessageNotify_reportHID(struct _hid_data * d) {
	if (owner_handle != NULL) {
		USBAndroidHID_reportHIDEvent(owner_handle, d);
		return 0;
	} else {
		return -1;
	}
}

int MessageNotify_getHID(struct _hid_data * d) {
	return -1;
}
