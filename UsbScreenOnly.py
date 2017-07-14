#!/usr/bin/python
# coding=utf-8

"""
 python 2.7 & ubuntu 14.04LTS
 libusb:  https://github.com/walac/pyusb
 permission
    /etc/udev/rules.d/51-android.rules
    SUBSYSTEM=="usb", ATTR{idVendor}=="18d1", ATTR{idProduct}=="2d01", MODE="0666", OWNER="<username>"
    SUBSYSTEM=="usb", ATTR{idVendor}=="2717", ATTR{idProduct}=="ff68", MODE="0666", OWNER="<username>"
    SUBSYSTEM=="usb", ATTR{idVendor}=="2717", ATTR{idProduct}=="ff48", MODE="0666", OWNER="<username>"
    or used sudo when permission exception

"""

import array
import sys
import time
import usb
import struct
import threading
import os
import Queue
import getopt


class AndroidAccessoryDisplay:
    _B = 'B' if sys.version_info.major == 3 else b'B'
    _BIT_RATE = 6000000
    _FRAME_RATE = 30
    _I_FRAME_INTERVAL = 10

    _MAX_CONTENT_SIZE = 8 * 64 * 1024  # com.android.accessorydisplay.common.Protocol.MAX_ENVELOPE_SIZE
    _HEADER_SIZE = 8

    _INTERNAL_ID = 0
    _INTERNAL_CMD_TRYAGAIN = 1

    # send to Sink
    _ID = 1
    _MSG_QUERY = 1
    _MSG_CONTENT = 2  # Send MPEG2-TS H.264 encoded content.

    _MSG_QUERY_EXT = 100

    # send to Source
    _SOURCE_ID = 2
    _MSG_SINK_AVAILABLE = 1
    _MSG_SINK_NOT_AVAILABLE = 2

    _manufacturer = "Android"
    _model = "Accessory Display"
    _description = "Mirror Screen to Accessory Display"
    _version = 1
    _uri = "frameworks/base/tests/AccessoryDisplay"

    _accessory_pid = 0x2d01
    _accessory_vid = 0x18d1


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

        self._remain_data = array.array(self._B, [])
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
            if len(self._remain_data) >= self._HEADER_SIZE:
                service, what, size = struct.unpack("!2HI", self._remain_data[:self._HEADER_SIZE])
                if size == 0:
                    self._remain_data = self._remain_data[self._HEADER_SIZE:]
                    return service, what, None
                elif size <= len(self._remain_data) - self._HEADER_SIZE:
                    _bytes = self._remain_data[self._HEADER_SIZE : self._HEADER_SIZE + size]
                    self._remain_data = self._remain_data[self._HEADER_SIZE + size:]
                    return service, what, _bytes

            _buffer_bytes = self._endpoint_in.read(self._MAX_CONTENT_SIZE, timeout=_timeout)
            self._remain_data = self._remain_data + _buffer_bytes
            return self._INTERNAL_ID, self._INTERNAL_CMD_TRYAGAIN, None

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
                if id == self._INTERNAL_ID:
                    pass
                elif id == self._ID:
                    if cmd == self._MSG_QUERY:
                        _bytes = struct.pack("!3I", width, height, dpi)
                        self._write(self._SOURCE_ID, self._MSG_SINK_AVAILABLE, _bytes)
                    elif cmd == self._MSG_QUERY_EXT:
                        phone_width, phone_height, phone_dpi = struct.unpack("!3I", _bytes)
                        _bytes = struct.pack("!3I", width, height, dpi)
                        self._write(self._SOURCE_ID, self._MSG_SINK_AVAILABLE, _bytes)
                    elif cmd == self._MSG_CONTENT:
                        #self._log("Content Size : ", len(_bytes))
                        self.putMediaData(_bytes)
                    else:
                        self._log("not Support:", id, cmd)
                else:
                    self._log("_ID Error")
                    pass
            except usb.core.USBError as e:
                if e.errno == 110:  # Operation timed out.
                    # self._log("Read Timeout")
                    pass
                else:
                    self._log(e)
                    self._isProcessing = False

    @property
    def isStop(self):
        return not self._isProcessing

    def stop(self):
        self._close()


if __name__ == '__main__':
    from signal import signal, SIGTERM, SIGINT

    def _log(*args):
        print(args)

    write_fd = None
    _filename = "saved"
    _pipename = None
    _width = 640
    _height = 480
    _dpi = 96

    try:
        options, args = getopt.getopt(sys.argv[1:], "p:f:w:h:d:", ["help"])
    except getopt.GetoptError as e:
        _log(e)
        sys.exit()

    for option, value in options:
        if option in ("-v", "--help"):
            print("-f fileName , store to file ")
            print("-p pipename , store to pipe , which can be play by ffmpeg ")
            print("-w width ")
            print("-h height ")
            print("-d dpi ")
            print("ffplay -f h264 -vcodec h264 -sn -an")
        elif option in ("-f"):
            _filename = value
        elif option in ("-p"):
            _pipename = value
        elif option in ("-w"):
            _width = int(value)
            print('width : %d' %  _width)
        elif option in ("-h"):
            _height = int(value)
            print('height : %d' % _height)
        elif option in ("-d"):
            _dpi = int(value)
            print('dpi : %d' % _dpi)


    _TIMEOUT_COUNT = 5
    shutdown = False

    try:
        if _pipename is not None :
            print('Use Pipe : %s' % _pipename)
            if not os.path.exists(_pipename) :
                os.mkfifo(_pipename)
            write_fd = os.open(_pipename, os.O_SYNC | os.O_CREAT | os.O_RDWR | os.O_NONBLOCK)
        else :
            if os.path.exists(_filename) :
                os.remove(_filename)
            print('Use File : %s' % _filename)
            write_fd = os.open(_filename, os.O_CREAT | os.O_RDWR )

    except OSError as e:
         _log(e)
         sys.exit()


    print('\n\n\n')


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
                print('UsbScreenShare start Screen')

                aad.start(_height, _width, _dpi)

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
                            _write_size = os.write(write_fd, _bytes)
                            _left_size -= _write_size
                            _bytes = _bytes[_write_size:]
                        except Exception as e:
                            _log("write to fifo error :", len(_bytes),e)
                            pass
                print('USB r/w error Restarting…')
                aad.stop()
                time.sleep(2)
        except usb.core.USBError as e:
            print(e.args)
            time.sleep(2)
            print('USBError occurred. Restarting…')
    try:
        os.close(write_fd)
    except:
        pass  # ignore this
