/*
 ============================================================================
 Name        : USBAndroidScreen.c
 Author      : lvhwa0716@gmail.com
 Version     :
 Copyright   : 
 Description :  Ansi-style
 ============================================================================
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <arpa/inet.h>
#endif

#include "USBAndroid.h"

#define MAX_ENVELOPE_SIZE (2 * 1024 * 1024) // same as java
#define HEAD_SIZE	8

#define WAIT_TIME (500 * 1000000) // unit ns

// send to sink
#define	SINK_ID  1
#define	SINK_MSG_QUERY  1
#define	SINK_MSG_CONTENT  2 // Send MPEG2-TS H.264 encoded content.

// send to Source
#define	SOURCE_ID 2
#define	MSG_SINK_AVAILABLE 1
#define	MSG_SINK_NOT_AVAILABLE 2

static int screen_width = 0;
static int screen_height = 0;

static int getHead(unsigned char *pBuf, int BufSize, int *size, int *what) {
	if (BufSize >= HEAD_SIZE) {
		unsigned short _id = 0;
		unsigned short _what = 0;
		int _size = 0;
		memcpy(&_id, pBuf, 2);
		_id = ntohs(_id);
		memcpy(&_what, pBuf + 2, 2);
		_what = ntohs(_what);
		memcpy(&_size, pBuf + 4, 4);
		_size = htonl(_size);
		// check it 
		if(_size >= (MAX_ENVELOPE_SIZE)) {
			_usbandroid_logerr( "dataError , id:%d, what:%d , size=%d\n" , _id, _what, _size);
			return -2;
		} else if (_size > (BufSize - HEAD_SIZE)) {
			_usbandroid_loginfo("not enough data , id:%d, what:%d , size=%d\n" , _id, _what, _size);
			return -1;
		} else if (_size < 0) {
			_usbandroid_logerr( "dataError , id:%d, what:%d , size=%d\n" , _id, _what, _size);
			return -2;
		}
		*size = _size;
		*what = _what;
		if(_id < 0) {
			return -2;
		}
		return _id;
	}

	return -1;
}


/*
 *  !=0 must terminate loop
 * */

static int dispatchReceived(int id,	int what, unsigned char * p, int size) {
	int response;
	int transferred;
	//int _write_fd = MessageNotify_getWriteFd();
	_usbandroid_loginfo("id : %d, what : %d, size : %d\n", id, what, size);
	if (id == SINK_ID) {
		switch (what) {
		case SINK_MSG_QUERY: { // parameter
			union {
				uint32_t u32[5]; // head size = 8 , payload 12(width, height,dpi)
				uint8_t u8[20];
			} send;
			struct {
				int32_t w;
				int32_t h;
			} source_info;
			memset(&source_info, 0, sizeof(source_info));

			if (size >= 8) {
				memcpy(&source_info, p, sizeof(source_info));
			}
			screen_width = ntohl(source_info.w);
			screen_height = ntohl(source_info.h);
			if ((screen_width <= 0) || (screen_height <= 0)) {
				screen_width = 0;
				screen_height = 0;
			}
			_usbandroid_loginfo("screen_width : %d, screen_height : %d\n", screen_width,
					screen_height);

			uint16_t *p16 = (uint16_t*) &send.u8[0];
			p16[0] = htons(SOURCE_ID);
			p16[1] = htons(MSG_SINK_AVAILABLE);
			uint32_t *p32 = (uint32_t*) &send.u8[4];
			p32[0] = htonl(12);
			p32[1] = htonl(720); //width
			p32[2] = htonl(1280); //height
			p32[3] = htonl(96); //dpi
			unsigned char *transfer_buf = (unsigned char*) malloc(sizeof(send));
			memcpy(transfer_buf, &send, sizeof(send));
			USBAndroidDevice_sendToPeer(transfer_buf, sizeof(send) , 1);
			

		}
			break;
		case SINK_MSG_CONTENT: // Media Stream;
			//write(_write_fd, p, size);
			MessageNotify_writeStream((void*)p, size) ;
			break;
		default:
			_usbandroid_logerr( " USBAndroidScreen  ERROR : what = %d\n", what);
			return -2;
			break;
		}
	} else {
		_usbandroid_logerr( " USBAndroidScreen  ERROR : service = %d\n", id);
		return -1;
	}
	return 0;
}

static unsigned char receive_buffer[MAX_ENVELOPE_SIZE + (16384 * 2)]; // BULK_SIZE 
#define receiver_buffer_size sizeof(receive_buffer)

void *USBAndroidScreen_thread_main(void *arg) {
	int video_in_end_position = 0;

	while (1) {
		unsigned char *pBuf;
		int size;
		int pkg_id = 0;
		int pkg_size = 0;
		int pkg_what = 0;
		unsigned char *p;
		if (0 != MessageNotify_getData(&pBuf, &size)) {
			//sleep(1);
			continue;
		}
		_usbandroid_loginfo("size = %d\n", size);

		memcpy(receive_buffer + video_in_end_position, pBuf, size);
		USBAndroidDevice_freePool(pBuf);
		video_in_end_position = video_in_end_position + size;
		p = receive_buffer;

		int head = 0;
		do {
			pkg_id = getHead(p + head, video_in_end_position - head, &pkg_size, &pkg_what);
			switch(pkg_id) {
				case -1:
					// need more data
					if(head != 0) {
						if(video_in_end_position > head) {
							memmove(receive_buffer, receive_buffer + head,
								video_in_end_position - head);
							video_in_end_position = video_in_end_position - head;
						} else {
							video_in_end_position = 0;
						}
						
					}
					break;
				case -2: // discard it
					video_in_end_position = 0;
					break;
				default: 
					//
					if (0
							== dispatchReceived(pkg_id, pkg_what,
								p + head + HEAD_SIZE, pkg_size)) {

						head = head + HEAD_SIZE + pkg_size;
						
					} else {
						video_in_end_position = 0;
					}
			}
		}while((video_in_end_position !=0) && (pkg_id != -1));

	}
	return NULL;
}

int USBAndroidScreen_getWidth() {
	return screen_width;
}
int USBAndroidScreen_getHeight() {
	return screen_height;
}
