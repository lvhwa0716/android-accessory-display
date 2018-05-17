1. put ffmpeg 3.2.4 here
	http://ffmpeg.org/releases/ffmpeg-3.2.10.tar.bz2
2. configure ffmpeg 3.2.4 , build
	./configure
	make
	./USBHumanInterfaceHost

3. copy USBHumanInterfaceHost_src/ffmpeg-3.2.4_patch here , rebuild
4. Ubuntu OK , Windows can't work because of libusb , need winusb
