//#include <Arduino.h>
//#include <SPI.h>
//#include "Adafruit_BLE.h"
//#include "Adafruit_BluefruitLE_SPI.h"
//#include "Adafruit_BluefruitLE_UART.h"
////#include "Adafruit_BluefruitLE_SEcurity.h"
//
//#include "utils.h"
#include "debug.h"
#include "BluefruitConfig.h"

/*=========================================================================
    APPLICATION SETTINGS

    FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
   
                              Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                              running this at least once is a good idea.
   
                              When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                              Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
       
                              Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
    MODE_LED_BEHAVIOUR        LED activity, valid options are
                              "DISABLE" or "MODE" or "BLEUART" or
                              "HWUART"  or "SPI"  or "MANUAL"
    -----------------------------------------------------------------------*/
    #define FACTORYRESET_ENABLE         0
    #define MINIMUM_FIRMWARE_VERSION    "0.6.6"
    #define MODE_LED_BEHAVIOUR          "MODE"
/*=========================================================================*/
#ifdef __cplusplus
 extern "C" {
#endif

/* ==== Serial Input Command State ==== */

// for command receiving
//static String inputString = "";
static char inputString[BLE_BUFSIZE+1];
static bool stringComplete = false;

// receiving move commands
static int argc = 0;
// max 4 args
static int args[4] = {0,0,0,0};
static uint16_t mvx,mvy = 0;

// syntax for receiving numbers
const char startOfNumberDelimiter = '<';
const char endOfNumberDelimiter = '>';

void reset_cmd_input(void) {
//  inputString = "";
    argc = 0;
}

/* ==================================== */

// Create the bluefruit object
/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

void setup_ble_device() {
  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in COMMAND mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  #if FACTORYRESET_ENABLE
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset(true) ){
      error(F("Couldn't factory reset"));
    }

    Serial.println(F("Configuring device preferences... "));
    ble.println("AT+GAPDEVNAME=Segfault 0x1337");
  #endif
  
  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
  Serial.println(F("Then Enter characters to send to Bluefruit"));
  Serial.println();

  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  while (! ble.isConnected()) {
      delay(500);
  }

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("******************************"));
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
    Serial.println(F("******************************"));
  }
}

void ProcessSerial(char inChar) {
  static long receivedNumber = 0;
  static boolean negative = false;
  
  switch(inChar)
    {
      case '\n':
        //stringComplete = true;
        //Serial.write("ending string");
        break;
      case '\r':
        //stringComplete = true;
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
        //inputString += inChar;
        break;
    }
}

// TODO: run ProcessSerial on each char as they come in
uint16_t readline(char * buf, uint16_t bufsize, uint16_t timeout, boolean multiline)
{
  uint16_t replyidx = 0;

  while (timeout--) {
    while(ble.available()) {
      yield();
      char c = ble.read();
      //SerialDebug.println(c);
      
      ProcessSerial(c);

      if (c == '\r') continue;
      if (c == '\n') {
        // the first '\n' is ignored
        // ignore if \n is the first character in the line
        if (replyidx == 0) continue;

//        stringComplete = true;

        timeout = 0;
        break;
        
      }
      
      buf[replyidx] = c;
      replyidx++;

      // Buffer is full
      if (replyidx >= bufsize) {
        DBGPRINTLN("*overflow*");  // for my debuggin' only!
        timeout = 0;
        break;
      }
    }

    // delay if needed
    if (timeout) delay(1);
  }

  buf[replyidx] = 0;  // null term

  // Print out if debug
//  #if DEBUG
//  if (replyidx > 0)
//  {
//    Serial.print(buf);
//    if (replyidx < bufsize) Serial.println();
//  }
//  #endif

  return replyidx;
}

uint16_t readln(void) {
  return readline(ble.buffer, BLE_BUFSIZE, BLE_DEFAULT_TIMEOUT, false);
}

void receive_uart() {
  //   Check for incoming characters from Bluefruit
  ble.println("AT+BLEUARTRX");
  
  // read the incoming line
  
  while ( readln() ) {
    if ( !strncmp(ble.buffer, "OK", strlen("OK")) || !strncmp(ble.buffer, "ERROR", strlen("ERROR"))  ) {
//      reset_cmd_input();
      break;
    } else {
      DBGPRINT(F("[R] ")); DBGPRINTLN(ble.buffer);
      strcpy(inputString, ble.buffer);
      stringComplete = true;
      boop();
    }
  }
}

#ifdef __cplusplus
 }
#endif
