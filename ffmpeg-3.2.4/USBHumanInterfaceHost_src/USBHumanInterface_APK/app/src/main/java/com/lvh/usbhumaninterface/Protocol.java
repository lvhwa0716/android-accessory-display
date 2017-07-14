
package com.lvh.usbhumaninterface;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

/**
 * Defines message types.
 */
public class Protocol {
	// Message header.
	// 0: service id (16 bits)
	// 2: what (16 bits)
	// 4: content size (32 bits)
	// 8: ... content follows ...
	static final int HEADER_SIZE = 8;

	// Maximum size of a message envelope including the header and contents.
	static final int MAX_ENVELOPE_SIZE = 2 * 1024 * 1024;

	/**
	 * Maximum message content size.
	 */
	public static final int MAX_CONTENT_SIZE = MAX_ENVELOPE_SIZE - HEADER_SIZE;

	public interface Callback {
		/**
		 * Indicates that a message was received.
		 *
		 * @param service
		 *            The service to whom the message is addressed.
		 * @param what
		 *            The message type.
		 * @param content
		 *            The content, or null if there is none.
		 */
		public void onMessageReceived(int service, int what, ByteBuffer content);
	}

	public static final class Internal {
		public static final int ID = 0;
		// error no < 100 , usb error , stop all
		public static final int MSG_READ_ERR = 1;
		public static final int MSG_WRITE_ERR = 2;
		public static final int DEVICE_ERROR_LIMIT = 100;

		public static final int MSG_DISCONNECT = 1000;
		public static final int MSG_PERMISSION_AGRANTED = 1001;
		public static final int MSG_PERMISSION_DENIED = 1002;
		public static final int MSG_DISPLAY_OK = 1003;
		public static final int MSG_DISPLAY_FAIL = 1004;
		public static final int MSG_DEVICE_OK = 1005;

	}

	public static final class MediaSource {
		public static final int ID = 1;
		public static final int MSG_QUERY = 1;
		public static final int MSG_CONTENT = 2; // MediaCodec "avc"

	}

	public static final class Render {

		public static final int ID = 2;

		// Sink is now available for use.
		// 0: width (32 bits)
		// 4: height (32 bits)
		// 8: density dpi (32 bits)
		public static final int MSG_SINK_AVAILABLE = 1;

		// Sink is no longer available for use.
		public static final int MSG_SINK_NOT_AVAILABLE = 2;

	}

	public static final String ACTION_USB_PERMISSION = "com.lvh.usbhumaninterface.USBPermission";

	public static final class EncodecParameter {
		public int mWidth;
		public int mHeight;
		public int mBps;
		public int mDpi;
		public int mFrameRate;

		public EncodecParameter(int w, int h, int bps, int frameRate, int dpi) {
			mWidth = w;
			mHeight = h;
			mBps = bps;
			mDpi = dpi;
			mFrameRate = frameRate;
		}

		public EncodecParameter clone() {
			return new EncodecParameter(mWidth, mHeight, mBps, mFrameRate, mDpi);
		}

		@Override
		public String toString() {
			StringBuilder sb = new StringBuilder("");
			sb.append("w=");
			sb.append(mWidth);

			sb.append(" h=");
			sb.append(mHeight);

			sb.append(" bps=");
			sb.append(mBps);

			sb.append(" fps=");
			sb.append(mFrameRate);

			sb.append(" dpi=");
			sb.append(mDpi);
			return sb.toString();
		}
	}

	public static final List<EncodecParameter> H264Profile = new ArrayList<EncodecParameter>();
	static {
		H264Profile.add(new EncodecParameter(720, 480, 1800000, 20, 96)); // 480P
		H264Profile.add(new EncodecParameter(1280, 720, 3500000, 20, 96)); // 720P
		H264Profile.add(new EncodecParameter(1920, 1080, 8500000, 20, 96)); // 1280P
	};
}
