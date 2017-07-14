/*
 ============================================================================
 Name        : USBAndroidMediaPlay.c
 Author      : lvhwa0716@gmail.com
 Version     :
 Copyright   : 
 Description :  Ansi-style
 ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "USBAndroid.h"

extern int ffplay_main(int argc, char **argv);

void *USBAndroidMediaPlay_thread_main(void *arg) {
	char str_fd[32] = "";

	memset(str_fd, 0, sizeof(str_fd));

	char *p[] = { "ffplay_main", "-f", "h264", "-vcodec", "h264", "-sn", "-an",
			str_fd
	};
#if defined(H264_MULTICAST_UDP_ADDR)
	sprintf(str_fd, "udp://%s:%d", H264_MULTICAST_UDP_ADDR, H264_MULTICAST_UDP_PORT);
#else
	sprintf(str_fd, "pipe:%d", MessageNotify_getReadFd());
#endif
	_usbandroid_loginfo("ffplay file: %s\n", str_fd);
	int result = ffplay_main(sizeof(p) / sizeof(char*), p);
	_usbandroid_loginfo("ffplay exit = %d\n", result);
	return NULL;
}
