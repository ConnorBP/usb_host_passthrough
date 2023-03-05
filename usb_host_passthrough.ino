// Simple test of USB Host Mouse/Keyboard
//
// This example is in the public domain

#include "USBHost_t36.h"

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
KeyboardController keyboard1(myusb);
KeyboardController keyboard2(myusb);
USBHIDParser hid1(myusb);
USBHIDParser hid2(myusb);
USBHIDParser hid3(myusb);
USBHIDParser hid4(myusb);
USBHIDParser hid5(myusb);
MouseController mouse1(myusb);
JoystickController joystick1(myusb);
//BluetoothController bluet(myusb, true, "0000");   // Version does pairing to device
BluetoothController bluet(myusb);   // version assumes it already was paired
int user_axis[64];
uint32_t buttons_prev = 0;
RawHIDController rawhid1(myusb);
RawHIDController rawhid2(myusb, 0xffc90004);

USBDriver *drivers[] = {&hub1, &hub2,&keyboard1, &keyboard2, &joystick1, &bluet, &hid1, &hid2, &hid3, &hid4, &hid5};
#define CNT_DEVICES (sizeof(drivers)/sizeof(drivers[0]))
const char * driver_names[CNT_DEVICES] = {"Hub1","Hub2", "KB1", "KB2", "JOY1D", "Bluet", "HID1" , "HID2", "HID3", "HID4", "HID5"};
bool driver_active[CNT_DEVICES] = {false, false, false, false};

// Lets also look at HID Input devices
USBHIDInput *hiddrivers[] = {&mouse1, &joystick1, &rawhid1, &rawhid2};
#define CNT_HIDDEVICES (sizeof(hiddrivers)/sizeof(hiddrivers[0]))
const char * hid_driver_names[CNT_DEVICES] = {"Mouse1", "Joystick1", "RawHid1", "RawHid2"};
bool hid_driver_active[CNT_DEVICES] = {false, false};
bool show_changed_only = false;

uint8_t joystick_left_trigger_value = 0;
uint8_t joystick_right_trigger_value = 0;
uint64_t joystick_full_notify_mask = (uint64_t) - 1;

// written from interrupt
volatile uint8_t get_buttons = 0;
//uint8_t old_buttons_value = 0;

// button input from serial commands
uint8_t serial_buttons_value = 0;

elapsedMillis sinceMouseData; // get elapsed time since last mouse

// for command receiving
String inputString = "";
bool stringComplete = false;

// receiving move commands
int argc = 0;
// max 4 args
int args[4] = {0,0,0,0};
uint16_t mvx,mvy = 0;

// syntax for receiving numbers
const char startOfNumberDelimiter = '<';
const char endOfNumberDelimiter = '>';

// interupt timer for reading usb mouse events at 1khz
IntervalTimer usbReadTimer;

void setup()
{
  //while (!Serial) ; // wait for Arduino Serial Monitor
  Serial.begin(9600);
  Serial.println("\n\nUSB Host Testing");
  Serial.println(sizeof(USBHub), DEC);
  Serial1.begin(9600);
  inputString.reserve(200);
  myusb.begin();
  usbReadTimer.begin(readMouseIn,1000);

  
}

bool prefix(const char *pre, const char *str)
{
  return strncmp(pre, str, strlen(pre)) == 0;
}

// casts four bytes from a byte array into an int
//int get_int(const char *str, int startByte)
//{
//  int i = (int) ((str[startByte] << 24) | (str[startByte+1] << 16) | str[startByte+2] << 8) | (str[startByte+3]);
//  return i;
//}

void loop()
{
  myusb.Task();

//  if (Serial.available()) {
//    int ch = Serial.read(); // get the first char.
//    while (Serial.read() != -1) ;
//    if ((ch == 'b') || (ch == 'B')) {
//      Serial.println("Only notify on Basic Axis changes");
//      joystick1.axisChangeNotifyMask(0x3ff);
//    } else if ((ch == 'f') || (ch == 'F')) {
//      Serial.println("Only notify on Full Axis changes");
//      joystick1.axisChangeNotifyMask(joystick_full_notify_mask);
//
//    } else {
//      if (show_changed_only) {
//        show_changed_only = false;
//        Serial.println("\n*** Show All fields mode ***");
//      } else {
//        show_changed_only = true;
//        Serial.println("\n*** Show only changed fields mode ***");
//      }
//    }
//  }

  // read serial in data until completion
  while(Serial1.available()) {
    // echo serial outputs
    //Serial.write(Serial1.read());
    ProcessSerial(Serial1.read());
  }
  //serial_buttons_value = 0;
  if(stringComplete) {

    if(inputString == "ml") {
      //Serial.println("clicking");
      noInterrupts();
      bool bMouse4Down = (get_buttons & MOUSE_BACK) > 0;
      interrupts();
      if(bMouse4Down) {
        serial_buttons_value |= MOUSE_LEFT;
      }
    } else if(inputString == "mlu") { // mouse left up
      serial_buttons_value &= ~MOUSE_LEFT;
    } else if(inputString == "mr") { // MR mouse right
      serial_buttons_value |= MOUSE_RIGHT;
    } else if(inputString == "mru") { // mouse right up
      serial_buttons_value &= ~MOUSE_RIGHT;
    } else if(prefix("mv", inputString.c_str())) {
      if(argc >= 2) {
        mvx = args[0];
        mvy = args[1];
        Serial.printf("mouse move x:%i y:%i",mvx,mvy);
      }
    }
    serial_buttons_value &= MOUSE_ALL;
    // reset command string
    inputString = "";
    argc = 0;
    stringComplete = false;
  }

  for (uint8_t i = 0; i < CNT_DEVICES; i++) {
    if (*drivers[i] != driver_active[i]) {
      if (driver_active[i]) {
        Serial.printf("*** Device %s - disconnected ***\n", driver_names[i]);
        driver_active[i] = false;
      } else {
        Serial.printf("*** Device %s %x:%x - connected ***\n", driver_names[i], drivers[i]->idVendor(), drivers[i]->idProduct());
        driver_active[i] = true;

        const uint8_t *psz = drivers[i]->manufacturer();
        if (psz && *psz) Serial.printf("  manufacturer: %s\n", psz);
        psz = drivers[i]->product();
        if (psz && *psz) Serial.printf("  product: %s\n", psz);
        psz = drivers[i]->serialNumber();
        if (psz && *psz) Serial.printf("  Serial: %s\n", psz);

        // Note: with some keyboards there is an issue that they don't output in boot protocol mode
        // and may not work.  The above code can try to force the keyboard into boot mode, but there
        // are issues with doing this blindly with combo devices like wireless keyboard/mouse, which
        // may cause the mouse to not work.  Note: the above id is in the builtin list of
        // vendor IDs that are already forced
        if (drivers[i] == &keyboard1) {
          if (keyboard1.idVendor() == 0x04D9) {
            Serial.println("Gigabyte vendor: force boot protocol");
            // Gigabyte keyboard
            keyboard1.forceBootProtocol();
          }
        }
      }
    }
  }

  for (uint8_t i = 0; i < CNT_HIDDEVICES; i++) {
    if (*hiddrivers[i] != hid_driver_active[i]) {
      if (hid_driver_active[i]) {
        Serial.printf("*** HID Device %s - disconnected ***\n", hid_driver_names[i]);
        hid_driver_active[i] = false;
      } else {
        Serial.printf("*** HID Device %s %x:%x - connected ***\n", hid_driver_names[i], hiddrivers[i]->idVendor(), hiddrivers[i]->idProduct());
        hid_driver_active[i] = true;

        const uint8_t *psz = hiddrivers[i]->manufacturer();
        if (psz && *psz) Serial.printf("  manufacturer: %s\n", psz);
        psz = hiddrivers[i]->product();
        if (psz && *psz) Serial.printf("  product: %s\n", psz);
        psz = hiddrivers[i]->serialNumber();
        if (psz && *psz) Serial.printf("  Serial: %s\n", psz);
      }
    }
  }

  // copy data to and from the interrupt storage
  noInterrupts();
  // set the output button state plus the old button state from last frame for debouncing
  usb_mouse_buttons_state = (get_buttons | serial_buttons_value/* | old_buttons_value*/) & (MOUSE_ALL & ~MOUSE_BACK) & MOUSE_ALL;
  // set the old button frame for next debounce
  //old_buttons_value = get_buttons;
  interrupts();
  if (mouse1.available()) {
    Mouse.move((int16_t)mouse1.getMouseX()+(int16_t)mvx,mouse1.getMouseY()+mvy);
    Mouse.scroll(mouse1.getWheel());

    //usb_mouse_move((int16_t)mouse1.getMouseX()+(int16_t)mvx,mouse1.getMouseY()+mvy,mouse1.getWheel(),0);

    // clear the mouse data (EXCEPT IMPORTANT ########################### WE MODIFIED THE LIBRARY TO NOT CLEAR BUTTON PRESSES)
    mouse1.mouseDataClear();
  } else {
    // mouse move 0,0 to make it send a packet anyways when its only button presses
    // WE MODIFIED THE MOUSE MOVE LIBRARY FUNCTION TO SUBMIT 16BIT NUMBERS NOT 8
    Mouse.move((int16_t)mvx,(int16_t)mvy);
   // usb_mouse_move(mvx,mvy,0,0);
  }
  // reset the move command values after use so they don't get re used
  mvx = 0;
  mvy = 0;
}

// interupt that runs once every MS to read mouse data at 1KHZ polling rate
void readMouseIn() {
  // get the buttons currently pressed with the all buttons mask so we don't have any extra bits
  get_buttons = mouse1.getButtons()& MOUSE_ALL;

//  if (get_buttons > 0) {
//      sinceMouseData = 0;
//  }
//  if(old_buttons_value != get_buttons) {
//    if(get_buttons == 0) {
//      if(sinceMouseData<30) {
//        get_buttons = old_buttons_value;
//      }
//    }
//  }
}

void ProcessSerial(char inChar) {
  static long receivedNumber = 0;
  static boolean negative = false;
  
  switch(inChar)
    {
      case '\n':
        stringComplete = true;
        //Serial.write("ending string");
        break;
      case endOfNumberDelimiter:
        if(negative)
          args[argc-1] = -receivedNumber;
        else
          args[argc-1] = receivedNumber;
        break;
      case startOfNumberDelimiter:
        argc++;
        receivedNumber = 0;
        negative = false;
        break;
      case '0' ... '9':
        receivedNumber *= 10;
        receivedNumber += inChar - '0';
        break;
      case '-':
        negative = true;
        break;
      default:
        //Serial.write("got char: ");
        //Serial.write(inChar);
        //Serial.write("\n");
        inputString += inChar;
    }
}
