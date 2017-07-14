
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
#include <stdarg.h>
#include <string.h>

#include "USBAndroid.h"

static int iUSBAndroidLog_Level = 2;
#define MAX_LOG_LEN		256
void USBAndroid_log(int level, const char *function, const char *format, ...)
{
	char buf[MAX_LOG_LEN + 1];

	va_list args;
	if(level < iUSBAndroidLog_Level ) {
		return;
	}
	va_start (args, format);
	vsnprintf(buf, MAX_LOG_LEN ,format, args);
	va_end (args);
	if (level == __LOG_LEVEL_ERROR__ ) {
		fprintf(stderr,"[%d]%s: %s", level, function, buf);
	} else {
		fprintf(stdout,"[%d]%s: %s", level, function, buf);
	}
	
}
void USBAndroid_SetLogLevel(int level)
{
	iUSBAndroidLog_Level = level;
}
