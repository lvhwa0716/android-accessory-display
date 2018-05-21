/*
 ============================================================================
 Name        : USBAndroidDevice.c
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

#include "USBAndroidDescriptor.h"
#include "USBAndroid.h"

struct support_device_id {
	uint16_t vid;
	uint16_t pid;
};

static const struct support_device_id _vid_pid[] = {
// { _accessory_vid, _accessory_pid }, // when detect this , must reset device , maybe die before
		{ 0x2717, 0xff68 }, { 0x2717, 0xff48 }, { 0x18d1, 0x4e22 }, { 0x18d1,
				0x4e20 }, { 0x18d1, 0x4e30 }, { 0x18d1, 0xd002 }, { 0x18d1,
				0x4e42 }, { 0x18d1, 0x4e40 }, { 0x18d1, 0x4ee2 }, { 0x18d1,
				0x4ee0 }, { 0x18d1, 0x4ee1 }, };

static const ssize_t _vid_pid_length = sizeof(_vid_pid)
		/ sizeof(struct support_device_id);

int USBAndroidDevice_isValid(usbandroid_handle *handle) {
	if (handle->handle != NULL) {
		return 0;
	}
	return -1;
}
void USBAndroidDevice_Dump(usbandroid_handle *handle) {
	_usbandroid_logdbg("Handle = %p, interface =%d\n", handle->handle, handle->interface);
}
/*
 * handle must be closed by caller with libusb_close(handle);
 * */
int USBAndroidDevice_isSupport(libusb_device *dev, uint16_t *pVid,
		uint16_t *pPid) {
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *config = NULL;
	int rc;
	uint8_t cfg_index = 0;

	rc = libusb_get_device_descriptor(dev, &desc);
	if (LIBUSB_SUCCESS != rc) {
		_usbandroid_logerr( "Error getting device descriptor\n");
		return -1;
	}

	_usbandroid_loginfo("Device attached: %04x:%04x\n", desc.idVendor, desc.idProduct);
	*pVid = desc.idVendor;
	*pPid = desc.idProduct;

	for (cfg_index = 0; cfg_index < desc.bNumConfigurations; cfg_index++) {

		rc = libusb_get_config_descriptor(dev, cfg_index, &config);
		if (rc == LIBUSB_SUCCESS) {
			uint8_t i = 0;
			uint8_t e = 0;

			_usbandroid_loginfo("  Configuration:\n");
			_usbandroid_loginfo("    wTotalLength:         %d\n", config->wTotalLength);
			_usbandroid_loginfo("    bNumInterfaces:       %d\n", config->bNumInterfaces);
			_usbandroid_loginfo("    bConfigurationValue:  %d\n",
					config->bConfigurationValue);
			_usbandroid_loginfo("    iConfiguration:       %d\n", config->iConfiguration);
			_usbandroid_loginfo("    bmAttributes:         %02xh\n", config->bmAttributes);
			_usbandroid_loginfo("    MaxPower:             %d\n", config->MaxPower);

			for (i = 0; i < config->bNumInterfaces; i++) {
				const struct libusb_interface_descriptor *interface =
						config->interface[i].altsetting;
				for (e = 0; e < config->interface[i].num_altsetting; e++) {
					if ((interface[e].bNumEndpoints == 2)
							&& (interface[e].bInterfaceClass == 0xff)
							&& (interface[e].bInterfaceSubClass == 0x42)
							&& (interface[e].bInterfaceProtocol == 0x01)) {
						// found it
						_usbandroid_loginfo("  interface: %d\n", e);
						libusb_free_config_descriptor(config);
						return 0;
					}
				}
			}
			libusb_free_config_descriptor(config);
		}
	}

	return -1;
}

static int USBAndroidDevice_checkVidPid(uint16_t vid, uint16_t pid) {
	int i;
	for (i = 0; i < _vid_pid_length; i++) {
		if ((_vid_pid[i].pid == pid) && (_vid_pid[i].vid == vid)) {
			return LIBUSB_SUCCESS;
		}
	}
	return LIBUSB_ERROR_NOT_SUPPORTED;
}

usbandroid_handle USBAndroidDevice_openClaim(libusb_context *ctx, uint16_t vid,
		uint16_t pid, int interface) {

	int tries = 5;
	libusb_device_handle * handle = NULL;
	usbandroid_handle _uhandle = { ctx, NULL, 0, vid, pid };
	_usbandroid_loginfo("openClaim :%04x,%04x,%d\n", vid,pid,interface);
	for (; tries >= 0; tries--) {
		_usbandroid_logdbg( "Try Count :%d\n", tries);

		handle = libusb_open_device_with_vid_pid(ctx, vid, pid);
		if (handle != NULL) {
			_usbandroid_loginfo("Open Success , claim it \n");
			int rc = libusb_claim_interface(handle, interface);
			if (LIBUSB_SUCCESS == rc) {
				_uhandle.handle = handle;
				_uhandle.interface = interface;
				return _uhandle;
			} else {
				_usbandroid_logerr( "claim_interface error: %d\n",rc);
				libusb_close(handle);
				handle = NULL;
				_uhandle.handle = NULL;
			}
		}
		sleep(1);
	}

	return _uhandle;
}

void USBAndroidDevice_closeRelease(usbandroid_handle *handle) {
	if (handle->handle != NULL) {
		libusb_release_interface(handle->handle, handle->interface);
		libusb_close(handle->handle);
		handle->handle = NULL;
	}

}

int USBAndroidDevice_checkAll(libusb_context *ctx, uint16_t *pVid,
		uint16_t *pPid) {
	libusb_device **devs;
	ssize_t i = 0;
	ssize_t cnt;

	int iFound = -1;

	cnt = libusb_get_device_list(ctx, &devs);
	if (cnt < 0)
		return iFound;

	for (i = 0; i < cnt; i++) {
		libusb_device *dev = devs[i];
		iFound = USBAndroidDevice_isSupport(dev, pVid, pPid);
		if (0 == iFound) {
			break;
		}
	}
	if (0 != iFound) { // check this with pid&vid

		for (i = 0; i < cnt; i++) {
			libusb_device *dev = devs[i];
			struct libusb_device_descriptor desc;
			if (LIBUSB_SUCCESS == libusb_get_device_descriptor(dev, &desc)) {
				if (LIBUSB_SUCCESS
						== USBAndroidDevice_checkVidPid(desc.idVendor,
								desc.idProduct)) {
					iFound = 0;
					*pVid = desc.idVendor;
					*pPid = desc.idProduct;

					break;
				}
			}
		}
	}

	libusb_free_device_list(devs, 1);
	return iFound;

}

int USBAndroidDevice_setupUSBAcc(usbandroid_handle *_uhandle) {
	uint8_t ioBuffer[2];
	uint16_t devVersion;
	int rc;
	libusb_device_handle *handle = _uhandle->handle;
	USBAndroidDevice_Dump(_uhandle);
	int tries = 5;
	for (; tries > 0; tries--) {
		rc = libusb_control_transfer(handle, //handle
				0xC0, //bmRequestType
				51, //bRequest
				0, //wValue
				0, //wIndex
				ioBuffer, //data
				2, //wLength
				0 //timeout
				);
		if (rc != 2) {
			_usbandroid_loginfo("Step-0 : %s\n", libusb_error_name(rc));
			sleep(1); // wait
			continue;
		}
		devVersion = ioBuffer[1] << 8 | ioBuffer[0];
		_usbandroid_loginfo("Version Code Device: %d\n", devVersion);

		sleep(1); // wait
		rc = libusb_control_transfer(handle, 0x40, 52, 0, 0,
				(unsigned char*) _usbandroid_manufacturer,
				strlen(_usbandroid_manufacturer), 0);
		if (rc < 0) {
			_usbandroid_logerr( "Step-1 : %s\n", libusb_error_name(rc));
			continue;
		}
		rc = libusb_control_transfer(handle, 0x40, 52, 0, 1,
				(unsigned char*) _usbandroid_model,
				strlen(_usbandroid_model) + 1, 0);
		if (rc < 0) {
			_usbandroid_logerr( "Step-2 : %s\n", libusb_error_name(rc));
			continue;
		}
		rc = libusb_control_transfer(handle, 0x40, 52, 0, 2,
				(unsigned char*) _usbandroid_description,
				strlen(_usbandroid_description) + 1, 0);
		if (rc < 0) {
			_usbandroid_logerr("Step-3 : %s\n", libusb_error_name(rc));
			continue;
		}
		rc = libusb_control_transfer(handle, 0x40, 52, 0, 3,
				(unsigned char*) _usbandroid_version,
				strlen(_usbandroid_version) + 1, 0);
		if (rc < 0) {
			_usbandroid_logerr("Step-4 : %s\n", libusb_error_name(rc));
			continue;
		}
		rc = libusb_control_transfer(handle, 0x40, 52, 0, 4,
				(unsigned char*) _usbandroid_uri, strlen(_usbandroid_uri) + 1,
				0);
		if (rc < 0) {
			_usbandroid_logerr("Step-5 : %s\n", libusb_error_name(rc));
			continue;
		}
		rc = libusb_control_transfer(handle, 0x40, 52, 0, 5,
				(unsigned char*) _usbandroid_serialnumber,
				strlen(_usbandroid_serialnumber) + 1, 0);
		if (rc < 0) {
			_usbandroid_logerr( "Step-6 : %s\n", libusb_error_name(rc));
			continue;
		}

		rc = libusb_control_transfer(handle, 0x40, 53, 0, 0, NULL, 0, 0);
		if (rc < 0) {
			_usbandroid_logerr( "Step-7 : %s\n", libusb_error_name(rc));
			continue;
		} else {
			break;
		}
	}
	if (tries <= 0) {
		return LIBUSB_ERROR_OTHER;
	} else {
		return LIBUSB_SUCCESS;
	}
}

// when call in hotplug callback ,must set ctx=NULL, which will create a new context
int USBAndroidDevice_enterUsbAcc(libusb_context *ctx, uint16_t vid,
		uint16_t pid, int interface) {
	int result = -1;
	int rc;
	int need_release = 0;
	libusb_context *local_ctx = ctx;
	if (local_ctx == NULL) {
		rc = libusb_init(&local_ctx);
		if (rc < 0) {
			_usbandroid_logerr("failed to initialise libusb: %s\n", libusb_error_name(rc));
			return EXIT_FAILURE;
		}
		need_release = 1;
	}
	usbandroid_handle _u = USBAndroidDevice_openClaim(local_ctx, vid, pid, 0);
	if (0 == USBAndroidDevice_isValid(&_u)) {
		_usbandroid_loginfo("    Open Successes! \n");
		if (LIBUSB_SUCCESS == USBAndroidDevice_setupUSBAcc(&_u)) {
			_usbandroid_loginfo("    setup USBAcc OK \n");
			result = 0;
		} else {
			_usbandroid_logerr("    setup USBAcc Failed \n");
		}
		USBAndroidDevice_closeRelease(&_u);
	}
	if (1 == need_release) {
		libusb_exit(local_ctx);
	}
	return result;
}
void USBAndroidDevice_resetUsbAcc(libusb_context *ctx) {
	libusb_device_handle * handle = NULL;
	int need_release = 0;
	int rc;
	libusb_context *local_ctx = ctx;
	if (local_ctx == NULL) {
		rc = libusb_init(&local_ctx);
		if (rc < 0) {
			_usbandroid_logerr("failed to initialise libusb: %s\n", libusb_error_name(rc));
			return;
		}
		need_release = 1;
	}
	handle = libusb_open_device_with_vid_pid(ctx, _accessory_vid,
	_accessory_pid);
	if (handle != NULL) {
		_usbandroid_logdbg("Alread in USB Accessory mode , reset it\n");
		libusb_reset_device(handle);
		libusb_close(handle);
	}
	if (1 == need_release) {
		libusb_exit(local_ctx);
	}
}

int USBAndroidDevice_checkAccExist(libusb_context *ctx, uint16_t *pVid, uint16_t *pPid) {
	libusb_device **devs;
	ssize_t i = 0;
	ssize_t cnt;
	int ret = -1;

	cnt = libusb_get_device_list(ctx, &devs);
	if (cnt > 0) {

		for (i = 0; i < cnt; i++) {
			libusb_device *dev = devs[i];
			struct libusb_device_descriptor desc;
			if (LIBUSB_SUCCESS == libusb_get_device_descriptor(dev, &desc)) {
				if ( (desc.idVendor == _accessory_vid) && (desc.idProduct == _accessory_pid )) {
					*pVid = _accessory_vid;
					*pPid = _accessory_pid;
					ret = 0;
					break;
				}
			}
		}

		libusb_free_device_list(devs, 1);
	}
	return ret;
}

int USBAndroidDevice_checkExistByVidPid(libusb_context *ctx, uint16_t vid, uint16_t pid) {
	libusb_device **devs;
	ssize_t i = 0;
	ssize_t cnt;
	int ret = -1;

	cnt = libusb_get_device_list(ctx, &devs);
	if (cnt > 0) {

		for (i = 0; i < cnt; i++) {
			libusb_device *dev = devs[i];
			struct libusb_device_descriptor desc;
			if (LIBUSB_SUCCESS == libusb_get_device_descriptor(dev, &desc)) {
				if ( (desc.idVendor == vid) && (desc.idProduct == pid )) {
					ret = 0;
					break;
				}
			}
		}

		libusb_free_device_list(devs, 1);
	}
	return ret;
}

////// device 
#define BULK_SIZE 16384	// truck size , same as java

struct _endpoint_video {
	uint8_t epIn;
	uint8_t epOut;
} _endpoint_video_active;
static struct libusb_transfer *transfer_video_IN = NULL;
static struct libusb_transfer *transfer_video_OUT = NULL;
static int dipatch_status = 0;
static libusb_device_handle *current_handle = NULL;


void *USBAndroidDevice_getPool(void) {
	return malloc(BULK_SIZE);
}
void USBAndroidDevice_freePool(void *p) {
	free(p);
}
static void LIBUSB_CALL cb_video_stream(struct libusb_transfer *transfer) {
	unsigned char * pBuf = NULL;

	_usbandroid_loginfo( " enter \n");

	struct _endpoint_video *_in_out =
			(struct _endpoint_video*) transfer->user_data;
	if (transfer->status != LIBUSB_TRANSFER_TIMED_OUT) {
		_usbandroid_loginfo("status = %d, size = %d\n",
				transfer->status, transfer->actual_length);
	}
	switch (transfer->status) {
	case LIBUSB_TRANSFER_COMPLETED:
		// OK
		// (transfer->buffer, transfer->actual_length);
		if(transfer->actual_length > 0) {
			// post message to Screen
			int rc = MessageNotify_PostData(transfer->buffer,transfer->actual_length);
			_usbandroid_loginfo( " MessageNotify_PostData %d\n", rc);
		} else {
			pBuf = transfer->buffer; // reuse old buf
		}
		break;

	case LIBUSB_TRANSFER_ERROR: // terminal wait new device
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_STALL:
	case LIBUSB_TRANSFER_NO_DEVICE:
	case LIBUSB_TRANSFER_OVERFLOW:
	default:
		// error 
		USBAndroidDevice_freePool(transfer->buffer);
		dipatch_status = -255;
		return;
	case LIBUSB_TRANSFER_TIMED_OUT: // try again
		// setup_next_transfer;
		pBuf = transfer->buffer;
		break;

	}

	if(pBuf == NULL) {
		pBuf = USBAndroidDevice_getPool();
		if(pBuf == NULL) {
			_usbandroid_logerr( " Allocate Memory Error , terminal\n");
			dipatch_status = -255;
			return;
		}
	}

	libusb_fill_bulk_transfer(transfer_video_IN,
			transfer->dev_handle, _in_out->epIn, pBuf,
			BULK_SIZE, cb_video_stream, (void*) _in_out, 0);
	libusb_submit_transfer(transfer_video_IN);
	return;

}
static int findEndpoints(libusb_device_handle* handle, uint8_t *_in,
		uint8_t *_out) {
	libusb_device * dev = libusb_get_device(handle);
	struct libusb_config_descriptor *config;

	unsigned char epIn = 0x00;
	unsigned char epOut = 0x80;

	if (LIBUSB_SUCCESS != libusb_get_active_config_descriptor(dev, &config))
		return -1;

	uint8_t iface_idx;
	for (iface_idx = 0; iface_idx < config->bNumInterfaces; iface_idx++) {
		const struct libusb_interface *iface = &config->interface[iface_idx];
		int altsetting_idx;

		for (altsetting_idx = 0; altsetting_idx < iface->num_altsetting;
				altsetting_idx++) {
			const struct libusb_interface_descriptor *altsetting =
					&iface->altsetting[altsetting_idx];
			uint8_t ep_idx;

			for (ep_idx = 0; ep_idx < altsetting->bNumEndpoints; ep_idx++) {
				const struct libusb_endpoint_descriptor *ep =
						&altsetting->endpoint[ep_idx];
				if (0 != (ep->bEndpointAddress & 0x80)) {
					// IN
					if (epIn == 0x00) {
						epIn = ep->bEndpointAddress;
					}
				} else {
					// OUT
					if (epOut == 0x80) {
						epOut = ep->bEndpointAddress;
					}
				}
			}
		}
		if ((epIn != 0x00) && (epOut != 0x80)) {
			break;
		}
	}

	libusb_free_config_descriptor(config);
	if ((epIn != 0x00) && (epOut != 0x80)) {
		*_in = epIn;
		*_out = epOut;
		return 0;
	}
	return -1;
}
static void LIBUSB_CALL cb_dummy(struct libusb_transfer *transfer) {
	_usbandroid_loginfo("cb_dummy... status = %d, size = %d\n",
			transfer->status, transfer->actual_length);
}
int USBAndroidDevice_sendToPeer(unsigned char *p, int size , int needfree) {
	libusb_device_handle *handle = current_handle;
	if( handle != NULL) {
		libusb_fill_bulk_transfer(transfer_video_OUT, handle, _endpoint_video_active.epOut,
						p, size, cb_dummy, NULL, 0);
		if(needfree != 0 )
			transfer_video_OUT->flags = LIBUSB_TRANSFER_FREE_BUFFER;
		libusb_submit_transfer(transfer_video_OUT);
		return 0;
	}
	return -1;
}


void *USBAndroidDevice_thread_main(void *arg) {
	libusb_context *_libusb_ctx = NULL;
	int rc;
	uint16_t vid;
	uint16_t pid;
	uint8_t epIn;
	uint8_t epOut;

	rc = libusb_init(&_libusb_ctx);
	if (rc < 0) {
		_usbandroid_logerr("USBAndroidScreen failed to initialise libusb: %s\n",
				libusb_error_name(rc));
		exit(-1);
	}
	_usbandroid_loginfo("Accessory Context : %p\n", _libusb_ctx);
	transfer_video_IN = libusb_alloc_transfer(0);
	transfer_video_OUT = libusb_alloc_transfer(0);

	while (1) {
		if (0 != MessageNotify_WaitUSBAccAttach(&vid, &pid)) {
			sleep(1);
			continue;
		}
		dipatch_status = 0;

		_usbandroid_loginfo("VID:%04x,PID:%04x\n", vid, pid);



		usbandroid_handle _u = USBAndroidDevice_openClaim(_libusb_ctx, vid, pid,
				0);
		libusb_device_handle *handle = _u.handle;
		if (0 != USBAndroidDevice_isValid(&_u)) {
			_usbandroid_logerr("USBAndroidScreen Open USB Accessory Error ! \n");
			continue;
		}
		current_handle = handle ;
		// Get IN/OUT Endpoint
		if (0 != findEndpoints(handle, &epIn, &epOut)) {
			_usbandroid_logerr(" Open USB Accessory Endpoint error\n");
			goto error_close_handle;
		} else {
			_usbandroid_loginfo("Endpoint IN=%d,OUT=%d\n", epIn, epOut);
		}
		_endpoint_video_active.epIn = epIn;
		_endpoint_video_active.epOut = epOut;
		// set notify context
		MessageNotify_setContext(handle);
		// register HID
		USBAndroidHID_registerHID(handle);
		// loop 
		unsigned char *pBuf = USBAndroidDevice_getPool();
		if(pBuf == NULL) {
			_usbandroid_logerr( " Allocate Memory Error , terminal\n");
			dipatch_status = -255;
			exit(-1);
		}

		libusb_fill_bulk_transfer(transfer_video_IN, handle, epIn,
				pBuf,
				BULK_SIZE, cb_video_stream, &_endpoint_video_active, 0);
		dipatch_status = libusb_submit_transfer(transfer_video_IN);
		while (dipatch_status == 0) {
			rc = libusb_handle_events(_libusb_ctx);
			if (rc < 0) {
				_usbandroid_logerr("libusb_handle_events() failed: %s\n",
						libusb_error_name(rc));
			}
		}
		_usbandroid_logerr("(device error)dipatch_status: %d\n",dipatch_status);
		//
		error_close_handle: libusb_reset_device(handle);
		current_handle = NULL;
		MessageNotify_setContext(NULL);
		USBAndroidDevice_closeRelease(&_u);

	}
	libusb_exit(_libusb_ctx);
	return NULL;
}
