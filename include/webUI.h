#pragma once

/* I N C L U D E S ****************************************************/
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <EspWebUI.h>
#include <config.h>
#include <language.h>

extern EspWebUI webUI;

struct s_example {
  int temp1 = 10;
  int setTemp = 20;
  int actTemp = 30;
  int opmode = 0;
  int tabval[5] = {1, 2, 3, 4, 5};
};

extern s_example example;

/* P R O T O T Y P E S ********************************************************/
void webUISetup();
void webUICyclic();
void webReadLogBuffer();
