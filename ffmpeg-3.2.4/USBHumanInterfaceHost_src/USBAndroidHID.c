/*
 ============================================================================
 Name        : USBAndroidHID.c
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

#include "USBAndroid.h"
#include "USBAndroidHID.h"


#define KEYBOARD_DEVICE_ID 0
#define TOUCH_DEVICE_ID  1
#define MOUSE_DEVICE_ID		3

#define ACCESSORY_REGISTER_HID 54
#define ACCESSORY_SET_HID_REPORT_DESC 56
#define ACCESSORY_SEND_HID_EVENT 57

static int keyboard_exist = 0;
static int mouse_exist = 0;
#define screen_width USBAndroidScreen_getWidth()
#define screen_height  USBAndroidScreen_getHeight()

static int USBAndroidHID_registerKeyboard(libusb_device_handle* handle) {
	int mHidLength = sizeof(KeyBoardReportDescriptor);
	keyboard_exist = 0;

	int response = libusb_control_transfer(handle, 0x40, ACCESSORY_REGISTER_HID,
		KEYBOARD_DEVICE_ID, mHidLength, NULL, 0, 0);
	_usbandroid_loginfo("registerHid KeyBoard%d(%d)\n", mHidLength, response);
	if (response < 0) {
		return -1;
	}

	response = libusb_control_transfer(handle, 0x40,
	ACCESSORY_SET_HID_REPORT_DESC, KEYBOARD_DEVICE_ID, 0,
			(unsigned char*) KeyBoardReportDescriptor, mHidLength, 0);

	_usbandroid_loginfo("registerHid Desc %d(%d)\n", mHidLength, response);
	if (response < 0) {
		return -1;
	}
	keyboard_exist = 1;
	return 0;
}

static int USBAndroidHID_registerMouse(libusb_device_handle* handle) {

	int mHidLength = sizeof(MouseReportDescriptor);
	mouse_exist = 0;

	int response = libusb_control_transfer(handle, 0x40, ACCESSORY_REGISTER_HID,
	MOUSE_DEVICE_ID, mHidLength, NULL, 0, 0);
	_usbandroid_loginfo("registerHid %d(%d)\n", mHidLength, response);
	if (response < 0) {
		return -1;
	}

	response = libusb_control_transfer(handle, 0x40,
	ACCESSORY_SET_HID_REPORT_DESC, MOUSE_DEVICE_ID, 0,
			(unsigned char *) MouseReportDescriptor, mHidLength, 0);

	_usbandroid_loginfo("registerHid Desc %d(%d)\n", mHidLength, response);
	if (response < 0) {
		return -2;
	}
	mouse_exist = 1;
	return 0;
}
static int USBAndroidHID_registerMultiTouch(libusb_device_handle* handle) {
	int mHidLength = sizeof(MultiTouchReportDescriptor);

	int response = libusb_control_transfer(handle, 0x40, ACCESSORY_REGISTER_HID,
		TOUCH_DEVICE_ID, mHidLength, NULL, 0, 0);
	_usbandroid_loginfo("registerHid MultiTouch%d(%d)\n", mHidLength, response);
	if (response < 0) {
		return -1;
	}

	response = libusb_control_transfer(handle, 0x40,
	ACCESSORY_SET_HID_REPORT_DESC, TOUCH_DEVICE_ID, 0,
			(unsigned char*) MultiTouchReportDescriptor, mHidLength, 0);

	_usbandroid_loginfo("registerHid Desc %d(%d)\n", mHidLength, response);
	if (response < 0) {
		return -1;
	}
	return 0;
}

static int USBAndroidHID_registerSingleTouch(libusb_device_handle* handle) {
	int mHidLength = sizeof(SingleTouchReportDescriptor);

	int response = libusb_control_transfer(handle, 0x40, ACCESSORY_REGISTER_HID,
		TOUCH_DEVICE_ID, mHidLength, NULL, 0, 0);
	_usbandroid_loginfo("registerHid SingleTouch %d(%d)\n", mHidLength, response);
	if (response < 0) {
		return -1;
	}

	response = libusb_control_transfer(handle, 0x40,
		ACCESSORY_SET_HID_REPORT_DESC, TOUCH_DEVICE_ID, 0,
			(unsigned char*) SingleTouchReportDescriptor, mHidLength, 0);

	_usbandroid_loginfo("registerHid Desc %d(%d)\n", mHidLength, response);
	if (response < 0) {
		return -1;
	}
	return 0;
}

static void LIBUSB_CALL cb_dummy(struct libusb_transfer *transfer) {
	if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
		_usbandroid_logerr( "mode change transfer not completed!\n");
	} else {
		#if 0
			_usbandroid_loginfo("async report HID length=%d actual_length=%d\n", transfer->length,
				transfer->actual_length);
		#endif
	}
}
static void USBAndroidHID_reportMouse(libusb_device_handle* handle,
		struct _hid_data *_hid) {

	const int mouse_length = 6;
	struct libusb_transfer *transfer;
	struct {
		uint8_t sign;
		uint8_t status;
		int16_t dx;
		int16_t dy;
		uint8_t dummy;
	} mouse;
	unsigned char *buf = (unsigned char*) malloc(
	LIBUSB_CONTROL_SETUP_SIZE + mouse_length);
	unsigned char *pMouse = (unsigned char*) &mouse;
	memset(&mouse, 0, sizeof(mouse));
	mouse.status = _hid->status;
	mouse.dx = (int)(_hid->dx * 1.6);
	mouse.dy = (int)(_hid->dy * 1.6);

	if (!buf)
		return;

	transfer = libusb_alloc_transfer(0);
	if (!transfer) {
		free(buf);
		return;
	}

	libusb_fill_control_setup(buf, 0x40, ACCESSORY_SEND_HID_EVENT,
		MOUSE_DEVICE_ID, 0, mouse_length);
	memcpy(buf + LIBUSB_CONTROL_SETUP_SIZE, pMouse + 1, mouse_length);
	libusb_fill_control_transfer(transfer, handle, buf, cb_dummy, NULL, 0);

	transfer->flags = LIBUSB_TRANSFER_FREE_BUFFER
			| LIBUSB_TRANSFER_FREE_TRANSFER;
	libusb_submit_transfer(transfer);

}



static void USBAndroidHID_reportMultiTouch(libusb_device_handle* handle,
		struct _hid_data *_hid) {

	int x = _hid->x;
	int y = _hid->y;

	if (_hid->status == 0) {
		return;
	}

	const int multi_touch_length = 12;
	struct libusb_transfer *transfer;
	struct {
		uint8_t dummy;
		uint8_t one1;
		uint8_t status;
		uint8_t one2;
		int16_t x;
		int16_t y;
		uint8_t zero[4];
		uint8_t one3;
	} multi_touch;
	memset(&multi_touch, 0, sizeof(multi_touch));

	unsigned char *buf = (unsigned char*) malloc(
		LIBUSB_CONTROL_SETUP_SIZE + multi_touch_length);
	unsigned char *pMT = (unsigned char*) &multi_touch;

	multi_touch.status = (_hid->status & 0x1) == 0 ? 0 : 1;
	multi_touch.x = x * POINTER_MULTI_TOUCH;
	multi_touch.y = y * POINTER_MULTI_TOUCH;
	multi_touch.one1 = 1;
	multi_touch.one2 = 1;
	multi_touch.one3 = 1;

	if (!buf)
		return;

	transfer = libusb_alloc_transfer(0);
	if (!transfer) {
		free(buf);
		return;
	}

	libusb_fill_control_setup(buf, 0x40, ACCESSORY_SEND_HID_EVENT,
		TOUCH_DEVICE_ID, 0, multi_touch_length);
	memcpy(buf + LIBUSB_CONTROL_SETUP_SIZE, pMT + 1, multi_touch_length);
	libusb_fill_control_transfer(transfer, handle, buf, cb_dummy, NULL, 0);

	transfer->flags = LIBUSB_TRANSFER_FREE_BUFFER
			| LIBUSB_TRANSFER_FREE_TRANSFER;
	libusb_submit_transfer(transfer);

}

//https://github.com/mentatpsi/Microchip/tree/master/USB/Device%20-%20HID%20-%20Digitizers/Single%20Touch%20-%20Firmware
static void USBAndroidHID_reportSingleTouch(libusb_device_handle* handle,
		struct _hid_data *_hid) {

	int x = _hid->x * 80;
	int y = _hid->y * 80;

	if (_hid->status == 0) {
		return;
	}

	struct libusb_transfer *transfer;

	const int single_touch_length = 13;
	unsigned char single_touch[13];

	memset(&single_touch, 0, sizeof(single_touch));
	{
		#define buffer single_touch
		//Report ID is 0x01 always for this example, based on the report descriptor.
			buffer[0] = 0x01;	

		//	byte[1]: bit 0 = tip switch
		//	byte[1]: bit 1 = barrel switch
		//	byte[1]: bit 2 = eraser switch
		//	byte[1]: bit 3 = invert
		//	byte[1]: bit 4 = in-range (used for hover detection.  If actually indicating tip switch pressed, must set in-range = 1 as well)
		//	byte[1]: bits 5-7 = pad bits, unused
			buffer[1] = 0b00010000;		//See above individual bit definitions.
			buffer[2] = 0b00000000;		//bits 0-7 are all pad bits and are not used

			buffer[3] = x & 0xFF;		//Byte 2 is LSB of x coord (0-32767)
			buffer[4] = (x >> 8);	//Byte 3 is MSB of x coord (0-32767)
	
			buffer[5] = y & 0xFF;		//Byte 2 is LSB of y coord (0-32767)
			buffer[6] = (y >> 8);	//Byte 3 is MSB of y coord (0-32767)

			buffer[7] = 0x00;	//Byte 6 is LSB of tip pressure (0-32767)
			buffer[8] = 0x00;	//Byte 7 is MSB of tip pressure (0-32767)

			buffer[9] = 0x00;	//Byte 8 is x tilt LSB	(-32767 to 32767)
			buffer[10] = 0x00;	//Byte 9 is x tilt MSB	(-32767 to 32767)
	
			buffer[11] = 0x00;	//Byte 10 is y tilt LSB	(-32767 to 32767)
			buffer[12] = 0x00;	//Byte 11 is y tilt MSB	(-32767 to 32767)

			if( (_hid->status & 0x1 ) != 0 ) {
				buffer[1] = 0b00010001;	//Tip switch and in range indicators set.
				buffer[7] = 0xFF;	//Byte 6 is LSB of tip pressure (0-32767)
				buffer[8] = 0x7F;	//Byte 7 is MSB of tip pressure (0-32767)
			}

		#undef buffer
	}
	unsigned char *buf = (unsigned char*) malloc( LIBUSB_CONTROL_SETUP_SIZE + single_touch_length);
	unsigned char *pMT = (unsigned char*) single_touch;



	if (!buf)
		return;

	transfer = libusb_alloc_transfer(0);
	if (!transfer) {
		free(buf);
		return;
	}

	libusb_fill_control_setup(buf, 0x40, ACCESSORY_SEND_HID_EVENT,
			TOUCH_DEVICE_ID, 0, single_touch_length);
	memcpy(buf + LIBUSB_CONTROL_SETUP_SIZE, pMT, single_touch_length);
	libusb_fill_control_transfer(transfer, handle, buf, cb_dummy, NULL, 0);

	transfer->flags = LIBUSB_TRANSFER_FREE_BUFFER
			| LIBUSB_TRANSFER_FREE_TRANSFER;
	libusb_submit_transfer(transfer);

}

extern int USBAndroidKeymap_getKeyMap(int key);
static uint8_t key_status = 0;
static void USBAndroidHID_reportKey(libusb_device_handle* handle,
		struct _hid_data *_hid) {

	const int key_length = 8;
	struct libusb_transfer *transfer;
	int key = USBAndroidKeymap_getKeyMap(_hid->code);
	if (key < 0)
		return;

	unsigned char *buf = (unsigned char*) malloc(
	LIBUSB_CONTROL_SETUP_SIZE + key_length);
	if (!buf)
		return;

	transfer = libusb_alloc_transfer(0);
	if (!transfer) {
		free(buf);
		return;
	}
	libusb_fill_control_setup(buf, 0x40, ACCESSORY_SEND_HID_EVENT,
	KEYBOARD_DEVICE_ID, 0, key_length);
	memset(buf + LIBUSB_CONTROL_SETUP_SIZE, 0, key_length);
	uint8_t _ctrl_mask = 0;

	switch (_hid->code) {
	case 306: 	// Left Ctrl
		_ctrl_mask = 0x01;
		break;
	case 304: 	// Left Shift
		_ctrl_mask = 0x02;
		break;
	case 308: 	// Left Alt
		_ctrl_mask = 0x04;
		break;
	case 305:  // Right Ctrl
		_ctrl_mask = 0x10;
		break;
	case 303: //# Right Shift
		_ctrl_mask = 0x20;
		break;
	case 307: //# Right Alt
		_ctrl_mask = 0x40;
		break;
	}
	if (_ctrl_mask != 0) {
		if (_hid->status != 0) {
			key_status |= _ctrl_mask;
		} else {
			key_status &= ~_ctrl_mask;
		}
	}
	if (_hid->status == 0)  // Key release
		key = 0;
	buf[ LIBUSB_CONTROL_SETUP_SIZE + 0] = key_status;
	buf[ LIBUSB_CONTROL_SETUP_SIZE + 2] = key;
	libusb_fill_control_transfer(transfer, handle, buf, cb_dummy, NULL, 0);

	transfer->flags = LIBUSB_TRANSFER_FREE_BUFFER
			| LIBUSB_TRANSFER_FREE_TRANSFER;
	libusb_submit_transfer(transfer);
	_usbandroid_loginfo("reportTouch Key(%d,%d)\n", key, _hid->status);

}

void USBAndroidHID_registerHID(libusb_device_handle* handle) {
	keyboard_exist = 0;
	mouse_exist = 0;
	key_status = 0;

#ifdef SUPPORT_HID
	USBAndroidHID_registerKeyboard(handle);
	#if (POINTER_DEVICE == POINTER_MULTI_TOUCH)
		USBAndroidHID_registerMultiTouch(handle);
	#elif (POINTER_DEVICE == POINTER_SINGLE_TOUCH)
		USBAndroidHID_registerSingleTouch(handle);
	#elif (POINTER_DEVICE == POINTER_MOUSE)
		USBAndroidHID_registerMouse(handle);
	#endif
#endif
}

void USBAndroidHID_reportHIDEvent(libusb_device_handle* handle,
		struct _hid_data *_hid) {
	switch (_hid->type) {
	case 1: // key event
		if (keyboard_exist != 0) {
			USBAndroidHID_reportKey(handle, _hid);
		}

		break;
	case 2: // mouse relation event
		{
			float aspect_ratio = (float)screen_height / _hid->video_h;

			int x = (int) (1.0 * (_hid->x - ((float)_hid->video_w - screen_width / aspect_ratio) / 2 ) * aspect_ratio);
			int y = (int) (1.0 * (_hid->y - _hid->video_y) * aspect_ratio);
			//_usbandroid_loginfo("reportTouch (%d,%d,%d,%d,%d,%d,)\n",_hid->x, _hid->video_x, _hid->video_w ,_hid->y, _hid->video_y, _hid->video_h );
			//_usbandroid_loginfo("reportTouch Screen (%d,%d)\n", screen_width, screen_height);
			if( _hid->status != 0) {
				_usbandroid_loginfo("reportTouch (%d,%d) (%d,%d)\n", x, y, _hid->dx , _hid->dy);
			}
			#if (POINTER_DEVICE == POINTER_MULTI_TOUCH)
				if (x < 0 || y < 0 || x >= screen_width || y >= screen_height) {
					//return;
				}

				_hid->x = x;
				_hid->y = y;
				
				USBAndroidHID_reportMultiTouch(handle, _hid);
			#elif (POINTER_DEVICE == POINTER_SINGLE_TOUCH)
				USBAndroidHID_reportSingleTouch(handle, _hid);
			#elif (POINTER_DEVICE == POINTER_MOUSE)
				
				if (mouse_exist != 0) {
					USBAndroidHID_reportMouse(handle, _hid);
				}
			#endif

		}
		break;
	default:
		break;
	}
}
