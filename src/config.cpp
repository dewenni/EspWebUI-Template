#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <basics.h>
#include <config.h>
#include <message.h>

/* D E C L A R A T I O N S ****************************************************/

const int CFG_VERSION = 1;

char filename[24] = {"/config.json"};
bool setupMode;
bool configInitDone = false;
unsigned long hashOld;
s_config config;
muTimer checkTimer = muTimer(); // timer to refresh other values
static const char *TAG = "CFG"; // LOG TAG
char encrypted[256] = {0};
char decrypted[128] = {0};
const unsigned char key[16] = {0x6d, 0x79, 0x5f, 0x73, 0x65, 0x63, 0x75, 0x72, 0x65, 0x5f, 0x6b, 0x65, 0x79, 0x31, 0x32, 0x33};

/* P R O T O T Y P E S ********************************************************/
void checkGPIO();
void configInitValue();
void configFinalCheck();

/**
 * *******************************************************************
 * @brief   Setup for intitial configuration
 * @param   none
 * @return  none
 * *******************************************************************/
void configSetup() {
  // start Filesystem
  if (LittleFS.begin(true)) {
    ESP_LOGI(TAG, "LittleFS successfully started");
  } else {
    ESP_LOGE(TAG, "LittleFS error");
  }

  // load config from file
  configLoadFromFile();
  // check final settings
  configFinalCheck();
}

/**
 * *******************************************************************
 * @brief   init hash value
 * @param   none
 * @return  none
 * *******************************************************************/
void configHashInit() {
  hashOld = EspStrUtil::hash(&config, sizeof(s_config));
  configInitDone = true;
}

/**
 * *******************************************************************
 * @brief   cyclic function for configuration
 * @param   none
 * @return  none
 * *******************************************************************/
void configCyclic() {

  if (checkTimer.cycleTrigger(1000) && configInitDone) {
    unsigned long hashNew = EspStrUtil::hash(&config, sizeof(s_config));
    if (hashNew != hashOld) {
      hashOld = hashNew;
      configSaveToFile();
    }
  }
}

/**
 * *******************************************************************
 * @brief   intitial configuration values
 * @param   none
 * @return  none
 * *******************************************************************/
void configInitValue() {

  memset((void *)&config, 0, sizeof(config));

  // Logger
  config.log.level = 4;

  // WiFi
  config.wifi.enable = true;
  snprintf(config.wifi.hostname, sizeof(config.wifi.hostname), "EspWebUI");

  // MQTT
  config.mqtt.port = 1883;
  config.mqtt.enable = false;
  snprintf(config.mqtt.ha_topic, sizeof(config.mqtt.ha_topic), "homeassistant");
  snprintf(config.mqtt.ha_device, sizeof(config.mqtt.ha_device), "EspWebUI");

  // NTP
  snprintf(config.ntp.server, sizeof(config.ntp.server), "de.pool.ntp.org");
  snprintf(config.ntp.tz, sizeof(config.ntp.tz), "CET-1CEST,M3.5.0,M10.5.0/3");
  config.ntp.enable = true;

  // language
  config.lang = 0;
}

/**
 * *******************************************************************
 * @brief   save configuration to file
 * @param   none
 * @return  none
 * *******************************************************************/
void configSaveToFile() {

  JsonDocument doc; // reserviert 2048 Bytes für das JSON-Objekt

  doc["version"] = CFG_VERSION;

  doc["lang"] = (config.lang);

  doc["wifi"]["enable"] = config.wifi.enable;
  doc["wifi"]["ssid"] = config.wifi.ssid;

  if (EspStrUtil::encryptPassword(config.wifi.password, key, encrypted, sizeof(encrypted))) {
    doc["wifi"]["password"] = encrypted;
  } else {
    ESP_LOGE(TAG, "error encrypting WiFi Password");
  }

  doc["wifi"]["hostname"] = config.wifi.hostname;
  doc["wifi"]["static_ip"] = config.wifi.static_ip;
  doc["wifi"]["ipaddress"] = config.wifi.ipaddress;
  doc["wifi"]["subnet"] = config.wifi.subnet;
  doc["wifi"]["gateway"] = config.wifi.gateway;
  doc["wifi"]["dns"] = config.wifi.dns;

  doc["eth"]["enable"] = config.eth.enable;
  doc["eth"]["hostname"] = config.eth.hostname;
  doc["eth"]["static_ip"] = config.eth.static_ip;
  doc["eth"]["ipaddress"] = config.eth.ipaddress;
  doc["eth"]["subnet"] = config.eth.subnet;
  doc["eth"]["gateway"] = config.eth.gateway;
  doc["eth"]["dns"] = config.eth.dns;
  doc["eth"]["gpio_sck"] = config.eth.gpio_sck;
  doc["eth"]["gpio_mosi"] = config.eth.gpio_mosi;
  doc["eth"]["gpio_miso"] = config.eth.gpio_miso;
  doc["eth"]["gpio_cs"] = config.eth.gpio_cs;
  doc["eth"]["gpio_irq"] = config.eth.gpio_irq;
  doc["eth"]["gpio_rst"] = config.eth.gpio_rst;

  doc["mqtt"]["enable"] = config.mqtt.enable;
  doc["mqtt"]["server"] = config.mqtt.server;
  doc["mqtt"]["user"] = config.mqtt.user;

  if (EspStrUtil::encryptPassword(config.mqtt.password, key, encrypted, sizeof(encrypted))) {
    doc["mqtt"]["password"] = encrypted;
  } else {
    ESP_LOGE(TAG, "error encrypting mqtt Password");
  }

  doc["mqtt"]["topic"] = config.mqtt.topic;
  doc["mqtt"]["port"] = config.mqtt.port;
  doc["mqtt"]["ha_enable"] = config.mqtt.ha_enable;
  doc["mqtt"]["ha_topic"] = config.mqtt.ha_topic;
  doc["mqtt"]["ha_device"] = config.mqtt.ha_device;

  doc["ntp"]["enable"] = config.ntp.enable;
  doc["ntp"]["server"] = config.ntp.server;
  doc["ntp"]["tz"] = config.ntp.tz;

  doc["auth"]["enable"] = config.auth.enable;
  doc["auth"]["user"] = config.auth.user;
  doc["auth"]["password"] = config.auth.password;

  doc["logger"]["enable"] = config.log.enable;
  doc["logger"]["level"] = config.log.level;
  doc["logger"]["order"] = config.log.order;

  // Delete existing file, otherwise the configuration is appended to the file
  LittleFS.remove(filename);

  // Open file for writing
  File file = LittleFS.open(filename, FILE_WRITE);
  if (!file) {
    ESP_LOGE(TAG, "Failed to create file");
    return;
  }

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    ESP_LOGE(TAG, "Failed to write to file");
  } else {
    ESP_LOGI(TAG, "config successfully saved to file: %s - Version: %i", filename, CFG_VERSION);
  }

  // Close the file
  file.close();
}

/**
 * *******************************************************************
 * @brief   load configuration from file
 * @param   none
 * @return  none
 * *******************************************************************/
void configLoadFromFile() {
  // Open file for reading
  File file = LittleFS.open(filename);

  // Allocate a temporary JsonDocument
  JsonDocument doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    ESP_LOGE(TAG, "SETUP-MODE-REASON: Failed to read file, using default configuration");
    configInitValue();
    setupMode = true;
    return;
  }

  config.version = doc["version"];

  config.lang = doc["lang"];

  config.wifi.enable = doc["wifi"]["enable"];
  EspStrUtil::readJSONstring(config.wifi.ssid, sizeof(config.wifi.ssid), doc["wifi"]["ssid"]);

  if (config.version == 0) {
    EspStrUtil::readJSONstring(config.wifi.password, sizeof(config.wifi.password), doc["wifi"]["password"]);
  } else {
    EspStrUtil::readJSONstring(encrypted, sizeof(encrypted), doc["wifi"]["password"]);
    if (EspStrUtil::decryptPassword(encrypted, key, config.wifi.password, sizeof(config.wifi.password))) {
      // ESP_LOGD(TAG, "decrypted WiFi password: %s", config.wifi.password);
    } else {
      ESP_LOGE(TAG, "error decrypting WiFi password");
    }
  }

  EspStrUtil::readJSONstring(config.wifi.hostname, sizeof(config.wifi.hostname), doc["wifi"]["hostname"]);
  config.wifi.static_ip = doc["wifi"]["static_ip"];
  EspStrUtil::readJSONstring(config.wifi.ipaddress, sizeof(config.wifi.ipaddress), doc["wifi"]["ipaddress"]);
  EspStrUtil::readJSONstring(config.wifi.subnet, sizeof(config.wifi.subnet), doc["wifi"]["subnet"]);
  EspStrUtil::readJSONstring(config.wifi.gateway, sizeof(config.wifi.gateway), doc["wifi"]["gateway"]);
  EspStrUtil::readJSONstring(config.wifi.dns, sizeof(config.wifi.dns), doc["wifi"]["dns"]);

  config.eth.enable = doc["eth"]["enable"];
  EspStrUtil::readJSONstring(config.eth.hostname, sizeof(config.eth.hostname), doc["eth"]["hostname"]);
  config.eth.static_ip = doc["eth"]["static_ip"];
  EspStrUtil::readJSONstring(config.eth.ipaddress, sizeof(config.eth.ipaddress), doc["eth"]["ipaddress"]);
  EspStrUtil::readJSONstring(config.eth.ipaddress, sizeof(config.eth.ipaddress), doc["eth"]["ipaddress"]);
  EspStrUtil::readJSONstring(config.eth.subnet, sizeof(config.eth.subnet), doc["eth"]["subnet"]);
  EspStrUtil::readJSONstring(config.eth.gateway, sizeof(config.eth.gateway), doc["eth"]["gateway"]);
  EspStrUtil::readJSONstring(config.eth.dns, sizeof(config.eth.dns), doc["eth"]["dns"]);
  config.eth.gpio_sck = doc["eth"]["gpio_sck"];
  config.eth.gpio_mosi = doc["eth"]["gpio_mosi"];
  config.eth.gpio_miso = doc["eth"]["gpio_miso"];
  config.eth.gpio_cs = doc["eth"]["gpio_cs"];
  config.eth.gpio_irq = doc["eth"]["gpio_irq"];
  config.eth.gpio_rst = doc["eth"]["gpio_rst"];

  config.mqtt.enable = doc["mqtt"]["enable"];
  EspStrUtil::readJSONstring(config.mqtt.server, sizeof(config.mqtt.server), doc["mqtt"]["server"]);
  EspStrUtil::readJSONstring(config.mqtt.user, sizeof(config.mqtt.user), doc["mqtt"]["user"]);

  EspStrUtil::readJSONstring(config.mqtt.password, sizeof(config.mqtt.password), doc["mqtt"]["password"]);

  if (config.version == 0) {
    EspStrUtil::readJSONstring(config.mqtt.password, sizeof(config.mqtt.password), doc["mqtt"]["password"]);
  } else {
    EspStrUtil::readJSONstring(encrypted, sizeof(encrypted), doc["mqtt"]["password"]);
    if (EspStrUtil::decryptPassword(encrypted, key, config.mqtt.password, sizeof(config.mqtt.password))) {
      // ESP_LOGD(TAG, "decrypted mqtt password: %s", config.mqtt.password);
    } else {
      ESP_LOGE(TAG, "error decrypting mqtt password");
    }
  }

  EspStrUtil::readJSONstring(config.mqtt.topic, sizeof(config.mqtt.topic), doc["mqtt"]["topic"]);
  config.mqtt.port = doc["mqtt"]["port"];
  config.mqtt.ha_enable = doc["mqtt"]["ha_enable"];
  EspStrUtil::readJSONstring(config.mqtt.ha_topic, sizeof(config.mqtt.ha_topic), doc["mqtt"]["ha_topic"]);
  EspStrUtil::readJSONstring(config.mqtt.ha_device, sizeof(config.mqtt.ha_device), doc["mqtt"]["ha_device"]);

  config.ntp.enable = doc["ntp"]["enable"];
  EspStrUtil::readJSONstring(config.ntp.server, sizeof(config.ntp.server), doc["ntp"]["server"]);
  EspStrUtil::readJSONstring(config.ntp.tz, sizeof(config.ntp.tz), doc["ntp"]["tz"]);

  config.auth.enable = doc["auth"]["enable"];
  EspStrUtil::readJSONstring(config.auth.user, sizeof(config.auth.user), doc["auth"]["user"]);
  EspStrUtil::readJSONstring(config.auth.password, sizeof(config.auth.password), doc["auth"]["password"]);

  config.log.enable = doc["logger"]["enable"];
  config.log.level = doc["logger"]["level"];
  config.log.order = doc["logger"]["order"];

  file.close();     // Close the file (Curiously, File's destructor doesn't close the file)
  configHashInit(); // init hash value

  // save config if version is different
  if (config.version != CFG_VERSION) {
    configSaveToFile();
    ESP_LOGI(TAG, "config file was updated from version %i to version: %i", config.version, CFG_VERSION);
  } else {
    ESP_LOGD(TAG, "config file version %i was successfully loaded", config.version);
  }
}

void configFinalCheck() {

  // check network settings
  if (strlen(config.wifi.ssid) == 0 && config.wifi.enable) {
    // no valid wifi setting => start AP-Mode
    ESP_LOGW(TAG, "SETUP-MODE-REASON: no valid wifi SSID set");
    setupMode = true;
  } else if (config.wifi.enable == false && config.eth.enable == false) {
    // no network enabled => start AP-Mode
    ESP_LOGW(TAG, "SETUP-MODE-REASON: WiFi and ETH disabled");
    setupMode = true;
  }

  setLogLevel(config.log.level);
}