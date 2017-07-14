package com.lvh.usbhumaninterface;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.Thread;
import java.nio.ByteBuffer;
import java.util.concurrent.atomic.AtomicBoolean;
import android.app.Application;
import android.hardware.usb.UsbAccessory;
import android.os.Handler;
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.hardware.usb.UsbManager;

public class UsbMessageDispatch {
	private final static String TAG = "UsbMessageDispatch";
	private static final int MAX_INPUT_BUFFERS = 8;
	private static final int MAX_PACKAGE_SIZE = 16384;

	private ReaderThread mThread = null;

	private Application mApplication = null;
	private final static Object mThreadSync = new Object();
	private final Object mLock = new Object();
	private final TransportHandler mHandler;

	private static UsbMessageDispatch mUsbMessageDispatch = null;
	private UsbAccessory mUsbAccessory = null;
	private UsbManager mUsbManager = null;

	public static final int STATUS_NOT_CONNECTED = 0;
	public static final int STATUS_CONNECTED = 1;

	private ParcelFileDescriptor mFd;
	private FileInputStream mInputStream;
	private FileOutputStream mOutputStream;

	private ByteBuffer mOutputBuffer;
	private BufferPool mInputBufferPool;

	private MainApplication getApp() {
		return ((MainApplication) mApplication);
	}

	public void quit() {
		mUsbMessageDispatch = null;
		synchronized (mLock) {
			if (mThread != null) {
				mThread.quit();
				mThread = null;
			} else {
				ioClose();
			}
			mOutputBuffer = null;
		}
	}

	public void start() {
		synchronized (mLock) {
			mThread = new ReaderThread();
			mThread.start();
		}
	}

	public static boolean isActive() {
		synchronized (mThreadSync) {
			if (mUsbMessageDispatch == null) {
				return false;
			}
			return true;
		}
	}

	public boolean setDevice(UsbManager usbMg, UsbAccessory accessory) {
		mUsbManager = usbMg;
		Logger.logInfo(TAG, "setDevice : " + accessory.toString());
		if (isSink(accessory) == false) {
			Logger.logDebug(TAG, "setDevice : device type error ");
			return false;
		}
		synchronized (mThreadSync) {

			try {
				mFd = mUsbManager.openAccessory(accessory);
				if (mFd == null) {
					Logger.logDebug(TAG, "Could not obtain accessory connection.");
					return false;
				}
				mInputStream = new FileInputStream(mFd.getFileDescriptor());
				mOutputStream = new FileOutputStream(mFd.getFileDescriptor());
			} catch (Exception e) {
				Logger.logError(TAG, e.toString());
				return false;
			}
			mUsbAccessory = accessory;
			return true;
		}
	}

	public void removeDevice(UsbAccessory accessory) {
		Logger.logInfo(TAG, "setDevice : " + (accessory == null ? "null" : accessory.toString()));
		synchronized (mThreadSync) {
			if (mUsbAccessory != null) {
				if ((accessory == null) || mUsbAccessory.equals(accessory)) {
					mUsbAccessory = null;
					quit();
					return;
				} else {
					Logger.logDebug(TAG, "Mismatch : " + accessory.toString());
				}
			}
		}

	}

	private UsbMessageDispatch(Application apx) {
		mApplication = apx;
		mHandler = new TransportHandler();
		mOutputBuffer = ByteBuffer.allocate(MAX_PACKAGE_SIZE);
		mInputBufferPool = new BufferPool(MAX_PACKAGE_SIZE, Protocol.MAX_ENVELOPE_SIZE, MAX_INPUT_BUFFERS);

	}

	public static UsbMessageDispatch getInstance(Application apx) {
		synchronized (mThreadSync) {
			if (mUsbMessageDispatch == null) {
				mUsbMessageDispatch = new UsbMessageDispatch(apx);
			}
			return mUsbMessageDispatch;
		}
	}

	private boolean isSink(UsbAccessory accessory) {
		final String MANUFACTURER = "Android";
		final String MODEL = "Accessory Display";
		return MANUFACTURER.equals(accessory.getManufacturer()) && MODEL.equals(accessory.getModel());

	}

	private void dispatchMessageReceived(int service, int what, ByteBuffer content) {
		getApp().NotifyUI(service, what, content);
	}

	private void notifyInternalMsg(int what, String obj) {
		getApp().NotifyUI(Protocol.Internal.ID, what, null);
	}

	private void ioClose() {

		try {
			mFd.close();
		} catch (Exception e) {
			e.printStackTrace();
		}
		mFd = null;
		try {
			mInputStream.close();
		} catch (Exception e) {
			e.printStackTrace();
		}
		mInputStream = null;
		try {
			mOutputStream.close();
		} catch (Exception e) {
			e.printStackTrace();
		}
		mOutputStream = null;
	}

	protected int ioRead(byte[] buffer, int offset, int count) throws IOException {
		if (mInputStream == null) {
			notifyInternalMsg(Protocol.Internal.MSG_READ_ERR, null);
			throw new IOException("Stream was closed.");
		}
		return mInputStream.read(buffer, offset, count);
	}

	protected void ioWrite(byte[] buffer, int offset, int count) throws IOException {
		if (mOutputStream == null) {
			notifyInternalMsg(Protocol.Internal.MSG_WRITE_ERR, null);
			throw new IOException("Stream was closed.");
		}
		mOutputStream.write(buffer, offset, count);
	}

	public boolean sendMessage(int service, int what, ByteBuffer content) {

		try {
			synchronized (mLock) {
				if (mOutputBuffer == null) {
					Logger.logError(TAG, "Send message failed because transport was closed.");
					return false;
				}

				final byte[] outputArray = mOutputBuffer.array();
				final int capacity = mOutputBuffer.capacity();
				mOutputBuffer.clear();
				mOutputBuffer.putShort((short) service);
				mOutputBuffer.putShort((short) what);
				// Logger.logError(TAG,String.format("Send message %d,%d,%d" ,
				// service,what, content == null? 0 : (content.limit() -
				// content.position())));
				if (content == null) {
					mOutputBuffer.putInt(0);
				} else {
					final int contentLimit = content.limit();
					int contentPosition = content.position();
					int contentRemaining = contentLimit - contentPosition;
					if (contentRemaining > Protocol.MAX_CONTENT_SIZE) {
						Logger.logError(TAG, "Message content too large:  discard");
						return true;
					}
					mOutputBuffer.putInt(contentRemaining);
					while (contentRemaining != 0) {
						final int outputAvailable = capacity - mOutputBuffer.position();
						if (contentRemaining <= outputAvailable) {
							mOutputBuffer.put(content);
							break;
						}
						content.limit(contentPosition + outputAvailable);
						mOutputBuffer.put(content);
						content.limit(contentLimit);
						ioWrite(outputArray, 0, capacity);
						contentPosition += outputAvailable;
						contentRemaining -= outputAvailable;
						mOutputBuffer.clear();
					}
				}
				ioWrite(outputArray, 0, mOutputBuffer.position());
				return true;
			}
		} catch (IOException ex) {
			Logger.logError(TAG, "Send message failed: " + " Service=" + service + " / What=" + what + ex);
			return false;
		}
	}

	final class TransportHandler extends Handler {
		@Override
		public void handleMessage(Message msg) {
			final ByteBuffer buffer = (ByteBuffer) msg.obj;
			try {
				final int limit = buffer.limit();
				while (buffer.position() < limit) {
					final int service = buffer.getShort() & 0xffff;
					final int what = buffer.getShort() & 0xffff;
					final int contentSize = buffer.getInt();
					Logger.logInfo(TAG, "id="+ service + " what=" + what + " size=" + contentSize);
					if (contentSize == 0) {
						dispatchMessageReceived(service, what, null);
					} else {
						final int end = buffer.position() + contentSize;
						buffer.limit(end);
						dispatchMessageReceived(service, what, buffer);
						buffer.limit(limit);
						buffer.position(end);
					}
				}
			} finally {
				mInputBufferPool.release(buffer);
			}
		}
	}

	private final class ReaderThread extends Thread {
		// Set to true when quitting.
		private volatile AtomicBoolean mQuit = new AtomicBoolean(false);

		public ReaderThread() {
			super("Accessory Display Transport");
		}

		@Override
		public void run() {
			Logger.logDebug(TAG, "ReaderThread :  started");
			loop();
			ioClose();
		}

		private void loop() {
			ByteBuffer buffer = null;
			int length = Protocol.HEADER_SIZE;
			int contentSize = -1;
			outer: while (!mQuit.get()) {
				// Get a buffer.
				if (buffer == null) {
					buffer = mInputBufferPool.acquire(length);
				} else {
					buffer = mInputBufferPool.grow(buffer, length);
				}

				// Read more data until needed number of bytes obtained.
				int position = buffer.position();
				int count;
				try {
					count = ioRead(buffer.array(), position, buffer.capacity() - position);
					if (count < 0) {
						break; // end of stream
					}
				} catch (IOException ex) {
					Logger.logError(TAG, "Read failed: " + ex);
					notifyInternalMsg(Protocol.Internal.MSG_READ_ERR, null);
					break; // error
				}
				position += count;
				buffer.position(position);
				if (contentSize < 0 && position >= Protocol.HEADER_SIZE) {
					contentSize = buffer.getInt(4);
					if (contentSize < 0 || contentSize > Protocol.MAX_CONTENT_SIZE) {
						Logger.logError(TAG, "Encountered invalid content size: " + contentSize);
						break; // malformed stream
					}
					length += contentSize;
				}
				if (position < length) {
					continue; // need more data
				}

				// There is at least one complete message in the buffer.
				// Find the end of a contiguous chunk of complete messages.
				int next = length;
				int remaining;
				for (;;) {
					length = Protocol.HEADER_SIZE;
					remaining = position - next;
					if (remaining < length) {
						contentSize = -1;
						break; // incomplete header, need more data
					}
					contentSize = buffer.getInt(next + 4);
					if (contentSize < 0 || contentSize > Protocol.MAX_CONTENT_SIZE) {
						Logger.logError(TAG, "Encountered invalid content size: " + contentSize);
						break outer; // malformed stream
					}
					length += contentSize;
					if (remaining < length) {
						break; // incomplete content, need more data
					}
					next += length;
				}

				// Post the buffer then don't modify it anymore.
				// Now this is kind of sneaky. We know that no other threads
				// will
				// be acquiring buffers from the buffer pool so we can keep on
				// referring to this buffer as long as we don't modify its
				// contents.
				// This allows us to operate in a single-buffered mode if
				// desired.
				buffer.limit(next);
				buffer.rewind();
				mHandler.obtainMessage(0, buffer).sendToTarget();

				// If there is an incomplete message at the end, then we will
				// need
				// to copy it to a fresh buffer before continuing. In the
				// single-buffered
				// case, we may acquire the same buffer as before which is fine.
				if (remaining == 0) {
					buffer = null;
				} else {
					final ByteBuffer oldBuffer = buffer;
					buffer = mInputBufferPool.acquire(length);
					System.arraycopy(oldBuffer.array(), next, buffer.array(), 0, remaining);
					buffer.position(remaining);
				}
			}

			if (buffer != null) {
				mInputBufferPool.release(buffer);
			}
		}

		public void quit() {
			mQuit.set(true);
		}
	}
}
