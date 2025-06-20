/*
  ModbusRTU ESP8266/ESP32
  Simple slave example

  (c)2019 Alexander Emelianov (a.m.emelianov@gmail.com)
  https://github.com/emelianov/modbus-esp8266

  modified 13 May 2020
  by brainelectronics

  This code is licensed under the BSD New License. See LICENSE.txt for more info.
*/


#include "../../setup.h"
#include <Arduino.h>
#include <SoftwareSerial.h>

#define PIN_ADR_1 5
#define PIN_ADR_2 6
#define PIN_ADR_4 7
#define PIN_ADR_8 8

int coilState = 0; //can only be state 1 or 0 
int server_id = -1;

#ifdef proj_serverlib_modbus_esp8266
#include <ModbusRTU.h>
//defining register numbers 1 in this case for modbus
ModbusRTU mb; 
#endif

#ifdef proj_serverlib_modbus_serial
#include <ModbusSerial.h>
const int TxenPin = 2;
ModbusSerial mb;
#endif

#ifdef proj_serverlib_emodbus
#include "ModbusServerRTU.h"
const int TxenPin = 2;
ModbusServerRTU mb(1000); // 1000ms timeout

ModbusMessage MessageGenerator(ModbusMessage request) {
  uint16_t address;           // requested register address
  uint16_t words;             // requested number of registers
  ModbusMessage response;     // response message to be sent back

  // get request values
  request.get(2, address);
  request.get(4, words);

  // Address and words valid?
  if (address && words) {
    // Looks okay. Set up message with serverID, FC and length of data
    response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 1));
    // Fill response with requested data
    response.add(coilState);
  } else {
    // No, either address or words are outside the limits. Set up error response.
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
  }
  return response;
}


#endif

void setup() {

	pinMode(PIN_ADR_1, INPUT_PULLUP);
	pinMode(PIN_ADR_2, INPUT_PULLUP);
	pinMode(PIN_ADR_4, INPUT_PULLUP);
	pinMode(PIN_ADR_8, INPUT_PULLUP);

  server_id = !digitalRead(PIN_ADR_8)*8 + !digitalRead(PIN_ADR_4)*4 + !digitalRead(PIN_ADR_2)*2 + !digitalRead(PIN_ADR_1);

  if(server_id == -1){
    while (1){ // blink violently if the serverID is read wrong.
	coilState = 1 - coilState;
	digitalWrite(16, coilState);  // Turn the LED off by making the voltage LOW
	delay(100);
    }
  }
  
  #ifdef proj_serverlib_modbus_esp8266
  Serial.begin(proj_baudrate, SERIAL_8N1); //initializes serial port communication 8 bits, no parity and 1 stop
  // mb.begin(&Serial);
  mb.begin(&Serial, 2);  //or use RX/TX direction control pin (if required)
  mb.setBaudrate(proj_baudrate); //initialization starts at line 30 with set baud rate
  mb.slave(server_id); //configured slave id 
  mb.addCoil(proj_server_reg);
  mb.Coil(proj_server_reg, coilState); //setting normal value to 100 = ON
  #endif

  #ifdef proj_serverlib_modbus_serial
  mb.config(&Serial,proj_baudrate,SERIAL_8N1,TxenPin);
  mb.setSlaveId(server_id);
  mb.addCoil(proj_server_reg);
  mb.Coil(proj_server_reg,coilState);
  #endif
  
  #ifdef proj_serverlib_emodbus 
  RTUutils::prepareHardwareSerial(Serial);
  Serial.begin(proj_baudrate, SERIAL_8N1);

  mb.setTimeout(300);
  mb.begin(&Serial, proj_baudrate);

  mb.registerWorker(server_id, READ_COIL, &MessageGenerator);
  #endif
	
  pinMode(16, OUTPUT); //using pin 16 for output on LED
}

void loop() {
  mb.task(); //process modbus requests and query state of modbus
  
  static unsigned long lastFlipTime = 0;
  if (millis() - lastFlipTime >= 5000) { //toggle coil every 5000ms = 5sec between 0 and 1, 
    coilState = 1-coilState; //done by toggling 0 and 1 
    #ifdef proj_serverlib_modbus_esp8266
    mb.Coil(proj_server_reg, coilState); // new state of coil is sent back to the modbus system using function
    #endif
    #ifdef proj_serverlib_modbus_serial
    mb.Coil(proj_server_reg,coilState);
    #endif
    digitalWrite(16, coilState);  // Turn the LED off by making the voltage LOW
    // Serial.print("Coil state changed to: ");
    lastFlipTime = millis(); // Update the last flip time
  }
}
