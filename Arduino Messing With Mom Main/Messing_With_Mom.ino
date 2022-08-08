// ask_receiver.pde
// -*- mode: C++ -*-
// Simple example of how to use RadioHead to receive messages
// with a simple ASK transmitter in a very simple way.
// Implements a simplex (one-way) receiver with an Rx-B1 module
// Tested on Arduino Mega, Duemilanova, Uno, Due, Teensy, ESP-12

#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif
#include "string.h"
#include <Arduino.h>
#include <IRremote.hpp>

#define NO_LED_FEEDBACK_CODE    // part of IRremote library, not using an external LED
#define LIVINGROOM_SEND_PIN 3   // connected to set of IR emitter LEDs for sending commands to living room TV
#define BEDROOM_SEND_PIN 9      // for sending commands to bedroom TV

RH_ASK driver;

// Commands are saved in 2 arrays, 7 elements long
                                // power   , home      , mute      , menu(*)   , netflix
const uint32_t sLivingRoom[5] = {0xE817C7EA, 0xFC03C7EA, 0xDF20C7EA, 0x9E61C7EA, 0xAD52C7EA};   //NEC Protocol

                              // power  , input     , mute      , menu      , info
const uint32_t sBedRoom[5] = {0xED12C738, 0xEC13C738, 0xE718C738, 0xE817C738, 0xF30CC738};  //NEC Protocol

bool sendCommand(char cmd[]);
void switchTV();

bool livingRoom = true;

// Method for sending commands to the TVs
bool sendCommand(char cmd[]){
  uint8_t choice;
  
  Serial.print("Sending command: ");
  Serial.println(cmd);
  Serial.flush();
  if(!strcmp(cmd, "Swtch")){
    switchTV();
  }
  else if(!strcmp(cmd, "Power")){
    choice = 0;
  } 
  else if(!strcmp(cmd, "OnOff")){   //placeholder for Input/Home
    choice = 1;
  }
  else if(!strcmp(cmd, "Muted")){
    choice = 2;
  } 
  else if(!strcmp(cmd, "Menus")){
    choice = 3;
  }
  else if(!strcmp(cmd, "Ntflx")){   //Netflix or Info button
    choice = 4;
  } 
  else {
    return false;
  }
  
  if (livingRoom){
    IrSender.sendNECRaw(sLivingRoom[choice], 0);
    //IrSender.sendSony(0x1 & 0x1F, 0x15 & 0x7F, 2);    // for testing on my TV, power command, 2 repeats
  } 
  else {
    IrSender.sendNECRaw(sBedRoom[choice], 0);
    //IrSender.sendSony(0x1 & 0x1F, 0x15 & 0x7F, 2);    // for testing on my TV, power command, 2 repeats
  }
  return true;
}


// Method of switching between living room TV and bedroom TV. The setSendPin function changes between the IR emitter LEDs
void switchTV(){
  if(livingRoom){
    Serial.println("Switch to bedroom");
    livingRoom = false;
    IrSender.setSendPin(BEDROOM_SEND_PIN);
  } else {
    Serial.println("Switch to living room");
    livingRoom = true;
    IrSender.setSendPin(LIVINGROOM_SEND_PIN);
  }
  Serial.flush();   // wait to sleep after all serial data is transmitted
}

void setup()
{
#ifdef RH_HAVE_SERIAL
    Serial.begin(115200);    // Debugging only
#endif
    if (!driver.init())
#ifdef RH_HAVE_SERIAL
         Serial.println("init failed");
#else
  ;
#endif
  IrSender.begin(LIVINGROOM_SEND_PIN); // Start with pin 3 as send pin
}

void loop()
{
  uint8_t buf[5], str[6];
  uint8_t buflen = sizeof(buf);

  if (driver.recv(buf, &buflen)) // Non-blocking
  {
    strcpy(str, buf);
    str[5] = 0;
    //Serial.println((char*)str);
    // Message with a good checksum received, dump it.
    //driver.printBuffer("Got:", buf, buflen);
    sendCommand(str);
  }
}
