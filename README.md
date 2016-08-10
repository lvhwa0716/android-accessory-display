# android-accessory-display
Android USB Accessory,lvhwa0716@163.com
# Host Required
    python 2.7 & ubuntu 14.04LTS
# 1. install libusb
# 2. install ffmpeg
    sudo add-apt-repository ppa:mc3man/trusty-media
    sudo apt-get update
    sudo apt-get install ffmpeg
# 3. install pyusb
	or download source :  https://github.com/walac/pyusb
# 4. permission
    /etc/udev/rules.d/51-android.rules
    SUBSYSTEM=="usb", ATTR{idVendor}=="18d1", ATTR{idProduct}=="2d01", MODE="0666", OWNER="<username>"
    SUBSYSTEM=="usb", ATTR{idVendor}=="2717", ATTR{idProduct}=="ff68", MODE="0666", OWNER="<username>"
    SUBSYSTEM=="usb", ATTR{idVendor}=="2717", ATTR{idProduct}=="ff48", MODE="0666", OWNER="<username>"
    or used sudo when permission exception
# 5. run
	enable USB debug on phone
    a. aosp/frameworks/base/tests/AccessoryDisplay on phone
        when use this , screen size must less 320x240,
        or modify com.android.accessorydisplay.common.Protocol.MAX_ENVELOPE_SIZE more larger
    b. run this file
    c. run ffplay /tmp/android_accessory_display.pipe on ubuntu.
	   prebuilt ffplay support video sync , it show slow then phone.
	   getcode : https://github.com/FFmpeg/FFmpeg
	   fixed it(not use video sync) : get ffmpeg source 
		 modify ffplay.c like follow :
			time= av_gettime_relative()/1000000.0;
            if (time < is->frame_timer + delay) {
                *remaining_time = FFMIN(is->frame_timer + delay - time, *remaining_time);
                //goto display; // == mask this line
            
            }
            is->frame_timer += delay;


