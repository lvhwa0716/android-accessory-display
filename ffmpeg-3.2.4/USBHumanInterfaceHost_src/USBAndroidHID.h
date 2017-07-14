/*
 ============================================================================
 Name        : USBAndroidHID.h
 Author      : lvhwa0716@gmail.com
 Version     :
 Copyright   : 
 Description :  Ansi-style
 ============================================================================
 */
#ifndef _USBAndroidHID_h__
#define _USBAndroidHID_h__
//http://www.microsoft.com/whdc/device/input/DigitizerDrvs_touch.mspx
//https://msdn.microsoft.com/en-us/library/windows/hardware/dn672287.aspx
// DigitizerDrvs_touch.docx
//https://github.com/Microsoft/Windows-driver-samples/blob/master/input/hiddigi/SynapticsTouch
#define REPORTID_MTOUCH                 1
#define REPORTID_MOUSE                  3
#define REPORTID_FEATURE                7
#define REPORTID_MAX_COUNT              8
#define REPORTID_CAPKEY                 9

#define FINGER_STATUS                   0x01 // finger down
#define RANGE_STATUS                    0x02 // in range
#define RANGE_FINGER_STATUS             0x03 // finger down (range + finger)

#define KEY_DOWN_START                  (1 << 0)
#define KEY_DOWN_SEARCH                 (1 << 1)
#define KEY_DOWN_BACK                   (1 << 2)

const unsigned char MouseReportDescriptor[] = { 0x05, 0x01, // USAGE_PAGE (Generic Desktop)
		0x09, 0x02,                    // USAGE (Mouse)
		0xa1, 0x01,                    // COLLECTION (Application)
		0x05, 0x09,                    //   USAGE_PAGE (Button)
		0x19, 0x01,                    //   USAGE_MINIMUM (Button 1)
		0x29, 0x03,                    //   USAGE_MAXIMUM (Button 3)
		0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
		0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
		0x95, 0x03,                    //   REPORT_COUNT (3)
		0x75, 0x01,                    //   REPORT_SIZE (1)
		0x81, 0x02,                    //   INPUT (Data,Var,Abs)
		0x95, 0x01,                    //   REPORT_COUNT (1)
		0x75, 0x05,                    //   REPORT_SIZE (5)
		0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
		0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop)
		0x09, 0x01,                    //   USAGE (Pointer)
		0xa1, 0x00,                    //   COLLECTION (Physical)
		0x09, 0x30,                    //     USAGE (X)
		0x09, 0x31,                    //     USAGE (Y)
		0x16, 0x01, 0x80,              //     LOGICAL_MINIMUM (-32767)
		0x26, 0xff, 0x7f,              //     LOGICAL_MAXIMUM (32767)
		0x75, 0x10,                    //     REPORT_SIZE (16)
		0x95, 0x02,                    //     REPORT_COUNT (2)
		0x81, 0x06,                    //     INPUT (Data,Var,Rel)
		0xc0,                          //     END_COLLECTION
		0x09, 0x38,                    //   USAGE (Wheel)
		0x15, 0x81,                    //   LOGICAL_MINIMUM (-127)
		0x25, 0x7f,                    //   LOGICAL_MAXIMUM (127)
		0x75, 0x08,                    //   REPORT_SIZE (8)
		0x95, 0x01,                    //   REPORT_COUNT (1)
		0x81, 0x06,                    //   INPUT (Data,Var,Rel)
		0xc0                           // END_COLLECTION

		};

//
//https://github.com/NordicSemiconductor/ble-sdk-arduino/tree/master/libraries/BLE/examples/ble_HID_keyboard_template
const unsigned char KeyBoardReportDescriptor[] = { 0x05, 0x01, // Usage Page (Generic Desktop)
		0x09, 0x06,                         // Usage (Keyboard)
		0xA1, 0x01,                         // Collection (Application)
		0x05, 0x07,                         //     Usage Page (Key Codes)
		0x19, 0xe0,                         //     Usage Minimum (224)
		0x29, 0xe7,                         //     Usage Maximum (231)
		0x15, 0x00,                         //     Logical Minimum (0)
		0x25, 0x01,                         //     Logical Maximum (1)
		0x75, 0x01,                         //     Report Size (1)
		0x95, 0x08,                         //     Report Count (8)
		0x81, 0x02,                      //     Input (Data, Variable, Absolute)

		0x95, 0x01,                         //     Report Count (1)
		0x75, 0x08,                         //     Report Size (8)
		0x81, 0x01,                     //     Input (Constant) reserved byte(1)

		0x95, 0x05,                         //     Report Count (5)
		0x75, 0x01,                         //     Report Size (1)
		0x05, 0x08,                         //     Usage Page (Page# for LEDs)
		0x19, 0x01,                         //     Usage Minimum (1)
		0x29, 0x05,                         //     Usage Maximum (5)
		0x91, 0x02,         //     Output (Data, Variable, Absolute), Led report
		0x95, 0x01,                         //     Report Count (1)
		0x75, 0x03,                         //     Report Size (3)
		0x91, 0x01, //     Output (Data, Variable, Absolute), Led report padding

		0x95, 0x06,                         //     Report Count (6)
		0x75, 0x08,                         //     Report Size (8)
		0x15, 0x00,                         //     Logical Minimum (0)
		0x25, 0x65,                         //     Logical Maximum (101)
		0x05, 0x07,                         //     Usage Page (Key codes)
		0x19, 0x00,                         //     Usage Minimum (0)
		0x29, 0x65,                         //     Usage Maximum (101)
		0x81, 0x00,                //     Input (Data, Array) Key array(6 bytes)

		0x09, 0x05,                         //     Usage (Vendor Defined)
		0x15, 0x00,                         //     Logical Minimum (0)
		0x26, 0xFF, 0x00,                   //     Logical Maximum (255)
		0x75, 0x08,                         //     Report Count (2)
		0x95, 0x02,                         //     Report Size (8 bit)
		0xB1, 0x02,                    //     Feature (Data, Variable, Absolute)

		0xC0 // End Collection (Application)
		};

const unsigned char MultiTouchReportDescriptor[] = { 0x05, 0x0d,	// USAGE_PAGE (Digitizers)
		0x09, 0x04,	                        // USAGE (Touch Screen)
		0xa1, 0x01,                         // COLLECTION (Application)
		0x85, REPORTID_MTOUCH,              //   REPORT_ID (Touch)
		0x09, 0x22,                         //   USAGE (Finger)
		0xa1, 0x02,                         //   COLLECTION (Logical)
		0x09, 0x42,                         //     USAGE (Tip Switch)
		0x15, 0x00,                         //     LOGICAL_MINIMUM (0)
		0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)
		0x75, 0x01,                         //     REPORT_SIZE (1)
		0x95, 0x01,                         //     REPORT_COUNT (1)
		0x81, 0x02,                         //       INPUT (Data,Var,Abs)
		0x09, 0x32,	                        //     USAGE (In Range)
		0x81, 0x02,                         //       INPUT (Data,Var,Abs)
		0x95, 0x06,                         //     REPORT_COUNT (6)
		0x81, 0x03,                         //       INPUT (Cnst,Ary,Abs)
		0x75, 0x08,                         //     REPORT_SIZE (8)
		0x09, 0x51,                         //     USAGE (Contact Identifier)
		0x95, 0x01,                         //     REPORT_COUNT (1)
		0x81, 0x02,                         //       INPUT (Data,Var,Abs)
		0x05, 0x01,                         //     USAGE_PAGE (Generic Desk..
		0x26, 0xff, 0x7f, //     LOGICAL_MAXIMUM (768)        NOTE: ADJUST LOGICAL MAXIMUM FOR X BASED ON TOUCH SCREEN RESOLUTION!
		0x75, 0x10,                         //     REPORT_SIZE (16)
		0x55, 0x0e,                         //     UNIT_EXPONENT (-2)
		0x65, 0x13,                         //     UNIT (Inch,EngLinear)
		0x09, 0x30,                         //     USAGE (X)
		0x35, 0x00,                         //     PHYSICAL_MINIMUM (0)
		0x46, 0x00, 0x00, //     PHYSICAL_MAXIMUM (232)       NOTE: ADJUST PHYSICAL MAXIMUM FOR X BASED ON TOUCH SCREEN DIMENSION (100ths of an inch)!
		0x81, 0x02,                         //       INPUT (Data,Var,Abs)
		0x46, 0x00, 0x00, //     PHYSICAL_MAXIMUM (382)       NOTE: ADJUST PHYSICAL MAXIMUM FOR Y BASED ON TOUCH SCREEN DIMENSION (100ths of an inch)!
		0x26, 0xff, 0x7f, //     LOGICAL_MAXIMUM (1280)       NOTE: ADJUST LOGICAL MAXIMUM FOR Y BASED ON TOUCH SCREEN RESOLUTION!
		0x09, 0x31,                         //     USAGE (Y)
		0x81, 0x02,                         //       INPUT (Data,Var,Abs)
		0xc0,                               //   END_COLLECTION
		0x05, 0x0d,	                        //   USAGE_PAGE (Digitizers)
		0x09, 0x54,	                        //   USAGE (Actual count)
		0x95, 0x01,                         //   REPORT_COUNT (1)
		0x75, 0x08,                         //   REPORT_SIZE (8)
		0x81, 0x02,                         //     INPUT (Data,Var,Abs)
		0x85, REPORTID_MAX_COUNT,           //   REPORT_ID (Feature)
		0x09, 0x55,                         //   USAGE(Maximum Count)
		0x25, 0x02,                         //   LOGICAL_MAXIMUM (2)
		0xb1, 0x02,                         //   FEATURE (Data,Var,Abs)
		0xc0,                               // END_COLLECTION
		0x09, 0x0E,                         // USAGE (Configuration)
		0xa1, 0x01,                         // COLLECTION (Application)
		0x85, REPORTID_FEATURE,             //   REPORT_ID (Feature)
		0x09, 0x22,                         //   USAGE (Finger)
		0xa1, 0x00,                         //   COLLECTION (physical)
		0x09, 0x52,                         //     USAGE (Input Mode)
		0x09, 0x53,                         //     USAGE (Device Index
		0x15, 0x00,                         //     LOGICAL_MINIMUM (0)
		0x25, 0x0a,                         //     LOGICAL_MAXIMUM (10)
		0x75, 0x08,                         //     REPORT_SIZE (8)
		0x95, 0x02,                         //     REPORT_COUNT (2)
		0xb1, 0x02,                         //     FEATURE (Data,Var,Abs)
		0xc0,                               //   END_COLLECTION
		0xc0,                               // END_COLLECTION
		0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)
		0x09, 0xEE,                         // USAGE (HID_USAGE_KEYBOARD_MOBILE)
		0xa1, 0x01,                         // COLLECTION (Application)
		0x85, REPORTID_CAPKEY,              //   REPORT_ID
		0x05, 0x07,                         //   USAGE_PAGE (Key Codes)
		0x09, 0x3B,                         //   USAGE(F2 Key)  - Start/Home
		0x09, 0x3C,                         //   USAGE(F3 Key)  - Search
		0x09, 0x29,                         //   USAGE(Esc Key) - Back
		0x15, 0x00,                         //   LOGICAL_MINIMUM (0)
		0x25, 0x01,                         //   LOGICAL_MAXIMUM (1)
		0x75, 0x01,                         //   REPORT_SIZE (1)
		0x95, 0x03,                         //   REPORT_COUNT (3)
		0x81, 0x02,                         //   INPUT (Data,Var,Abs)
		0x95, 0x1d,                         //   REPORT_COUNT (29)
		0x81, 0x03,                         //   INPUT (Cnst,Var,Abs)
		0xc0,                               // END_COLLECTION
		};

#endif // #ifndef _USBAndroidHID_h__
