// includes
#include <ArduinoOTA.h>
#include <basics.h>
#include <config.h>
#include <message.h>
#include <mqtt.h>
#include <telnet.h>
#include <webUI.h>
#include <webUIupdates.h>

/* D E C L A R A T I O N S ****************************************************/
static muTimer heartbeat = muTimer();      // timer for heartbeat signal
static muTimer setupModeTimer = muTimer(); // timer for heartbeat signal
static muTimer wdtTimer = muTimer();       // timer to reset wdt

static EspSysUtil::MRD32 *mrd;  // Multi-Reset-Detector
static bool main_reboot = true; // reboot flag

static const char *TAG = "MAIN"; // LOG TAG

static auto &wdt = EspSysUtil::Wdt::getInstance();
static auto &ota = EspSysUtil::OTA::getInstance();

/**
 * *******************************************************************
 * @brief   Main Setup routine
 * @param   none
 * @return  none
 * *******************************************************************/
void setup() {

  // Message Service Setup (before use of MY_LOGx)
  messageSetup();

  // check for double reset
  mrd = new EspSysUtil::MRD32(MRD_TIMEOUT, MRD_RETRIES);
  if (mrd->detectMultipleResets()) {
    ESP_LOGI(TAG, "SETUP-MODE-REASON: MRD detected");
    setupMode = true;
  }

  // initial configuration (can also activate the Setup Mode)
  configSetup();

  // setup watchdog timer
  if (!setupMode) {
    wdt.enable();
  }

  // basic setup functions
  basicSetup();

  // Setup OTA
  ArduinoOTA.onStart([]() {
    ESP_LOGI(TAG, "OTA-started");
    wdt.disable(); // disable watchdog timer
    ota.setActive(true);
  });
  ArduinoOTA.onEnd([]() {
    ESP_LOGI(TAG, "OTA-finished");
    if (!setupMode) {
      wdt.enable();
    }
    ota.setActive(false);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    ESP_LOGI(TAG, "OTA-error");
    if (!setupMode) {
      wdt.enable();
    }
    ota.setActive(false);
  });
  ArduinoOTA.setHostname(config.wifi.hostname);
  ArduinoOTA.begin();

  
  // webUI Setup
  webUISetup();

  // telnet Setup
  setupTelnet();
}

/**
 * *******************************************************************
 * @brief   Main Loop
 * @param   none
 * @return  none
 * *******************************************************************/
void loop() {

  // reset watchdog
  if (wdt.isActive() && wdtTimer.cycleTrigger(2000)) {
    esp_task_wdt_reset();
  }

  // OTA Update
  ArduinoOTA.handle();

  // double reset detector
  mrd->loop();

  // webUI Cyclic
  webUICyclic();

  // Message Service
  messageCyclic();

  // telnet communication
  cyclicTelnet();

  // check if config has changed
  configCyclic();

  // check WiFi - automatic reconnect
  if (!setupMode) {
    checkWiFi();
  }

  // check MQTT - automatic reconnect
  if (config.mqtt.enable && !setupMode) {
    mqttCyclic();
  }


  main_reboot = false; // reset reboot flag
}
