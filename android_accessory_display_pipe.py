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
	   prebuilt ffplay support video sync , it show slow then phone.
	   fixed it(not use video sync) : get ffmpeg source 
		 modify ffplay.c like follow :
			time= av_gettime_relative()/1000000.0;
            if (time < is->frame_timer + delay) {
                *remaining_time = FFMIN(is->frame_timer + delay - time, *remaining_time);
                //goto display; // == mask this line
            }

            is->frame_timer += delay;


"""

import array
import sys
import time
import usb
import struct
import threading
import os
import Queue
#import pygst

class AndroidAccessoryDisplay :
    _B = 'B' if sys.version_info.major == 3 else b'B'
    _BIT_RATE = 6000000
    _FRAME_RATE = 30
    _I_FRAME_INTERVAL = 10

    _MAX_CONTENT_SIZE = 8 * 64 * 1024 # com.android.accessorydisplay.common.Protocol.MAX_ENVELOPE_SIZE
    _HEADER_SIZE = 8

    _ERR_ID = -1
    _ERR_TIMEOUT = 1

    #send to Sink
    _ID = 1
    _MSG_QUERY = 1
    _MSG_CONTENT = 2 # Send MPEG2-TS H.264 encoded content.

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

    # when auto detect failed ,add vid, pid here
    _android_device_list = [
        #[Vendor ID, (Product ID, Product ID ... Product ID) ],
        [_accessory_vid,(_accessory_pid,)], # accessory mode
        [0x2717,(0xff68,0xff48)],
        [0x18d1,(0x4e22,0x4e20,0x4e30,0xd002,0x4e42,0x4e40,0x4ee2,0x4ee0,0x4ee1) ],
    ]

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self._close()

    def __init__(self,log, serial) :
        self._media_data_queue = Queue.Queue(maxsize = 50)
        self._log = log
        self._isProcessing = False
        self._threadProcess = None
        self.phone_width = 720
        self.phone_height = 1280
        self.phone_dpi = 240
        device = self._autoDetectDevice()
        if device is None :
            raise usb.core.USBError('Device not connected')

        self._vendor_id = device.idVendor
        self._product_id = device.idProduct

        if self._vendor_id == self._accessory_vid and self._product_id == self._accessory_pid :
            # alread in accessory mode  , but maybe it not in accessory display mode, reset it
            device.reset()
            time.sleep(5)
            # detect again
            device = self._autoDetectDevice()
            if device is None :
                raise usb.core.USBError('Device not connected')
            self._vendor_id = device.idVendor
            self._product_id = device.idProduct

        # now device is un-configure device, configure it
        self._log("configure Device")
        attempts_left = 5
        while attempts_left:
            try :
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
            self._device = usb.core.find(idVendor=self._accessory_vid,idProduct=self._accessory_pid)
            if self._device is not None:
                break;
            time.sleep(1)
            attempts_left -= 1
        if self._device is None :
            device.reset()
            raise usb.core.USBError('Device Configure Error')

        self._endpoint_out, self._endpoint_in = self._detectEndpoints()



    def _autoDetectDevice(self):
        #auto detect
        devs = usb.core.find(find_all=True)
        for dev in devs:
            for cfg in dev:
                for i in cfg:
                    if i.bNumEndpoints == 2\
                        and i.bInterfaceClass == 0xff\
                        and i.bInterfaceSubClass == 0x42\
                        and i.bInterfaceProtocol == 0x01 :
                        return i.device
        #auto detect failed , use device list
        def _detectDevice(me):
            for device in me._android_device_list :
                vid = device[0]
                for pid in device[1] :
                    android_device = usb.core.find(idVendor=vid, idProduct=pid)
                    if android_device :
                        return android_device
            return None
        return _detectDevice(self)

    def _configure(self,device,manufacturer, model, description, version, uri, serial) :
        # Validate version code.
        buf = device.ctrl_transfer(0xc0, 51, data_or_wLength=2)
        assert(len(buf) == 2 and (buf[0] | buf[1] << 8) == 2)
        time.sleep(1)
        # Send accessory information.
        for i, data in enumerate(
            (manufacturer, model, description, version, uri, serial)):
            assert(device.ctrl_transfer(0x40, 52, wIndex=i, data_or_wLength=data) == len(data))
        # Put device into accessory mode.
        assert(device.ctrl_transfer(0x40, 53) == 0)
        usb.util.dispose_resources(device)

    def _detectEndpoints(self):
        configuration = self._device.get_active_configuration()
        interface = configuration[(0, 0)]

        ep_out = usb.util.find_descriptor(
            interface, custom_match= lambda e:\
                        usb.util.endpoint_direction(e.bEndpointAddress) ==\
                        usb.util.ENDPOINT_OUT)
        ep_in = usb.util.find_descriptor(
            interface, custom_match=lambda e:\
                        usb.util.endpoint_direction(e.bEndpointAddress) ==\
                        usb.util.ENDPOINT_IN)
        assert(ep_out and ep_in)
        return ep_out, ep_in

    def _write(self,id, cmd,data, timeout=1000):
        assert(self._device and self._endpoint_out)

        size = 0
        if data is not None:
            size = len(data)

        data_bytes = struct.pack("!2HI", id, cmd, size);

        assert(self._endpoint_out.write(data_bytes, timeout=timeout) == len(data_bytes))
        assert(self._endpoint_out.write(data, timeout=timeout) == len(data))

    def _read(self, _timeout=1000):
        '''
        :param timeout:
        :return: (service, what, content)
        '''
        assert(self._device and self._endpoint_in)
        try:
            read_bytes = self._endpoint_in.read(self._MAX_CONTENT_SIZE, timeout=_timeout)
            service, what, size = struct.unpack("!2HI", read_bytes[:8])
            _bytes = None
            if size > 0 :
                #read_bytes = self._endpoint_in.read(size, timeout=_timeout)
                _bytes = read_bytes[8:]
            return service, what, _bytes

        except usb.core.USBError as e:
            raise e

    def _close(self):
        if self._isProcessing:
            self._isProcessing = False
            time.sleep(2)

    def _release(self):
        aad._log("Release all handle")
        self._threadProcess = None
        try :
            self._endpoint_out.write(array.array(self._B, [0, 0]))
            time.sleep(0.2)
            self._device.reset() # exit accessory mode
        except :
            pass # ignore this
        usb.util.dispose_resources(self._device)
        self._device = None
        self._endpoint_out = None
        self._endpoint_in = None



    def start(self,width, height,dpi):
        self._isProcessing = True
        self._threadProcess = threading.Thread(target=AndroidAccessoryDisplay.process,args=(self,width, height,dpi))
        #self._threadProcess.setDaemon(True)
        self._threadProcess.start()

    def getMediaData(self,timeout_sec = 0.5):
        try:
            return self._media_data_queue.get(timeout=timeout_sec)
        except Queue.Empty:
            return None

    def putMediaData(self,data):
        try:
            self._media_data_queue.put_nowait(data)
        except Queue.Full:
            self._log("Queue Full discard")
            pass

    def inject_event(self, ie):
        _bytes = struct.pack("!5H",ie.type, ie.status, ie.code, ie.x, ie.y)
        #self._log("inject_event : " ,ie.type, ie.status, ie.code, ie.x, ie.y)
        #if (ie.code is not None) and (ie.code != 0):
        #    self._write(self._SOURCE_ID,self._MSG_SINK_INJECT_EVENT,_bytes)

        pass

    @staticmethod
    def process(aad,width, height,dpi):
        aad._process(width, height,dpi)
        aad._log("Process Exit")
        aad._release()

    def _process(self,width, height,dpi):
        self._isProcessing = True

        while self._isProcessing:
            try :
                id, cmd, _bytes = self._read()
                self._log('Read in value:', id,cmd)
                if id != self._ID :
                    self._log("_ID Error")
                    continue
                if cmd == self._MSG_QUERY :
                    _bytes = struct.pack("!3I",width, height,dpi )
                    self._write(self._SOURCE_ID,self._MSG_SINK_AVAILABLE,_bytes)
                if cmd == self._MSG_QUERY_EXT :
                    phone_width, phone_height,phone_dpi = struct.unpack("!3I",_bytes )
                    _bytes = struct.pack("!3I",width, height,dpi )
                    self._write(self._SOURCE_ID,self._MSG_SINK_AVAILABLE,_bytes)
                elif cmd == self._MSG_CONTENT :
                    self._log("Content Size : ", len(_bytes))
                    self.putMediaData(_bytes)
                else:
                    self._log("not Support")
            except usb.core.USBError as e:
                if e.errno == 110:  # Operation timed out.
                    #self._log("Read Timeout")
                    pass
                else :
                    self._isProcessing = False

    @property
    def isStop(self):
        return not self._isProcessing

    def stop(self):
        self._close()


class AndroidEventInject:
    EVENT_SIZE = 10
    SDL2KeyEventDict = {
        8:0,#   SDLK_BACKSPACE		= 8,
	    9:61,#   SDLK_TAB		= 9,
	    12:28,#  SDLK_CLEAR		= 12,
	    13:66,#  SDLK_RETURN		= 13,
	    19:0,#  SDLK_PAUSE		= 19,
        27:111,#   SDLK_ESCAPE		= 27,
        32:62,#   SDLK_SPACE		= 32,
        #   SDLK_EXCLAIM		= 33,
        #   SDLK_QUOTEDBL		= 34,
        35:18,#   SDLK_HASH		= 35,
        #   SDLK_DOLLAR		= 36,
        #   SDLK_AMPERSAND		= 38,
        #   SDLK_QUOTE		= 39,
        #   SDLK_LEFTPAREN		= 40,
        #   SDLK_RIGHTPAREN		= 41,
        #   SDLK_ASTERISK		= 42,
        #   SDLK_PLUS		= 43,
        #   SDLK_COMMA		= 44,
        #   SDLK_MINUS		= 45,
        #   SDLK_PERIOD		= 46,
        #   SDLK_SLASH		= 47,
        48:7,#   SDLK_0			= 48,
        49:8,#   SDLK_1			= 49,
        50:9,#   SDLK_2			= 50,
        51:10,#   SDLK_3			= 51,
        52:11,#   SDLK_4			= 52,
        53:12,#   SDLK_5			= 53,
        54:13,#   SDLK_6			= 54,
        55:14,#   SDLK_7			= 55,
        56:15,#   SDLK_8			= 56,
        57:16,#   SDLK_9			= 57,
        #   SDLK_COLON		= 58,
        #   SDLK_SEMICOLON		= 59,
        #   SDLK_LESS		= 60,
        #   SDLK_EQUALS		= 61,
        #   SDLK_GREATER		= 62,
        #   SDLK_QUESTION		= 63,
        #   SDLK_AT			= 64,
        #   /*
        #      Skip uppercase letters
        #    */
        #   SDLK_LEFTBRACKET	= 91,
        #   SDLK_BACKSLASH		= 92,
        #   SDLK_RIGHTBRACKET	= 93,
        #   SDLK_CARET		= 94,
        #   SDLK_UNDERSCORE		= 95,
        #   SDLK_BACKQUOTE		= 96,
        97:29,#   SDLK_a			= 97,
        98:30,#   SDLK_b			= 98,
        99:31,#   SDLK_c			= 99,
        100:32,#   SDLK_d			= 100,
        101:33,#   SDLK_e			= 101,
        102:34,#   SDLK_f			= 102,
        103:35,#   SDLK_g			= 103,
        104:36,#   SDLK_h			= 104,
        105:37,#   SDLK_i			= 105,
        106:38,#   SDLK_j			= 106,
        107:39,#   SDLK_k			= 107,
        108:40,#   SDLK_l			= 108,
        109:41,#   SDLK_m			= 109,
        110:42,#   SDLK_n			= 110,
        111:43,#   SDLK_o			= 111,
        112:44,#   SDLK_p			= 112,
        113:45,#   SDLK_q			= 113,
        114:46,#   SDLK_r			= 114,
        115:47,#   SDLK_s			= 115,
        116:48,#   SDLK_t			= 116,
        117:49,#   SDLK_u			= 117,
        118:50,#   SDLK_v			= 118,
        119:51,#   SDLK_w			= 119,
        120:52,#   SDLK_x			= 120,
        121:53,#   SDLK_y			= 121,
        122:54,#   SDLK_z			= 122,
        123:67,#   SDLK_DELETE		= 127,
        #   /* End of ASCII mapped keysyms */


        #   /** @name Numeric keypad */
        #       /*@{*/
        #   SDLK_KP0		= 256,
        #   SDLK_KP1		= 257,
        #   SDLK_KP2		= 258,
        #   SDLK_KP3		= 259,
        #   SDLK_KP4		= 260,
        #   SDLK_KP5		= 261,
        #   SDLK_KP6		= 262,
        #   SDLK_KP7		= 263,
        #   SDLK_KP8		= 264,
        #   SDLK_KP9		= 265,
        #   SDLK_KP_PERIOD		= 266,
        #   SDLK_KP_DIVIDE		= 267,
        #   SDLK_KP_MULTIPLY	= 268,
        #   SDLK_KP_MINUS		= 269,
        #   SDLK_KP_PLUS		= 270,
        #   SDLK_KP_ENTER		= 271,
        #   SDLK_KP_EQUALS		= 272,
        #       /*@}*/

        #   /** @name Arrows + Home/End pad */
        #       /*@{*/
        #   SDLK_UP			= 273,
        #   SDLK_DOWN		= 274,
        #   SDLK_RIGHT		= 275,
        #   SDLK_LEFT		= 276,
        #   SDLK_INSERT		= 277,
        #   SDLK_HOME		= 278,
        #   SDLK_END		= 279,
        #   SDLK_PAGEUP		= 280,
        #   SDLK_PAGEDOWN		= 281,
        #       /*@}*/

        #   /** @name Function keys */
        #       /*@{*/
        #   SDLK_F1			= 282,
        #   SDLK_F2			= 283,
        #   SDLK_F3			= 284,
        #   SDLK_F4			= 285,
        #   SDLK_F5			= 286,
        #   SDLK_F6			= 287,
        #   SDLK_F7			= 288,
        #   SDLK_F8			= 289,
        #   SDLK_F9			= 290,
        #   SDLK_F10		= 291,
        #   SDLK_F11		= 292,
        #   SDLK_F12		= 293,
        #   SDLK_F13		= 294,
        #   SDLK_F14		= 295,
        #   SDLK_F15		= 296,
        #       /*@}*/

        #   /** @name Key state modifier keys */
        #       /*@{*/
        #   SDLK_NUMLOCK		= 300,
        #   SDLK_CAPSLOCK		= 301,
        #   SDLK_SCROLLOCK		= 302,
        #   SDLK_RSHIFT		= 303,
        #   SDLK_LSHIFT		= 304,
        #   SDLK_RCTRL		= 305,
        #   SDLK_LCTRL		= 306,
        #   SDLK_RALT		= 307,
        #   SDLK_LALT		= 308,
        #   SDLK_RMETA		= 309,
        #   SDLK_LMETA		= 310,
        #   SDLK_LSUPER		= 311,		/**< Left "Windows" key */
        #   SDLK_RSUPER		= 312,		/**< Right "Windows" key */
        #   SDLK_MODE		= 313,		/**< "Alt Gr" key */
        #   SDLK_COMPOSE		= 314,		/**< Multi-key compose key */
        #       /*@}*/

        #   /** @name Miscellaneous function keys */
        #       /*@{*/
        #   SDLK_HELP		= 315,
        #   SDLK_PRINT		= 316,
        #   SDLK_SYSREQ		= 317,
        #   SDLK_BREAK		= 318,
        #   SDLK_MENU		= 319,
        #   SDLK_POWER		= 320,		/**< Power Macintosh power key */
        #   SDLK_EURO		= 321,		/**< Some european keyboards */
    	#   SDLK_UNDO		= 322,		/**< Atari keyboard has Undo */
    }
    def __init__(self, rawbytes) :
        self.type, self.status, self.code, self.x, self.y = struct.unpack("5H", rawbytes)
        if self.type == 1:
            #SDL code to android keycode ,
            try :
                self.code = self.SDL2KeyEventDict[self.code]
            except KeyError :
                self.code = 0




if __name__ == '__main__':
    from signal import signal, SIGTERM, SIGINT
    _VIDEO_PIPE = "/tmp/android_accessory_display.pipe"
    _INJECT_PIPE = _VIDEO_PIPE+"_ev"
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
        #_log(e)
        pass
    write_pipe = os.open(_VIDEO_PIPE, os.O_SYNC | os.O_CREAT | os.O_RDWR | os.O_NONBLOCK )
    read_event_pipe = os.open(_INJECT_PIPE, os.O_SYNC | os.O_CREAT | os.O_RDONLY)

    def signal_handler(signal, frame):
        global shutdown
        shutdown = True

    for signum in (SIGTERM, SIGINT):
        signal(signum, signal_handler)

    def _process_inject_event(_aad):
        while not _aad.isStop :
            read_bytes = os.read(read_event_pipe,AndroidEventInject.EVENT_SIZE)
            if _aad.isStop :
                break
            if read_bytes is not None and len(read_bytes) == AndroidEventInject.EVENT_SIZE:
                _aad.inject_event(AndroidEventInject(read_bytes))
            else:
                time.sleep(0.1)

    _threadProcess_inject_event = None

    while not shutdown:
        try:
            with AndroidAccessoryDisplay(
                    log=_log,
                    serial='SOFTSHOW012345678') as aad:
                print('AndroidAccessoryDisplay start Screen')
                aad.start(240, 320, 96)
                #aad.start(720, 1280, 96)
                #aad.start(480, 640, 96)

                while (not shutdown) and (not aad.isStop):
                    _bytes = aad.getMediaData()
                    if aad.isStop:
                        break
                    if _bytes is not None :
                        try:
                            #print('USB Write...')
                            os.write(write_pipe, _bytes)
                        except:
                            _log(e)
                        # read event from ffmpeg
                        if _threadProcess_inject_event is None:
                            print('AndroidAccessoryDisplay started EventInject')
                            _threadProcess_inject_event = threading.Thread(target=_process_inject_event,args=(aad,))
                            _threadProcess_inject_event.start()

                print('USB r/w error Restarting…')
                _threadProcess_inject_event = None
                aad.stop()
                time.sleep(2)
        except usb.core.USBError as e:
            print(e.args)
            time.sleep(2)
            print('USBError occurred. Restarting…')
    try :
        os.close(write_pipe)
        os.close(read_event_pipe)
    except :
        pass # ignore this
