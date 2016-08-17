#!/usr/bin/python
# coding=utf-8

"""
 lvhwa0716@163.com
 python 2.7 & ubuntu 14.04LTS
 1. install libusb
 2. install ffmpeg
    sudo add-apt-repository ppa:mc3man/trusty-media
    sudo apt-get update
    sudo apt-get install ffmpeg
 3. install pyusb
    or download source :  https://github.com/walac/pyusb
 4. permission
    /etc/udev/rules.d/51-android.rules
    SUBSYSTEM=="usb", ATTR{idVendor}=="18d1", ATTR{idProduct}=="2d01", MODE="0666", OWNER="<username>"
    SUBSYSTEM=="usb", ATTR{idVendor}=="2717", ATTR{idProduct}=="ff68", MODE="0666", OWNER="<username>"
    SUBSYSTEM=="usb", ATTR{idVendor}=="2717", ATTR{idProduct}=="ff48", MODE="0666", OWNER="<username>"
    or used sudo when permission exception
 5. run
    enable USB debug on phone
    a. aosp/frameworks/base/tests/AccessoryDisplay on phone
        when use this , screen size must less 320x240,
        or modify com.android.accessorydisplay.common.Protocol.MAX_ENVELOPE_SIZE more larger
    b. run this file
    c. run ffplay /tmp/android_accessory_display.pipe on ubuntu.
       ffplay -f h264 -vcodec h264 -sn -an /tmp/android_accessory_display.pipe
       prebuilt ffplay support video sync , it show slow then phone.
       fixed it(not use video sync) : get ffmpeg source
         modify ffplay.c like follow :
            time= av_gettime_relative()/1000000.0;
            if (time < is->frame_timer + delay) {
                *remaining_time = FFMIN(is->frame_timer + delay - time, *remaining_time);
                //goto display; // == mask this line
            }

            is->frame_timer += delay;

    Note : default use multi-touch hid, if phone not support please set _support_touch = 2 , drag and double click will work


"""

import array
import sys
import time
import usb
import struct
import threading
import os
import Queue


# import pygst

class AndroidAccessoryDisplay:
    _B = 'B' if sys.version_info.major == 3 else b'B'
    _BIT_RATE = 6000000
    _FRAME_RATE = 30
    _I_FRAME_INTERVAL = 10

    _MAX_CONTENT_SIZE = 8 * 64 * 1024  # com.android.accessorydisplay.common.Protocol.MAX_ENVELOPE_SIZE
    _HEADER_SIZE = 8

    _ERR_ID = -1
    _ERR_TIMEOUT = 1

    # send to Sink
    _ID = 1
    _MSG_QUERY = 1
    _MSG_CONTENT = 2  # Send MPEG2-TS H.264 encoded content.

    _MSG_QUERY_EXT = 100

    # send to Source
    _SOURCE_ID = 2
    _MSG_SINK_AVAILABLE = 1
    _MSG_SINK_NOT_AVAILABLE = 2

    _MSG_SINK_INJECT_EVENT = 100

    _manufacturer = "Android"
    _model = "Accessory Display"
    _description = "Mirror Screen to Accessory Display"
    _version = 1
    _uri = "https://github.com/lvhwa0716/android-accessory-display"

    _accessory_pid = 0x2d01
    _accessory_vid = 0x18d1

    # _support_touch = 0 : not support, 1 : multi-touch , 2 : single-touch,
    _support_touch = 1
    # _support_touch =  0 : not support, 1 : 101 keyboard
    _support_keyboard = 1
    _support_single_touch = 1
    ACCESSORY_REGISTER_HID = 54
    ACCESSORY_SET_HID_REPORT_DESC = 56
    ACCESSORY_SEND_HID_EVENT = 57
    TOUCH_DEVICE_ID = 0
    KEYBOARD_DEVICE_ID = 1
    SINGLE_TOUCH_DEVICE_ID = 2

    # when auto detect failed ,add vid, pid here
    _android_device_list = [
        # [Vendor ID, (Product ID, Product ID ... Product ID) ],
        [_accessory_vid, (_accessory_pid,)],  # accessory mode
        [0x2717, (0xff68, 0xff48)],
        [0x18d1, (0x4e22, 0x4e20, 0x4e30, 0xd002, 0x4e42, 0x4e40, 0x4ee2, 0x4ee0, 0x4ee1)],
    ]

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self._close()

    def __init__(self, log, serial):
        self._media_data_queue = Queue.Queue(maxsize=50)
        self._log = log
        self._isProcessing = False
        self._threadProcess = None
        self.phone_width = 720
        self.phone_height = 1280
        self.phone_dpi = 240
        self._key_status = 0
        self._touch_status = 0
        device = self._autoDetectDevice()
        if device is None:
            raise usb.core.USBError('Device not connected')

        self._vendor_id = device.idVendor
        self._product_id = device.idProduct

        if self._vendor_id == self._accessory_vid and self._product_id == self._accessory_pid:
            # alread in accessory mode  , but maybe it not in accessory display mode, reset it
            device.reset()
            time.sleep(5)
            # detect again
            device = self._autoDetectDevice()
            if device is None:
                raise usb.core.USBError('Device not connected')
            self._vendor_id = device.idVendor
            self._product_id = device.idProduct

        # now device is un-configure device, configure it
        self._log("configure Device")
        attempts_left = 5
        while attempts_left:
            try:
                self._configure(
                    device,
                    str(self._manufacturer),
                    str(self._model),
                    str(self._description),
                    str(self._version),
                    str(self._uri),
                    str(serial))
                break
            except usb.core.USBError as e:
                self._log(e)
                if e.errno == 16:  # Busy.
                    time.sleep(2)
                    attempts_left -= 1
                else:
                    raise e

        attempts_left = 5
        while attempts_left:
            self._device = usb.core.find(idVendor=self._accessory_vid, idProduct=self._accessory_pid)
            if self._device is not None:
                break;
            time.sleep(1)
            attempts_left -= 1
        if self._device is None:
            device.reset()
            raise usb.core.USBError('Device Configure Error')

        # config HID
        if self._support_touch == 1:
            self._device.ctrl_transfer(bmRequestType = 0x40,
                             bRequest = self.ACCESSORY_REGISTER_HID,
                             wValue = self.TOUCH_DEVICE_ID,
                             wIndex = len(AndroidEventInject.MultTouchReportDescriptor))
            self._device.ctrl_transfer(bmRequestType = 0x40,
                             bRequest = self.ACCESSORY_SET_HID_REPORT_DESC,
                             wValue = self.TOUCH_DEVICE_ID,
                             wIndex = 0,
                             data_or_wLength = AndroidEventInject.MultTouchReportDescriptor)
        elif self._support_touch == 2:
            self._device.ctrl_transfer(bmRequestType = 0x40,
                             bRequest = self.ACCESSORY_REGISTER_HID,
                             wValue = self.SINGLE_TOUCH_DEVICE_ID,
                             wIndex = len(AndroidEventInject.SingleTouchReportDescriptor))
            self._device.ctrl_transfer(bmRequestType = 0x40,
                             bRequest = self.ACCESSORY_SET_HID_REPORT_DESC,
                             wValue = self.SINGLE_TOUCH_DEVICE_ID,
                             wIndex = 0,
                             data_or_wLength = AndroidEventInject.SingleTouchReportDescriptor)

        if self._support_keyboard == 1:
            self._device.ctrl_transfer(bmRequestType = 0x40,
                             bRequest = self.ACCESSORY_REGISTER_HID,
                             wValue = self.KEYBOARD_DEVICE_ID,
                             wIndex = len(AndroidEventInject.KeyBoardReportDescriptor))
            self._device.ctrl_transfer(bmRequestType = 0x40,
                             bRequest = self.ACCESSORY_SET_HID_REPORT_DESC,
                             wValue = self.KEYBOARD_DEVICE_ID,
                             wIndex = 0,
                             data_or_wLength = AndroidEventInject.KeyBoardReportDescriptor)

        self._endpoint_out, self._endpoint_in = self._detectEndpoints()



    def _autoDetectDevice(self):
        # auto detect
        devs = usb.core.find(find_all=True)
        for dev in devs:
            for cfg in dev:
                for i in cfg:
                    if i.bNumEndpoints == 2 \
                            and i.bInterfaceClass == 0xff \
                            and i.bInterfaceSubClass == 0x42 \
                            and i.bInterfaceProtocol == 0x01:
                        return i.device

        # auto detect failed , use device list
        def _detectDevice(me):
            for device in me._android_device_list:
                vid = device[0]
                for pid in device[1]:
                    android_device = usb.core.find(idVendor=vid, idProduct=pid)
                    if android_device:
                        return android_device
            return None

        return _detectDevice(self)

    def _configure(self, device, manufacturer, model, description, version, uri, serial):
        # Validate version code.
        buf = device.ctrl_transfer(0xc0, 51, data_or_wLength=2)
        assert (len(buf) == 2 and (buf[0] | buf[1] << 8) == 2)
        time.sleep(1)
        # Send accessory information.
        for i, data in enumerate(
                (manufacturer, model, description, version, uri, serial)):
            assert (device.ctrl_transfer(0x40, 52, wIndex=i, data_or_wLength=data) == len(data))
        # Put device into accessory mode.
        assert (device.ctrl_transfer(0x40, 53) == 0)
        usb.util.dispose_resources(device)

    def _detectEndpoints(self):
        configuration = self._device.get_active_configuration()
        interface = configuration[(0, 0)]

        ep_out = usb.util.find_descriptor(
            interface, custom_match=lambda e: \
                usb.util.endpoint_direction(e.bEndpointAddress) == \
                usb.util.ENDPOINT_OUT)
        ep_in = usb.util.find_descriptor(
            interface, custom_match=lambda e: \
                usb.util.endpoint_direction(e.bEndpointAddress) == \
                usb.util.ENDPOINT_IN)
        assert (ep_out and ep_in)
        return ep_out, ep_in

    def _write(self, id, cmd, data, timeout=1000):
        assert (self._device and self._endpoint_out)

        size = 0
        if data is not None:
            size = len(data)

        data_bytes = struct.pack("!2HI", id, cmd, size);

        assert (self._endpoint_out.write(data_bytes, timeout=timeout) == len(data_bytes))
        assert (self._endpoint_out.write(data, timeout=timeout) == len(data))

    def _read(self, _timeout=1000):
        '''
        :param timeout:
        :return: (service, what, content)
        '''
        assert (self._device and self._endpoint_in)
        try:
            read_bytes = self._endpoint_in.read(self._MAX_CONTENT_SIZE, timeout=_timeout)
            service, what, size = struct.unpack("!2HI", read_bytes[:8])
            _bytes = None
            if size > 0:
                # read_bytes = self._endpoint_in.read(size, timeout=_timeout)
                _bytes = read_bytes[8:]
            return service, what, _bytes

        except usb.core.USBError as e:
            #self._log("_read Error :" , e)
            raise e
            pass

    def _close(self):
        if self._isProcessing:
            self._isProcessing = False
            time.sleep(2)

    def _release(self):
        aad._log("Release all handle")
        self._threadProcess = None
        try:
            self._endpoint_out.write(array.array(self._B, [0, 0]))
            time.sleep(0.2)
            self._device.reset()  # exit accessory mode
        except:
            pass  # ignore this
        usb.util.dispose_resources(self._device)
        self._device = None
        self._endpoint_out = None
        self._endpoint_in = None

    def start(self, width, height, dpi):
        self._isProcessing = True
        self._threadProcess = threading.Thread(target=AndroidAccessoryDisplay.process, args=(self, width, height, dpi))
        # self._threadProcess.setDaemon(True)
        self._threadProcess.start()

    def getMediaData(self, timeout_sec=0.5):
        try:
            return self._media_data_queue.get(timeout=timeout_sec)
        except Queue.Empty:
            return None

    def putMediaData(self, data):
        try:
            self._media_data_queue.put_nowait(data)
        except Queue.Full:
            self._log("Queue Full discard")
            pass

    def inject_event(self, ie):
        """
        :param ie:
        :return:
        key Code
        {
            byte0 byte1 byte2 byte3 byte4 byte5 byte6 byte7
            byte0 --
               |--bit0:   Left Control
               |--bit1:   Left Shift
               |--bit2:   Left Alt
               |--bit3:   Left GUI
               |--bit4:   Right Control
               |--bit5:   Right Shift
               |--bit6:   Right Alt
               |--bit7:   Right GUI
            byte1 -- N/A(0)
            byte2--byte7 -- Key Code
        }

        MULTI TOUCH , From MicroSoft
        {
            REPORTID_MTOUCH = 1
            union
            {
                struct
                {
                    UCHAR  bStatus;
                    UCHAR  ContactId;
                    USHORT wXData;
                    USHORT wYData;
                    UCHAR  bStatus2;
                    UCHAR  ContactId2;
                    USHORT wXData2;
                    USHORT wYData2;
                    UCHAR  ActualCount;
                } InputReport;
                UCHAR RawInput[13];
            };
        }
        """

        if ie.type == 1 :
            # SDL code to android keycode ,
            _code = ie.getUsbKey(ie.code)
            _ctrl_mask = 0
            if ie.code == 306: # Left Ctrl
                _ctrl_mask = 0x01
            elif ie.code == 304: # Left Shift
                _ctrl_mask = 0x02
            elif ie.code == 308: # Left Alt
                _ctrl_mask = 0x04
            elif ie.code == 305: # Right Ctrl
                _ctrl_mask = 0x10
            elif ie.code == 303: # Right Shift
                _ctrl_mask = 0x20
            elif ie.code == 307: # Right Alt
                _ctrl_mask = 0x40
            else:
                pass

            if _ctrl_mask != 0: # function key event
                if ie.status != 0:
                    self._key_status |= _ctrl_mask
                else:
                    self._key_status &= ~_ctrl_mask
                _code = 0
            if ie.status == 0: # Key release
                _code = 0
            _bytes = struct.pack("8B", self._key_status, 0, _code, 0, 0, 0, 0, 0)

            self._device.ctrl_transfer(bmRequestType = 0x40,
                         bRequest = self.ACCESSORY_SEND_HID_EVENT,
                         wValue = self.KEYBOARD_DEVICE_ID,
                         wIndex = 0,
                         data_or_wLength = _bytes)

        elif ie.type == 2 :
            if ie.status == 0 and self._touch_status == 0: # no need send
                return
            self._touch_status = ie.status
            if self._support_touch == 1:
                _bytes = struct.pack("4B2H2B2HB",0x7F, 1, ie.status, 1, ie.x, ie.y, 0, 0, 0, 0, 1)
                _bytes = _bytes[1:] # need align
                self._device.ctrl_transfer(bmRequestType = 0x40,
                             bRequest = self.ACCESSORY_SEND_HID_EVENT,
                             wValue = self.TOUCH_DEVICE_ID,
                             wIndex = 0,
                             data_or_wLength = _bytes)
            elif self._support_touch == 2:
                _inrange = 0
                if ie.status !=0:
                    _inrange = 0x08
                _bytes = struct.pack("4B2H", 0x7F, 1, ie.status, 0, ie.x, ie.y)
                _bytes = _bytes[1:] # need align

                self._device.ctrl_transfer(bmRequestType = 0x40,
                             bRequest = self.ACCESSORY_SEND_HID_EVENT,
                             wValue = self.SINGLE_TOUCH_DEVICE_ID,
                             wIndex = 0,
                             data_or_wLength = _bytes)

    @staticmethod
    def process(aad, width, height, dpi):
        aad._process(width, height, dpi)
        aad._log("Process Exit")
        aad._release()

    def _process(self, width, height, dpi):
        self._isProcessing = True

        while self._isProcessing:
            try:
                id, cmd, _bytes = self._read()
                #self._log('Read in value:', id, cmd)
                if id != self._ID:
                    self._log("_ID Error")
                    continue
                if cmd == self._MSG_QUERY:
                    _bytes = struct.pack("!3I", width, height, dpi)
                    self._write(self._SOURCE_ID, self._MSG_SINK_AVAILABLE, _bytes)
                if cmd == self._MSG_QUERY_EXT:
                    phone_width, phone_height, phone_dpi = struct.unpack("!3I", _bytes)
                    _bytes = struct.pack("!3I", width, height, dpi)
                    self._write(self._SOURCE_ID, self._MSG_SINK_AVAILABLE, _bytes)
                elif cmd == self._MSG_CONTENT:
                    #self._log("Content Size : ", len(_bytes))
                    self.putMediaData(_bytes)
                else:
                    self._log("not Support")
            except usb.core.USBError as e:
                if e.errno == 110:  # Operation timed out.
                    # self._log("Read Timeout")
                    pass
                else:
                    self._isProcessing = False

    @property
    def isStop(self):
        return not self._isProcessing

    def stop(self):
        self._close()


class AndroidEventInject:
    EVENT_SIZE = 4 * 11

    _SDL2KeyEventDict = {
        8: 0,  # SDLK_BACKSPACE        = 8,
        9: 0x2B,  # SDLK_TAB        = 9,
        12: 28,  # SDLK_CLEAR        = 12,
        13: 0x28,  # SDLK_RETURN        = 13,
        19: 0x48,  # SDLK_PAUSE        = 19,
        27: 0x29,  # SDLK_ESCAPE        = 27,
        32: 0x2C,  # SDLK_SPACE        = 32,
        #   SDLK_EXCLAIM        = 33,
        #   SDLK_QUOTEDBL        = 34,
        35: 18,  # SDLK_HASH        = 35,
        #   SDLK_DOLLAR        = 36,
        #   SDLK_AMPERSAND        = 38,
        #   SDLK_QUOTE        = 39,
        40:0x2F,#   SDLK_LEFTPAREN        = 40,
        41:0x30,#   SDLK_RIGHTPAREN        = 41,
        #   SDLK_ASTERISK        = 42,
        #   SDLK_PLUS        = 43,
        #   SDLK_COMMA        = 44,
        45:0x2D,#   SDLK_MINUS        = 45,
        #   SDLK_PERIOD        = 46,
        47:0x31,#   SDLK_SLASH        = 47,
        48: 0x27, # SDLK_0            = 48,
        49: 0x1E,  # SDLK_1            = 49,
        50: 0x1F,  # SDLK_2            = 50,
        51: 0x20,  # SDLK_3            = 51,
        52: 0x21,  # SDLK_4            = 52,
        53: 0x22,  # SDLK_5            = 53,
        54: 0x23,  # SDLK_6            = 54,
        55: 0x24,  # SDLK_7            = 55,
        56: 0x25,  # SDLK_8            = 56,
        57: 0x26,  # SDLK_9            = 57,
        #   SDLK_COLON        = 58,
        #   SDLK_SEMICOLON        = 59,
        #   SDLK_LESS        = 60,
        61:0x2E,#   SDLK_EQUALS        = 61,
        #   SDLK_GREATER        = 62,
        #   SDLK_QUESTION        = 63,
        #   SDLK_AT            = 64,
        #   /*
        #      Skip uppercase letters
        #    */
        #   SDLK_LEFTBRACKET    = 91,
        #   SDLK_BACKSLASH        = 92,
        #   SDLK_RIGHTBRACKET    = 93,
        #   SDLK_CARET        = 94,
        #   SDLK_UNDERSCORE        = 95,
        #   SDLK_BACKQUOTE        = 96,
        97: 0x04,  # SDLK_a            = 97,
        98: 0x05,  # SDLK_b            = 98,
        99: 0x06,  # SDLK_c            = 99,
        100: 0x07,  # SDLK_d            = 100,
        101: 0x08,  # SDLK_e            = 101,
        102: 0x09,  # SDLK_f            = 102,
        103: 0x0A,  # SDLK_g            = 103,
        104: 0x0B,  # SDLK_h            = 104,
        105: 0x0C,  # SDLK_i            = 105,
        106: 0x0D,  # SDLK_j            = 106,
        107: 0x0E,  # SDLK_k            = 107,
        108: 0x0F,  # SDLK_l            = 108,
        109: 0x10,  # SDLK_m            = 109,
        110: 0x11,  # SDLK_n            = 110,
        111: 0x12,  # SDLK_o            = 111,
        112: 0x13,  # SDLK_p            = 112,
        113: 0x14,  # SDLK_q            = 113,
        114: 0x15,  # SDLK_r            = 114,
        115: 0x16,  # SDLK_s            = 115,
        116: 0x17,  # SDLK_t            = 116,
        117: 0x18,  # SDLK_u            = 117,
        118: 0x19,  # SDLK_v            = 118,
        119: 0x1A,  # SDLK_w            = 119,
        120: 0x1B,  # SDLK_x            = 120,
        121: 0x1C,  # SDLK_y            = 121,
        122: 0x1D,  # SDLK_z            = 122,
        123: 0x2A,  # SDLK_DELETE        = 127,
        #   /* End of ASCII mapped keysyms */


        #   /** @name Numeric keypad */
        #       /*@{*/
        #   SDLK_KP0        = 256,
        #   SDLK_KP1        = 257,
        #   SDLK_KP2        = 258,
        #   SDLK_KP3        = 259,
        #   SDLK_KP4        = 260,
        #   SDLK_KP5        = 261,
        #   SDLK_KP6        = 262,
        #   SDLK_KP7        = 263,
        #   SDLK_KP8        = 264,
        #   SDLK_KP9        = 265,
        #   SDLK_KP_PERIOD        = 266,
        #   SDLK_KP_DIVIDE        = 267,
        #   SDLK_KP_MULTIPLY    = 268,
        #   SDLK_KP_MINUS        = 269,
        #   SDLK_KP_PLUS        = 270,
        #   SDLK_KP_ENTER        = 271,
        #   SDLK_KP_EQUALS        = 272,
        #       /*@}*/

        #   /** @name Arrows + Home/End pad */
        #       /*@{*/
        273:0x52,#   SDLK_UP            = 273,
        274:0x51,#   SDLK_DOWN        = 274,
        275:0x4F,#   SDLK_RIGHT        = 275,
        276:0x50,#   SDLK_LEFT        = 276,
        277:0x49,#   SDLK_INSERT        = 277,
        278:0x4A,#   SDLK_HOME        = 278,
        279:0x4D,#   SDLK_END        = 279,
        280:0x4B,#   SDLK_PAGEUP        = 280,
        281:0x4E,#   SDLK_PAGEDOWN        = 281,
        #       /*@}*/

        #   /** @name Function keys */
        #       /*@{*/
        282:0x3A,#   SDLK_F1            = 282,
        283:0x3B,#   SDLK_F2            = 283,
        284:0x3C,#   SDLK_F3            = 284,
        285:0x3D,#   SDLK_F4            = 285,
        286:0x3E,#   SDLK_F5            = 286,
        287:0x3F,#   SDLK_F6            = 287,
        288:0x40,#   SDLK_F7            = 288,
        289:0x41,#   SDLK_F8            = 289,
        290:0x42,#   SDLK_F9            = 290,
        291:0x43,#   SDLK_F10        = 291,
        292:0x44,#   SDLK_F11        = 292,
        293:0x45,#   SDLK_F12        = 293,
        294:0x68,#   SDLK_F13        = 294,
        295:0x69,#   SDLK_F14        = 295,
        296:0x6A,#   SDLK_F15        = 296,
        #       /*@}*/

        #   /** @name Key state modifier keys */
        #       /*@{*/
        #   SDLK_NUMLOCK        = 300,
        #   SDLK_CAPSLOCK        = 301,
        #   SDLK_SCROLLOCK        = 302,
        #   SDLK_RSHIFT        = 303,
        #   SDLK_LSHIFT        = 304,
        #   SDLK_RCTRL        = 305,
        #   SDLK_LCTRL        = 306,
        #   SDLK_RALT        = 307,
        #   SDLK_LALT        = 308,
        #   SDLK_RMETA        = 309,
        #   SDLK_LMETA        = 310,
        #   SDLK_LSUPER        = 311,        /**< Left "Windows" key */
        #   SDLK_RSUPER        = 312,        /**< Right "Windows" key */
        #   SDLK_MODE        = 313,        /**< "Alt Gr" key */
        #   SDLK_COMPOSE        = 314,        /**< Multi-key compose key */
        #       /*@}*/

        #   /** @name Miscellaneous function keys */
        #       /*@{*/
        #   SDLK_HELP        = 315,
        #   SDLK_PRINT        = 316,
        #   SDLK_SYSREQ        = 317,
        #   SDLK_BREAK        = 318,
        #   SDLK_MENU        = 319,
        #   SDLK_POWER        = 320,        /**< Power Macintosh power key */
        #   SDLK_EURO        = 321,        /**< Some european keyboards */
        #   SDLK_UNDO        = 322,        /**< Atari keyboard has Undo */
    }
    KeyBoardReportDescriptor = bytearray([
        0x05, 0x01,  ## Usage Page (Generic Desktop)
        0x09, 0x06,  ## Usage (Keyboard)
        0xA1, 0x01,  ## Collection (Application)
        0x05, 0x07,  ##     Usage Page (Key Codes)
        0x19, 0xe0,  ##     Usage Minimum (224)
        0x29, 0xe7,  ##     Usage Maximum (231)
        0x15, 0x00,  ##     Logical Minimum (0)
        0x25, 0x01,  ##     Logical Maximum (1)
        0x75, 0x01,  ##     Report Size (1)
        0x95, 0x08,  ##     Report Count (8)
        0x81, 0x02,  ##     Input (Data, Variable, Absolute)

        0x95, 0x01,  ##     Report Count (1)
        0x75, 0x08,  ##     Report Size (8)
        0x81, 0x01,  ##     Input (Constant) reserved byte(1)

        0x95, 0x05,  ##     Report Count (5)
        0x75, 0x01,  ##     Report Size (1)
        0x05, 0x08,  ##     Usage Page (Page# for LEDs)
        0x19, 0x01,  ##     Usage Minimum (1)
        0x29, 0x05,  ##     Usage Maximum (5)
        0x91, 0x02,  ##     Output (Data, Variable, Absolute), Led report
        0x95, 0x01,  ##     Report Count (1)
        0x75, 0x03,  ##     Report Size (3)
        0x91, 0x01,  ##     Output (Data, Variable, Absolute), Led report padding

        0x95, 0x06,  ##     Report Count (6)
        0x75, 0x08,  ##     Report Size (8)
        0x15, 0x00,  ##     Logical Minimum (0)
        0x25, 0x65,  ##     Logical Maximum (101)
        0x05, 0x07,  ##     Usage Page (Key codes)
        0x19, 0x00,  ##     Usage Minimum (0)
        0x29, 0x65,  ##     Usage Maximum (101)
        0x81, 0x00,  ##     Input (Data, Array) Key array(6 bytes)


        0x09, 0x05,  ##     Usage (Vendor Defined)
        0x15, 0x00,  ##     Logical Minimum (0)
        0x26, 0xFF, 0x00,  ##     Logical Maximum (255)
        0x75, 0x08,  ##     Report Count (2)
        0x95, 0x02,  ##     Report Size (8 bit)
        0xB1, 0x02,  ##     Feature (Data, Variable, Absolute)

        0xC0  ## End Collection (Application)
    ])
    REPORTID_TOUCH = 1
    REPORTID_MTOUCH = 1
    REPORTID_MOUSE = 3
    REPORTID_FEATURE = 7
    REPORTID_MAX_COUNT = 8
    REPORTID_CAPKEY = 9
    MultTouchReportDescriptor = bytearray([
        # {
        0x05, 0x0d,	                        ## USAGE_PAGE (Digitizers)
        0x09, 0x04,	                        ## USAGE (Touch Screen)
        0xa1, 0x01,                         ## COLLECTION (Application)
        0x85, REPORTID_MTOUCH,              ##   REPORT_ID (Touch)
        0x09, 0x22,                         ##   USAGE (Finger)
        0xa1, 0x02,                         ##   COLLECTION (Logical)
        0x09, 0x42,                         ##     USAGE (Tip Switch)
        0x15, 0x00,                         ##     LOGICAL_MINIMUM (0)
        0x25, 0x01,                         ##     LOGICAL_MAXIMUM (1)
        0x75, 0x01,                         ##     REPORT_SIZE (1)
        0x95, 0x01,                         ##     REPORT_COUNT (1)
        0x81, 0x02,                         ##       INPUT (Data,Var,Abs)
        0x09, 0x32,	                        ##     USAGE (In Range)
        0x81, 0x02,                         ##       INPUT (Data,Var,Abs)
        0x95, 0x06,                         ##     REPORT_COUNT (6)
        0x81, 0x03,                         ##       INPUT (Cnst,Ary,Abs)
        0x75, 0x08,                         ##     REPORT_SIZE (8)
        0x09, 0x51,                         ##     USAGE (Contact Identifier)
        0x95, 0x01,                         ##     REPORT_COUNT (1)
        0x81, 0x02,                         ##       INPUT (Data,Var,Abs)
        0x05, 0x01,                         ##     USAGE_PAGE (Generic Desk..
        0x26, 0xff, 0x7f,                   ##     LOGICAL_MAXIMUM (768)        NOTE: ADJUST LOGICAL MAXIMUM FOR X BASED ON TOUCH SCREEN RESOLUTION!
        0x75, 0x10,                         ##     REPORT_SIZE (16)
        0x55, 0x0e,                         ##     UNIT_EXPONENT (-2)
        0x65, 0x13,                         ##     UNIT (Inch,EngLinear)
        0x09, 0x30,                         ##     USAGE (X)
        0x35, 0x00,                         ##     PHYSICAL_MINIMUM (0)
        0x46, 0x00, 0x00,                   ##     PHYSICAL_MAXIMUM (232)       NOTE: ADJUST PHYSICAL MAXIMUM FOR X BASED ON TOUCH SCREEN DIMENSION (100ths of an inch)!
        0x81, 0x02,                         ##       INPUT (Data,Var,Abs)
        0x46, 0x00, 0x00,                   ##     PHYSICAL_MAXIMUM (382)       NOTE: ADJUST PHYSICAL MAXIMUM FOR Y BASED ON TOUCH SCREEN DIMENSION (100ths of an inch)!
        0x26, 0xff, 0x7f,                   ##     LOGICAL_MAXIMUM (1280)       NOTE: ADJUST LOGICAL MAXIMUM FOR Y BASED ON TOUCH SCREEN RESOLUTION!
        0x09, 0x31,                         ##     USAGE (Y)
        0x81, 0x02,                         ##       INPUT (Data,Var,Abs)
        0xc0,                               ##   END_COLLECTION
        0x05, 0x0d,	                        ##   USAGE_PAGE (Digitizers)
        0x09, 0x54,	                        ##   USAGE (Actual count)
        0x95, 0x01,                         ##   REPORT_COUNT (1)
        0x75, 0x08,                         ##   REPORT_SIZE (8)
        0x81, 0x02,                         ##     INPUT (Data,Var,Abs)
        0x85, REPORTID_MAX_COUNT,           ##   REPORT_ID (Feature)
        0x09, 0x55,                         ##   USAGE(Maximum Count)
        0x25, 0x02,                         ##   LOGICAL_MAXIMUM (2)
        0xb1, 0x02,                         ##   FEATURE (Data,Var,Abs)
        0xc0,                               ## END_COLLECTION
        0x09, 0x0E,                         ## USAGE (Configuration)
        0xa1, 0x01,                         ## COLLECTION (Application)
        0x85, REPORTID_FEATURE,             ##   REPORT_ID (Feature)
        0x09, 0x22,                         ##   USAGE (Finger)
        0xa1, 0x00,                         ##   COLLECTION (physical)
        0x09, 0x52,                         ##     USAGE (Input Mode)
        0x09, 0x53,                         ##     USAGE (Device Index
        0x15, 0x00,                         ##     LOGICAL_MINIMUM (0)
        0x25, 0x0a,                         ##     LOGICAL_MAXIMUM (10)
        0x75, 0x08,                         ##     REPORT_SIZE (8)
        0x95, 0x02,                         ##     REPORT_COUNT (2)
        0xb1, 0x02,                         ##     FEATURE (Data,Var,Abs)
        0xc0,                               ##   END_COLLECTION
        0xc0,                               ## END_COLLECTION
        0x05, 0x01,                         ## USAGE_PAGE (Generic Desktop)
        0x09, 0xEE,                         ## USAGE (HID_USAGE_KEYBOARD_MOBILE)
        0xa1, 0x01,                         ## COLLECTION (Application)
        0x85, REPORTID_CAPKEY,              ##   REPORT_ID
        0x05, 0x07,                         ##   USAGE_PAGE (Key Codes)
        0x09, 0x3B,                         ##   USAGE(F2 Key)  - Start/Home
        0x09, 0x3C,                         ##   USAGE(F3 Key)  - Search
        0x09, 0x29,                         ##   USAGE(Esc Key) - Back
        0x15, 0x00,                         ##   LOGICAL_MINIMUM (0)
        0x25, 0x01,                         ##   LOGICAL_MAXIMUM (1)
        0x75, 0x01,                         ##   REPORT_SIZE (1)
        0x95, 0x03,                         ##   REPORT_COUNT (3)
        0x81, 0x02,                         ##   INPUT (Data,Var,Abs)
        0x95, 0x1d,                         ##   REPORT_COUNT (29)
        0x81, 0x03,                         ##   INPUT (Cnst,Var,Abs)
        0xc0,                               ## END_COLLECTION
        # };
    ])

    SingleTouchReportDescriptor = bytearray([
        0x05, 0x0d,                    ##// USAGE_PAGE (Digitizers)
        0x09, 0x04,                    ##// USAGE (Touch Screen)
        0xa1, 0x01,                    ##// COLLECTION (Application)
        0x85, 0x01,                    ##//   REPORT_ID (1)
        0x09, 0x20,                    ##//   USAGE (Stylus)
        0xa1, 0x00,                    ##//   COLLECTION (Physical)
        0x09, 0x42,                    ##//     USAGE (Tip Switch)
        0x15, 0x00,                    ##//     LOGICAL_MINIMUM (0)
        0x25, 0x01,                    ##//     LOGICAL_MAXIMUM (1)
        0x75, 0x01,                    ##//     REPORT_SIZE (1)
        0x95, 0x01,                    ##//     REPORT_COUNT (1)
        0x81, 0x02,                    ##//     INPUT (Data,Var,Abs)
        0x95, 0x03,                    ##//     REPORT_COUNT (3)
        0x81, 0x01,                    ##//     INPUT (Cnst,Ary,Abs)
        0x09, 0x32,                    ##//     USAGE (In Range)
        0x09, 0x37,                    ##//     USAGE (Data Valid)
        0x95, 0x02,                    ##//     REPORT_COUNT (2)
        0x81, 0x03,                    ##//     INPUT (Cnst,Var,Abs)
        0x95, 0x0a,                    ##//     REPORT_COUNT (10)
        0x81, 0x01,                    ##//     INPUT (Cnst,Ary,Abs)
        0x05, 0x01,                    ##//     USAGE_PAGE (Generic Desktop)
        0x15, 0x00,                    ##//     LOGICAL_MINIMUM (0)
        0x26, 0xff, 0x7f,              ##//     LOGICAL_MAXIMUM (32767)
        0x75, 0x10,                    ##//     REPORT_SIZE (16)
        0x95, 0x01,                    ##//     REPORT_COUNT (1)
        0xa4,                          ##//     PUSH
        0x55, 0x0d,                    ##//     UNIT_EXPONENT (-3)
        0x65, 0x00,                    ##//     UNIT (None)
        0x09, 0x30,                    ##//     USAGE (X)
        0x35, 0x00,                    ##//     PHYSICAL_MINIMUM (0)
        0x45, 0x00,                    ##//     PHYSICAL_MAXIMUM (0)
        0x81, 0x02,                    ##//     INPUT (Data,Var,Abs)
        0x09, 0x31,                    ##//     USAGE (Y)
        0x35, 0x00,                    ##//     PHYSICAL_MINIMUM (0)
        0x45, 0x00,                    ##//     PHYSICAL_MAXIMUM (0)
        0x81, 0x02,                    ##//     INPUT (Data,Var,Abs)
        0xb4,                          ##//     POP
        0xc0,                           ##//     END_COLLECTION
        0xc0                           ##//     END_COLLECTION
    ])

    def __init__(self, rawbytes):
        """
        :param rawbytes:
        :return:
         	struct inject_event_struct {
                int type; // 1 = key , 2 = mouse, 3 = screen info
                int status; // 1 = down , 0 = up
                int code;   // key sym
                int x;
                int y;
                // display info
                int screen_w;
                int screen_h;
                int video_x;
                int video_y;
                int video_w;
                int video_h;
            };
        """
        _data = struct.unpack("11i", rawbytes)
        #print("Event : " , _data)
        self.type = _data[0]
        self.status = _data[1]
        self.code = _data[2]
        self.x = _data[3]
        self.y = _data[4]

        if self.type == 1:
            pass
        elif self.type == 2:
            try:
                _v_x = _data[7]
                _v_y = _data[8]
                _v_w = _data[9]
                _v_h = _data[10]
                self.x = (int)(1.0 * (self.x - _v_x) * 32768 / _v_w)
                self.y = (int)(1.0 * (self.y - _v_y) * 32768 / _v_h)
                if self.x < 0 or self.y < 0 or self.x > 32768 or self.y > 32768:
                    self.type = 0
                #print("Event Touch : ",self.type, self.status, self.code, self.x,self.y )
            except Exception as e:
                self.type = 0

    def getUsbKey(self, _sdl_key):
        try:
            return self._SDL2KeyEventDict[_sdl_key]
        except:
            pass
        return 0

if __name__ == '__main__':
    from signal import signal, SIGTERM, SIGINT

    _VIDEO_PIPE = "/tmp/android_accessory_display.pipe"
    _INJECT_PIPE = _VIDEO_PIPE + "_ev"


    def _log(*args):
        print(args)


    _TIMEOUT_COUNT = 5
    shutdown = False
    # create video fifo
    try:
        os.mkfifo(_VIDEO_PIPE)
        os.mkfifo(_INJECT_PIPE)
    except OSError as e:
        # already create , ignore
        # _log(e)
        pass
    write_pipe = os.open(_VIDEO_PIPE, os.O_SYNC | os.O_CREAT | os.O_RDWR | os.O_NONBLOCK)
    read_event_pipe = None


    def signal_handler(signal, frame):
        global shutdown
        shutdown = True


    for signum in (SIGTERM, SIGINT):
        signal(signum, signal_handler)


    def _process_inject_event(_aad):
        os.open(_INJECT_PIPE, os.O_SYNC | os.O_CREAT | os.O_RDONLY | os.O_NONBLOCK)
        while not _aad.isStop:
            try:
                read_bytes = os.read(read_event_pipe, AndroidEventInject.EVENT_SIZE)
                if _aad.isStop:
                    break
                if read_bytes is not None and len(read_bytes) == AndroidEventInject.EVENT_SIZE:
                    _aad.inject_event(AndroidEventInject(read_bytes))
                else:
                    #time.sleep(0.1)
                    pass
            except Exception as e:
                #time.sleep(0.1)
                pass


    _threadProcess_inject_event = None

    while not shutdown:
        try:
            with AndroidAccessoryDisplay(
                    log=_log,
                    serial='SOFTSHOW012345678') as aad:
                print('AndroidAccessoryDisplay start Screen')
                #aad.start(240, 320, 96)
                # aad.start(720, 1280, 96)
                aad.start(480, 640, 96)

                while (not shutdown) and (not aad.isStop):
                    _bytes = aad.getMediaData()
                    if _bytes is None:
                        continue
                    _left_size  = len(_bytes)
                    while _left_size > 0:
                        if aad.isStop:
                            break
                        try:
                            # print('USB Write...')
                            _write_size = os.write(write_pipe, _bytes)
                            _left_size -= _write_size
                            _bytes = _bytes[_write_size:]
                        except Exception as e:
                            _log("write to fifo error :", len(_bytes),e)
                            pass

                    # read event from ffmpeg
                    if _threadProcess_inject_event is None:
                        print('AndroidAccessoryDisplay started EventInject')
                        _threadProcess_inject_event = threading.Thread(target=_process_inject_event, args=(aad,))
                        _threadProcess_inject_event.start()

                print('USB r/w error Restarting…')
                _threadProcess_inject_event = None
                aad.stop()
                time.sleep(2)
        except usb.core.USBError as e:
            print(e.args)
            time.sleep(2)
            print('USBError occurred. Restarting…')
    try:
        os.close(write_pipe)
        os.close(read_event_pipe)
    except:
        pass  # ignore this
