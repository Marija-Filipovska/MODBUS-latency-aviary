#pragma once

static constexpr auto proj_baudrate = 9600;
static constexpr auto proj_user_rate = 460800;
static constexpr auto proj_server_reg = 1;

// Selector for library focused code
// #define proj_clientlib_modbus_esp8266
#define proj_clientlib_emodbus

// #define proj_serverlib_modbus_esp8266
#define proj_serverlib_modbus_serial
// #define proj_serverlib_emodbus