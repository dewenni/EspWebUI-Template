#include "EscapeCodes.h"
#include <basics.h>
#include <config.h>
#include <language.h>
#include <message.h>
#include <telnet.h>
#include <webUI.h>

/* D E C L A R A T I O N S ****************************************************/
ESPTelnet telnet;
s_telnetIF telnetIF;
static EscapeCodes ansi;
static char param[MAX_PAR][MAX_CHAR];
static bool msgAvailable = false;
static const char *TAG = "TELNET"; // LOG TAG

/* P R O T O T Y P E S ********************************************************/
void readLogger();
void dispatchCommand(char param[MAX_PAR][MAX_CHAR]);
bool extractMessage(String str, char param[MAX_PAR][MAX_CHAR]);
void cmdHelp(char param[MAX_PAR][MAX_CHAR]);
void cmdCls(char param[MAX_PAR][MAX_CHAR]);
void cmdConfig(char param[MAX_PAR][MAX_CHAR]);
void cmdInfo(char param[MAX_PAR][MAX_CHAR]);
void cmdDisconnect(char param[MAX_PAR][MAX_CHAR]);
void cmdRestart(char param[MAX_PAR][MAX_CHAR]);

Command commands[] = {
    {"cls", cmdCls, "Clear screen", ""},
    {"config", cmdConfig, "config commands", "<reset>"},
    {"disconnect", cmdDisconnect, "disconnect telnet", ""},
    {"help", cmdHelp, "Displays this help message", "[command]"},
    {"info", cmdInfo, "Print system information", ""},
    {"restart", cmdRestart, "Restart the ESP", ""},
};
const int commandsCount = sizeof(commands) / sizeof(commands[0]);

// print telnet shell
void telnetShell() {
  telnet.print(ansi.setFG(ANSI_BRIGHT_GREEN));
  telnet.print("$ >");
  telnet.print(ansi.reset());
}

void onTelnetConnect(String ip) {
  ESP_LOGI(TAG, "Telnet: %s connected", ip.c_str());
  telnet.println(ansi.setFG(ANSI_BRIGHT_GREEN));
  telnet.println("\n----------------------------------------------------------------------");
  telnet.println("\nEspWebUI");
  telnet.println("\nWelcome " + telnet.getIP());
  telnet.println("use command: \"help\" for further information");
  telnet.println("\n----------------------------------------------------------------------\n");
  telnet.println(ansi.reset());
  telnetShell();
}

void onTelnetDisconnect(String ip) { ESP_LOGI(TAG, "Telnet: %s disconnected", ip.c_str()); }

void onTelnetReconnect(String ip) { ESP_LOGI(TAG, "Telnet: %s reconnected", ip.c_str()); }

void onTelnetConnectionAttempt(String ip) { ESP_LOGI(TAG, "Telnet: %s tried to connect", ip.c_str()); }

void onTelnetInput(String str) {
  if (!extractMessage(str, param)) {
    telnet.println("Syntax error");
  } else {
    msgAvailable = true;
  }
  telnetShell();
}

/**
 * *******************************************************************
 * @brief   setup function for Telnet
 * @param   none
 * @return  none
 * *******************************************************************/
void setupTelnet() {
  // passing on functions for various telnet events
  telnet.onConnect(onTelnetConnect);
  telnet.onConnectionAttempt(onTelnetConnectionAttempt);
  telnet.onReconnect(onTelnetReconnect);
  telnet.onDisconnect(onTelnetDisconnect);
  telnet.onInputReceived(onTelnetInput);

  if (telnet.begin(23, false)) {
    ESP_LOGI(TAG, "Telnet Server running!");
  } else {
    ESP_LOGI(TAG, "Telnet Server error!");
  }
}

/**
 * *******************************************************************
 * @brief   cyclic function for Telnet
 * @param   none
 * @return  none
 * *******************************************************************/
void cyclicTelnet() {

  telnet.loop();

  // process incoming messages
  if (msgAvailable) {
    dispatchCommand(param);
    msgAvailable = false;
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: config structure
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdConfig(char param[MAX_PAR][MAX_CHAR]) {

  if (!strcmp(param[1], "reset") && !strcmp(param[2], "")) {
    configInitValue();
    configSaveToFile();
    telnet.println("config was set to defaults");
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: print system information
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdInfo(char param[MAX_PAR][MAX_CHAR]) {

  telnet.print(ansi.setFG(ANSI_BRIGHT_WHITE));
  telnet.println("ESP-INFO");
  telnet.print(ansi.reset());
  telnet.printf("ESP Flash Usage: %s %%\n", EspStrUtil::floatToString((float)ESP.getSketchSize() * 100 / ESP.getFreeSketchSpace(), 1));
  telnet.printf("ESP Heap Usage: %s %%\n", EspStrUtil::floatToString((float)ESP.getFreeHeap() * 100 / ESP.getHeapSize(), 1));
  telnet.printf("ESP MAX Alloc Heap: %s KB\n", EspStrUtil::floatToString((float)ESP.getMaxAllocHeap() / 1000.0, 1));
  telnet.printf("ESP MAX Alloc Heap: %s KB\n", EspStrUtil::floatToString((float)ESP.getMinFreeHeap() / 1000.0, 1));

  telnet.print(ansi.setFG(ANSI_BRIGHT_WHITE));
  telnet.println("\nRESTART - UPTIME");
  telnet.print(ansi.reset());
  char tmpMsg[64];
  getUptime(tmpMsg, sizeof(tmpMsg));
  telnet.printf("Uptime: %s\n", tmpMsg);
  telnet.printf("Restart Reason: %s\n", EspSysUtil::RestartReason::get());

  telnet.print(ansi.setFG(ANSI_BRIGHT_WHITE));
  telnet.println("\nWiFi-INFO");
  telnet.print(ansi.reset());
  telnet.printf("IP-Address: %s\n", wifi.ipAddress);
  telnet.printf("WiFi-Signal: %s %%\n", EspStrUtil::intToString(wifi.signal));
  telnet.printf("WiFi-Rssi: %s dbm\n", EspStrUtil::intToString(wifi.rssi));

  telnet.println();
}

/**
 * *******************************************************************
 * @brief   telnet command: clear output
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdCls(char param[MAX_PAR][MAX_CHAR]) {
  telnet.print(ansi.cls());
  telnet.print(ansi.home());
}

/**
 * *******************************************************************
 * @brief   telnet command: disconnect
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdDisconnect(char param[MAX_PAR][MAX_CHAR]) {
  telnet.println("disconnect");
  telnet.disconnectClient();
}

/**
 * *******************************************************************
 * @brief   telnet command: restart ESP
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdRestart(char param[MAX_PAR][MAX_CHAR]) {
  telnet.println("ESP will restart - you have to reconnect");
  EspSysUtil::RestartReason::saveLocal("telnet command");
  yield();
  delay(1000);
  yield();
  ESP.restart();
}

/**
 * *******************************************************************
 * @brief   telnet command: sub function to print help
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void printHelp(const char *command = nullptr) {
  if (command == nullptr) {
    // print help of all commands
    for (int i = 0; i < commandsCount; ++i) {
      telnet.printf("%-15s %-60s\n", commands[i].name, commands[i].parameters);
    }
  } else {
    // print help of specific command
    for (int i = 0; i < commandsCount; ++i) {
      if (strcmp(command, commands[i].name) == 0) {
        telnet.printf("%-15s %-60s\n%s\n", commands[i].name, commands[i].parameters, commands[i].description);
        return;
      }
    }
    telnet.println("Unknown command. Use 'help' to see all commands.");
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: print help
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdHelp(char param[MAX_PAR][MAX_CHAR]) {
  if (strlen(param[1]) > 0) {
    printHelp(param[1]);
  } else {
    printHelp();
  }
}

/**
 * *******************************************************************
 * @brief   check receives telnet message and extract to param array
 * @param   str received message
 * @param   param char array of parameters
 * @return  none
 * *******************************************************************/
bool extractMessage(String str, char param[MAX_PAR][MAX_CHAR]) {
  const char *p = str.c_str();
  int i = 0, par = 0;
  // initialize parameter strings
  for (int j = 0; j < MAX_PAR; j++) {
    memset(&param[j], 0, sizeof(param[0]));
  }
  // extract answer into parameter
  while (*p != '\0') {
    if (i >= MAX_CHAR - 1) {
      param[par][i] = '\0';
      return false;
    }
    if (*p == ' ' || i == MAX_CHAR - 1) {
      param[par][i] = '\0';
      par++;
      p++;
      i = 0;
      if (par >= MAX_PAR)
        return false;
    }
    param[par][i] = *p++;
    i++;
  }
  return true;
}

/**
 * *******************************************************************
 * @brief   telnet command dispatcher
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void dispatchCommand(char param[MAX_PAR][MAX_CHAR]) {
  for (int i = 0; i < commandsCount; ++i) {
    if (!strcmp(param[0], commands[i].name)) {
      commands[i].function(param);
      telnetShell();
      return;
    }
  }
  telnet.println("Unknown command");
  telnetShell();
}
