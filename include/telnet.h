#pragma once

/* I N C L U D E S ****************************************************/
#include "ESPTelnet.h"

/* D E C L A R A T I O N S ****************************************************/
// data struct for Telnet interface
struct s_telnetIF {
  bool serialStream;
};

const int MAX_PAR = 3;
const int MAX_CHAR = 64;

struct Command {
  const char *name;
  void (*function)(char param[MAX_PAR][MAX_CHAR]);
  const char *description;
  const char *parameters;
};

extern ESPTelnet telnet;
extern s_telnetIF telnetIF;

/* P R O T O T Y P E S ********************************************************/
void setupTelnet();
void cyclicTelnet();
void telnetShell();