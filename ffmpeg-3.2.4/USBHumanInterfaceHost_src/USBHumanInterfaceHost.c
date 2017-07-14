#if 0
AVPROGS-$(CONFIG_FFMPEG)   += ffmpeg
AVPROGS-$(CONFIG_FFPLAY)   += ffplay
AVPROGS-$(CONFIG_FFPROBE)  += ffprobe
AVPROGS-$(CONFIG_FFSERVER) += ffserver
AVPROGS-yes += USBHumanInterfaceHost
OBJS-USBHumanInterfaceHost := USBHumanInterfaceHost/MessageNotify.o \
		USBHumanInterfaceHost/USBAndroidDevice.o \
		USBHumanInterfaceHost/USBAndroidHID.o \
		USBHumanInterfaceHost/USBAndroidHotplug.o \
		USBHumanInterfaceHost/USBAndroidKeymap.o \
		USBHumanInterfaceHost/USBAndroidMain.o \
		USBHumanInterfaceHost/USBAndroidMediaPlay.o \
		USBHumanInterfaceHost/USBAndroidScreen.o \
		USBHumanInterfaceHost/ffplay_win32.o \
		
AVBASENAMES  = ffmpeg ffplay ffprobe ffserver USBHumanInterfaceHost
#endif