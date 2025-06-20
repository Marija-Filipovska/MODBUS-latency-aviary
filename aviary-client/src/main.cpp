#include "../../setup.h"
#include <SoftwareSerial.h>
EspSoftwareSerial::UART SoftSerial(26, 25, false);
constexpr EspSoftwareSerial::Config swSerialConfig = EspSoftwareSerial::SWSERIAL_8N1;
constexpr SerialConfig hwSerialConfig = ::SERIAL_8N1;

hw_timer_t *timer = NULL;
uint32_t startTime, endTime;

int server_select = 0;
bool block = false;
bool coilsA[1];
bool coilsB[1];

#ifdef proj_clientlib_modbus_esp8266
#include <ModbusRTU.h>

ModbusRTU mb;
//callback that executes after a modbus request is placed and it prints result in memory
bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  endTime = timerRead(timer);
  //Serial.printf_P("Request result: 0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
  //Serial.print(coilsA[0]);
  //Serial.print(" ");
  //Serial.println(coilsB[0]);
  Serial.print("T");
  Serial.println(endTime - startTime);
  server_select = 1-server_select;
  block = false;
  return true;
}
#endif

#ifdef proj_clientlib_emodbus
#include "ModbusClientRTU.h"
#define READ_INTERVAL 10
#define FIRST_REGISTER 0x0001
#define NUM_VALUES 1

float values[NUM_VALUES];
bool data_ready = false;
uint32_t request_time;
ModbusClientRTU mb(27);

void handleData(ModbusMessage response, uint32_t token) 
{
  endTime = timerRead(timer);
  Serial.print("T");
  Serial.println(endTime - startTime);
  // First value is on pos 3, after server ID, function code and length byte
  //uint16_t offs = 3;
  // The device has values all as IEEE754 float32 in two consecutive registers
  // Read the requested in a loop
  //Serial.println(response[offs]);
  server_select = 1-server_select;
  block = false;

}

void handleError(Error error, uint32_t token) 
{
  // ModbusError wraps the error code and provides a readable error message for it
  ModbusError me(error);
  Serial.printf("Error response: %02X - %s\n", (int)me, (const char *)me);
  server_select = 1-server_select;
  block = false;
  sleep(1);
}
#endif

void IRAM_ATTR onTimer() {
  // This function will be called every time the timer ticks (every 1µs)
}

void setup() {
  Serial.begin(proj_user_rate); 

  #ifdef proj_clientlib_modbus_esp8266
  SoftSerial.begin(proj_baudrate, swSerialConfig); //uses software serial 
  mb.begin(&SoftSerial, 27, true); 
  mb.master();
  #endif

  #ifdef proj_clientlib_emodbus 
  SoftSerial.begin(proj_baudrate, swSerialConfig); //uses software serial 
  mb.onDataHandler(&handleData);
  mb.onErrorHandler(&handleError);
  mb.setTimeout(300);
  mb.begin(SoftSerial, proj_baudrate);
  #endif

  timer = timerBegin(0, 80, true);
}


// defining array to store state of coils with loop function
void loop() { 

  #ifdef proj_clientlib_modbus_esp8266
  if (!mb.slave() and not block) {
    block = true;
    startTime = timerRead(timer);
    if (server_select == 0 ){
      mb.readCoil(1, 1, coilsA, 1, cbWrite);
    } else if(server_select == 1){
      mb.readCoil(2, 1, coilsB, 1, cbWrite);
    }
  }
  mb.task();
  #endif

  #ifdef proj_clientlib_emodbus
  static unsigned long next_request = millis();

  // Shall we do another request?
  if (not block) {
    // Issue the request
    block = true;
    startTime = timerRead(timer);

    Error err = Modbus::Error::INVALID_SERVER;
    if (server_select == 0 ){
    err = mb.addRequest((uint32_t)millis(), 1, READ_COIL, FIRST_REGISTER, NUM_VALUES * 2);
    } else if(server_select == 1){
    err = mb.addRequest((uint32_t)millis(), 2, READ_COIL, FIRST_REGISTER, NUM_VALUES * 2);
    }

    if (err!=SUCCESS) {
      ModbusError e(err);
      Serial.printf("Error creating request: %02X - %s\n", (int)e, (const char *)e);
      block = false;
    }
    
  }
  #endif
}
