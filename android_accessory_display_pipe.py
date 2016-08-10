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

    # send to Source
    _SOURCE_ID = 2
    _MSG_SINK_AVAILABLE = 1
    _MSG_SINK_NOT_AVAILABLE = 2
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
                    _bytes = struct.pack("!3I",width, height,dpi );
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

if __name__ == '__main__':
    from signal import signal, SIGTERM, SIGINT
    _VIDEO_PIPE = "/tmp/android_accessory_display.pipe"
    def _log(*args):
        print(args)
    _TIMEOUT_COUNT = 5
    shutdown = False
    # create video fifo
    try:
        os.mkfifo(_VIDEO_PIPE)
    except OSError as e:
        # already create , ignore
        #self._log(e)
        pass
    write_pipe = os.open(_VIDEO_PIPE, os.O_SYNC | os.O_CREAT | os.O_RDWR )
    def signal_handler(signal, frame):
        global shutdown
        shutdown = True

    for signum in (SIGTERM, SIGINT):
        signal(signum, signal_handler)

    while not shutdown:
        try:
            with AndroidAccessoryDisplay(
                    log=_log,
                    serial='SOFTSHOW012345678') as aad:
                #aad.start(240, 320, 240)
                #aad.start(720, 1280, 240)
                aad.start(480, 640, 240)
                print('AndroidAccessoryDisplay started')
                while (not shutdown) and (not aad.isStop):
                    _bytes = aad.getMediaData()
                    if aad.isStop:
                        break
                    if _bytes is not None :
                        #print('USB Write...')
                        os.write(write_pipe, _bytes)
                print('USB r/w error Restarting…')
                aad.stop()
        except usb.core.USBError as e:
            print(e.args)
            time.sleep(2)
            print('USBError occurred. Restarting…')
    try :
        os.close(write_pipe)
    except :
        pass # ignore this
