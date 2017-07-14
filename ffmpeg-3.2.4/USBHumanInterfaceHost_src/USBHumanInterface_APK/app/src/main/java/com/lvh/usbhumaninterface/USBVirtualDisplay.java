package com.lvh.usbhumaninterface;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.atomic.AtomicBoolean;
import android.app.Application;
import android.hardware.display.VirtualDisplay;
import android.media.MediaCodec;
import android.media.MediaCodec.BufferInfo;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.projection.MediaProjection;
import android.os.Handler;
import android.view.Surface;

public class USBVirtualDisplay extends Thread {
	private final static String TAG = "USBVirtualDisplay";
	private static final int TIMEOUT_USEC = 1000000;
	private static final String DISPLAY_NAME = "Encode Display";

	private MediaProjection mMediaProjection = null;
	private int mWidth;
	private int mHeight;
	private int mStyle;
	private int mDensityDpi;
	private int BIT_RATE = 10000000;
	private int FRAME_RATE = 20;

	
	private volatile AtomicBoolean mQuit = new AtomicBoolean(false);

	private Application mApplication = null;

	private static final int I_FRAME_INTERVAL = 10;

	private static Object mThreadSync = new Object();

	private static USBVirtualDisplay mUSBVirtualDisplay = null;
	private final Handler mHandler;

	public MainApplication getApp() {
		return ((MainApplication) mApplication);
	}

	public void quit() {
		mQuit.set(true);
		mUSBVirtualDisplay = null;
	}

	public static boolean isActive() {
		synchronized (mThreadSync) {
			if (mUSBVirtualDisplay == null) {
				return false;
			}
			return true;
		}
	}

	public void setDevice(MediaProjection mpj, int style, Protocol.EncodecParameter p) {
		Logger.logDebug(TAG, "Setting up a VirtualDisplay: Style = " + style);
		Logger.logDebug(TAG, p.toString());
		mMediaProjection = mpj;
		mStyle = style;

		mWidth = p.mWidth;
		mHeight = p.mHeight;
		mDensityDpi = p.mDpi;

		
		BIT_RATE = p.mBps;
		FRAME_RATE = p.mFrameRate;
	}

	public void removeDevice() {
		quit();
	}

	private void sendMessage(int what, ByteBuffer content) {
		getApp().sendMessage(Protocol.MediaSource.ID, what, content);
	}

	private USBVirtualDisplay(Application apx) {
		mApplication = apx;
		mHandler = new Handler();
	}

	public static USBVirtualDisplay getInstance(Application apx) {
		synchronized (mThreadSync) {
			if (mUSBVirtualDisplay == null) {
				mUSBVirtualDisplay = new USBVirtualDisplay(apx);
			}
			return mUSBVirtualDisplay;
		}
	}

	@Override
	public void run() {
		MediaFormat format = MediaFormat.createVideoFormat("video/avc", mWidth, mHeight);
		format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
		format.setInteger(MediaFormat.KEY_BIT_RATE, BIT_RATE);
		format.setInteger(MediaFormat.KEY_FRAME_RATE, FRAME_RATE);
		format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, I_FRAME_INTERVAL);
		MediaCodec codec;
		try {
			codec = MediaCodec.createEncoderByType("video/avc");
		} catch (IOException e) {
			throw new RuntimeException("failed to create video/avc encoder", e);
		}
		codec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
		Surface surface = codec.createInputSurface();
		codec.start();

		VirtualDisplay virtualDisplay = null;
		try {
			if (mMediaProjection != null) {
				virtualDisplay = mMediaProjection.createVirtualDisplay(DISPLAY_NAME, mWidth, mHeight, mDensityDpi,
						mStyle, surface, new VirtualDisplayCallback(), mHandler);
				Logger.logInfo(TAG, "virtualDisplay create ok");
			} else {
				Logger.logInfo(TAG, "virtualDisplay create error");
			}

		} catch (Exception e) {
			Logger.logInfo(TAG, "Create VirtualDisplay Error : " + e.toString());
		}
		if (virtualDisplay != null) {
			loop(codec);
			try {
				virtualDisplay.release();
				Logger.logInfo(TAG, "virtualDisplay release ");
			} catch (Exception e) {
				Logger.logInfo(TAG, "virtualDisplay release Error ignore: " + e.toString());
			}
		}

		codec.signalEndOfInputStream();
		codec.stop();
		synchronized (mThreadSync) {
			mUSBVirtualDisplay = null;
			mThreadSync.notifyAll();
		}
	}

	private void loop(MediaCodec codec) {
		BufferInfo info = new BufferInfo();
		while (!mQuit.get()) {
			int index = codec.dequeueOutputBuffer(info, TIMEOUT_USEC);
			if (index >= 0) {

				ByteBuffer buffer = codec.getOutputBuffer(index);
				buffer.limit(info.offset + info.size);
				buffer.position(info.offset);

				sendMessage(Protocol.MediaSource.MSG_CONTENT, buffer);
				codec.releaseOutputBuffer(index, false);
			} else if (index == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {

			} else if (index == MediaCodec.INFO_TRY_AGAIN_LATER) {
				Logger.logInfo(TAG, "Codec dequeue buffer timed out.");
			}
		}
	}

	private final class VirtualDisplayCallback extends VirtualDisplay.Callback {
		/**
		 * Called when the virtual display video projection has been paused by
		 * the system or when the surface has been detached by the application
		 * by calling setSurface(null). The surface will not receive any more
		 * buffers while paused.
		 */
		@Override
		public void onPaused() {
			Logger.logInfo(TAG, "VirtualDisplay Paused");
		}

		/**
		 * Called when the virtual display video projection has been resumed
		 * after having been paused.
		 */
		@Override
		public void onResumed() {
			Logger.logInfo(TAG, "VirtualDisplay Resumed");
		}

		/**
		 * Called when the virtual display video projection has been stopped by
		 * the system. It will no longer receive frames and it will never be
		 * resumed. It is still the responsibility of the application to
		 * release() the virtual display.
		 */
		@Override
		public void onStopped() {
			Logger.logInfo(TAG, "VirtualDisplay Stopped");
		}
	}
}
