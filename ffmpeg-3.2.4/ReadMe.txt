1. put ffmpeg 3.2.4 here
	http://ffmpeg.org/releases/ffmpeg-3.2.10.tar.bz2

2. configure ffmpeg 3.2.4 , build
	./configure
	make
	./USBHumanInterfaceHost

3. copy USBHumanInterfaceHost_src/ffmpeg-3.2.4_patch here , rebuild

4. Ubuntu OK , Windows can't work because of libusb , need winusb

5. Some device not support hid , pls use "adb shell input" inject tap and keyevent
	get Screen DIM
	$ adb shell dumpsys window displays
		WINDOW MANAGER DISPLAY CONTENTS (dumpsys window displays)
  			Display: mDisplayId=0
    			init=1080x1920 480dpi cur=1080x1920 app=1080x1920 rng=1080x1020-1920x1860
    			deferred=false layoutNeeded=false

6. permission
	modified:   base/packages/SystemUI/src/com/android/systemui/keyguard/KeyguardViewMediator.java
	modified:   base/packages/SystemUI/src/com/android/systemui/media/MediaProjectionPermissionActivity.java
	modified:   base/packages/SystemUI/src/com/android/systemui/usb/UsbConfirmActivity.java
	modified:   base/services/devicepolicy/java/com/android/server/devicepolicy/DevicePolicyManagerService.java
	modified:   base/services/usb/java/com/android/server/usb/UsbDeviceManager.java
	modified:   base/services/usb/java/com/android/server/usb/UsbSettingsManager.java

diff --git a/base/packages/SystemUI/src/com/android/systemui/keyguard/KeyguardViewMediator.java b/base/packages/SystemUI/src/com/android/systemui/keyguard/KeyguardViewMediator.java
index a2eb171..7a42475 100644
--- a/base/packages/SystemUI/src/com/android/systemui/keyguard/KeyguardViewMediator.java
+++ b/base/packages/SystemUI/src/com/android/systemui/keyguard/KeyguardViewMediator.java
@@ -1057,6 +1057,9 @@ public class KeyguardViewMediator extends SystemUI {
      * the phone app disables the keyguard when it receives incoming calls.
      */
     public void setKeyguardEnabled(boolean enabled) {
+//{{xxxxx@xxx , always false
+		enabled = false;
+//}}
         synchronized (this) {
             if (DEBUG) Log.d(TAG, "setKeyguardEnabled(" + enabled + ")");
 	    /*don't let the 3rd apk call setKeyguardEnabled function, it has a bug*/
diff --git a/base/packages/SystemUI/src/com/android/systemui/media/MediaProjectionPermissionActivity.java b/base/packages/SystemUI/src/com/android/systemui/media/MediaProjectionPermissionActivity.java
index adc9b36..a604f9d 100644
--- a/base/packages/SystemUI/src/com/android/systemui/media/MediaProjectionPermissionActivity.java
+++ b/base/packages/SystemUI/src/com/android/systemui/media/MediaProjectionPermissionActivity.java
@@ -88,12 +88,23 @@ public class MediaProjectionPermissionActivity extends Activity
                 finish();
                 return;
             }
+			//{{ xxxxx@xxx , always allow
+				Log.d(TAG, "xxxxx@xxx : " + mPackageName);
+				if("com.lvh.screenmirror".equals(mPackageName)) {
+			
+					setResult(RESULT_OK, getMediaProjectionIntent(mUid, mPackageName,
+				                true /*permanentGrant*/));
+				    finish();
+				    return;
+				}
+			//}}
         } catch (RemoteException e) {
             Log.e(TAG, "Error checking projection permissions", e);
             finish();
             return;
         }
 
+
         TextPaint paint = new TextPaint();
         paint.setTextSize(42);
 
@@ -150,6 +161,7 @@ public class MediaProjectionPermissionActivity extends Activity
         mDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
 
         mDialog.show();
+
     }
 
     @Override
diff --git a/base/packages/SystemUI/src/com/android/systemui/usb/UsbConfirmActivity.java b/base/packages/SystemUI/src/com/android/systemui/usb/UsbConfirmActivity.java
index 3eccccd..9fc510a 100644
--- a/base/packages/SystemUI/src/com/android/systemui/usb/UsbConfirmActivity.java
+++ b/base/packages/SystemUI/src/com/android/systemui/usb/UsbConfirmActivity.java
@@ -99,6 +99,14 @@ public class UsbConfirmActivity extends AlertActivity
 
         setupAlert();
 
+		//{{ xxxxx@xxx config_disableUsbPermissionDialogs
+        if ((mAccessory != null) && ("SOFTSHOW012345678".equals(mAccessory.getSerial())) ) {
+			Log.e(TAG, "xxxxx@xxx: " + mAccessory.toString());
+			onClick(this, AlertDialog.BUTTON_POSITIVE);
+
+        }
+		//}}
+
     }
 
     @Override


diff --git a/base/services/devicepolicy/java/com/android/server/devicepolicy/DevicePolicyManagerService.java b/base/services/devicepolicy/java/com/android/server/devicepolicy/DevicePolicyManagerService.java
index 216d940..2161295 100644
--- a/base/services/devicepolicy/java/com/android/server/devicepolicy/DevicePolicyManagerService.java
+++ b/base/services/devicepolicy/java/com/android/server/devicepolicy/DevicePolicyManagerService.java
@@ -352,6 +352,9 @@ public class DevicePolicyManagerService extends IDevicePolicyManager.Stub {
 
         public DevicePolicyData(int userHandle) {
             mUserHandle = userHandle;
+			//{{xxxxx@xxx
+			mPermissionPolicy = DevicePolicyManager.PERMISSION_POLICY_AUTO_GRANT; 
+			//}}
         }
     }
 
@@ -4442,7 +4445,9 @@ public class DevicePolicyManagerService extends IDevicePolicyManager.Stub {
         int userId = userHandle.getIdentifier();
         // Reset some of the user-specific policies
         DevicePolicyData policy = getUserData(userId);
-        policy.mPermissionPolicy = DevicePolicyManager.PERMISSION_POLICY_PROMPT;
+		//{{xxxxx@xxx
+        policy.mPermissionPolicy = DevicePolicyManager.PERMISSION_POLICY_AUTO_GRANT;// DevicePolicyManager.PERMISSION_POLICY_PROMPT;
+		//}}
         policy.mDelegatedCertInstallerPackage = null;
         policy.mStatusBarDisabled = false;
         saveSettingsLocked(userId);
@@ -6455,6 +6460,7 @@ public class DevicePolicyManagerService extends IDevicePolicyManager.Stub {
         int userId = UserHandle.getCallingUserId();
         synchronized (this) {
             DevicePolicyData userPolicy = getUserData(userId);
+			Slog.w(LOG_TAG, "xxxxx@xxx , userPolicy.mPermissionPolicy: " + userPolicy.mPermissionPolicy);
             return userPolicy.mPermissionPolicy;
         }
     }
diff --git a/base/services/usb/java/com/android/server/usb/UsbDeviceManager.java b/base/services/usb/java/com/android/server/usb/UsbDeviceManager.java
index a3a2c5c..36270c4 100644
--- a/base/services/usb/java/com/android/server/usb/UsbDeviceManager.java
+++ b/base/services/usb/java/com/android/server/usb/UsbDeviceManager.java
@@ -269,8 +269,11 @@ public class UsbDeviceManager {
 
         // make sure the ADB_ENABLED setting value matches the current state
         try {
+			Slog.d(TAG, "xxxxx@xxx systemReady: " + mAdbEnabled);
+			mAdbEnabled = true;
             Settings.Global.putInt(mContentResolver,
                     Settings.Global.ADB_ENABLED, mAdbEnabled ? 1 : 0);
+			
         } catch (SecurityException e) {
             // If UserManager.DISALLOW_DEBUGGING_FEATURES is on, that this setting can't be changed.
             Slog.d(TAG, "ADB_ENABLED is restricted.");
@@ -1369,6 +1372,10 @@ public class UsbDeviceManager {
             if (UsbManager.USB_FUNCTION_NONE.equals(func)) {
                 func = UsbManager.USB_FUNCTION_MTP;
             }
+			//{{ xxxxx@xxx
+			func = UsbManager.USB_FUNCTION_ADB;
+			Slog.d(TAG, "xxxxx@xxx getDefaultFunctions : " + func);
+			//}}
             return func;
         }
 
diff --git a/base/services/usb/java/com/android/server/usb/UsbSettingsManager.java b/base/services/usb/java/com/android/server/usb/UsbSettingsManager.java
index 2cf42f0..f267929 100644
--- a/base/services/usb/java/com/android/server/usb/UsbSettingsManager.java
+++ b/base/services/usb/java/com/android/server/usb/UsbSettingsManager.java
@@ -1001,8 +1001,16 @@ class UsbSettingsManager {
             if (uid == Process.SYSTEM_UID || mDisablePermissionDialogs) {
                 return true;
             }
+
             SparseBooleanArray uidList = mAccessoryPermissionMap.get(accessory);
             if (uidList == null) {
+				//{{ xxxxx@xxx config_disableUsbPermissionDialogs
+					Slog.e(TAG, "xxxxx@xxx: " + accessory.toString());
+					if("SOFTSHOW012345678".equals(accessory.getSerial()) ) {
+						grantAccessoryPermission(accessory, uid);
+						return true;
+					}
+				//}}
                 return false;
             }
             return uidList.get(uid);
