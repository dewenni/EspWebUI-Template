
#include <basics.h>
#include <message.h>
#include <webUI.h>
#include <webUIupdates.h>

static const char *TAG = "WEB"; // LOG TAG

/**
 * *******************************************************************
 * @brief   callback function for web elements
 * @param   elementID
 * @param   value
 * @return  none
 * *******************************************************************/
void webCallback(const char *elementId, const char *value) {

  ESP_LOGD(TAG, "Received - Element ID: %s = %s", elementId, value);

  // ------------------------------------------------------------------
  // GitHub / Version
  // ------------------------------------------------------------------

  // Github Check Version
  if (strcmp(elementId, "check_git_version") == 0 || strcmp(elementId, "p11_check_git_btn") == 0) {
    requestGitHubVersion();
  }
  // Github Update
  if (strcmp(elementId, "p00_update_btn") == 0) {
    requestGitHubUpdate();
  }
  // OTA-Confirm
  if (strcmp(elementId, "p00_ota_confirm_btn") == 0) {
    webUI.wsUpdateWebDialog("ota_update_done_dialog", "close");
    EspSysUtil::RestartReason::saveLocal("ota update");
    yield();
    delay(1000);
    yield();
    ESP.restart();
  }

  // ------------------------------------------------------------------
  // Settings callback
  // ------------------------------------------------------------------

  // WiFi
  if (strcmp(elementId, "cfg_wifi_enable") == 0) {
    config.wifi.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_wifi_hostname") == 0) {
    snprintf(config.wifi.hostname, sizeof(config.wifi.hostname), value);
  }
  if (strcmp(elementId, "cfg_wifi_ssid") == 0) {
    snprintf(config.wifi.ssid, sizeof(config.wifi.ssid), value);
  }
  if (strcmp(elementId, "cfg_wifi_password") == 0) {
    snprintf(config.wifi.password, sizeof(config.wifi.password), value);
  }
  if (strcmp(elementId, "cfg_wifi_static_ip") == 0) {
    config.wifi.static_ip = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_wifi_ipaddress") == 0) {
    snprintf(config.wifi.ipaddress, sizeof(config.wifi.ipaddress), value);
  }
  if (strcmp(elementId, "cfg_wifi_subnet") == 0) {
    snprintf(config.wifi.subnet, sizeof(config.wifi.subnet), value);
  }
  if (strcmp(elementId, "cfg_wifi_gateway") == 0) {
    snprintf(config.wifi.gateway, sizeof(config.wifi.gateway), value);
  }
  if (strcmp(elementId, "cfg_wifi_dns") == 0) {
    snprintf(config.wifi.dns, sizeof(config.wifi.dns), value);
  }

  // Ethernet
  if (strcmp(elementId, "cfg_eth_enable") == 0) {
    config.eth.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_eth_hostname") == 0) {
    snprintf(config.eth.hostname, sizeof(config.eth.hostname), value);
  }
  if (strcmp(elementId, "cfg_eth_gpio_sck") == 0) {
    config.eth.gpio_sck = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_mosi") == 0) {
    config.eth.gpio_mosi = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_miso") == 0) {
    config.eth.gpio_miso = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_cs") == 0) {
    config.eth.gpio_cs = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_irq") == 0) {
    config.eth.gpio_irq = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_rst") == 0) {
    config.eth.gpio_rst = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_static_ip") == 0) {
    config.eth.static_ip = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_eth_ipaddress") == 0) {
    snprintf(config.eth.ipaddress, sizeof(config.eth.ipaddress), value);
  }
  if (strcmp(elementId, "cfg_eth_subnet") == 0) {
    snprintf(config.eth.subnet, sizeof(config.eth.subnet), value);
  }
  if (strcmp(elementId, "cfg_eth_gateway") == 0) {
    snprintf(config.eth.gateway, sizeof(config.eth.gateway), value);
  }
  if (strcmp(elementId, "cfg_eth_dns") == 0) {
    snprintf(config.eth.dns, sizeof(config.eth.dns), value);
  }

  // Authentication
  if (strcmp(elementId, "cfg_auth_enable") == 0) {
    config.auth.enable = EspStrUtil::stringToBool(value);
    webUI.setAuthentication(config.auth.enable);
  }
  if (strcmp(elementId, "cfg_auth_user") == 0) {
    snprintf(config.auth.user, sizeof(config.auth.user), "%s", value);
    webUI.setCredentials(config.auth.user, config.auth.password);
  }
  if (strcmp(elementId, "cfg_auth_password") == 0) {
    snprintf(config.auth.password, sizeof(config.auth.password), "%s", value);
    webUI.setCredentials(config.auth.user, config.auth.password);
  }

  // NTP-Server
  if (strcmp(elementId, "cfg_ntp_enable") == 0) {
    config.ntp.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_ntp_server") == 0) {
    snprintf(config.ntp.server, sizeof(config.ntp.server), "%s", value);
  }
  if (strcmp(elementId, "cfg_ntp_tz") == 0) {
    snprintf(config.ntp.tz, sizeof(config.ntp.tz), "%s", value);
  }

  // MQTT
  if (strcmp(elementId, "cfg_mqtt_enable") == 0) {
    config.mqtt.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_mqtt_server") == 0) {
    snprintf(config.mqtt.server, sizeof(config.mqtt.server), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_port") == 0) {
    config.mqtt.port = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_mqtt_topic") == 0) {
    snprintf(config.mqtt.topic, sizeof(config.mqtt.topic), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_user") == 0) {
    snprintf(config.mqtt.user, sizeof(config.mqtt.user), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_password") == 0) {
    snprintf(config.mqtt.password, sizeof(config.mqtt.password), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_ha_enable") == 0) {
    config.mqtt.ha_enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_mqtt_ha_topic") == 0) {
    snprintf(config.mqtt.ha_topic, sizeof(config.mqtt.ha_topic), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_ha_device") == 0) {
    snprintf(config.mqtt.ha_device, sizeof(config.mqtt.ha_device), "%s", value);
  }

  // Language
  if (strcmp(elementId, "cfg_lang") == 0) {
    config.lang = strtoul(value, NULL, 10);
    updateAllElements();
  }

  // Restart (and save)
  if (strcmp(elementId, "restartAction") == 0) {
    EspSysUtil::RestartReason::saveLocal("webUI command");
    configSaveToFile();
    delay(1000);
    ESP.restart();
  }

  // Logger
  if (strcmp(elementId, "cfg_logger_enable") == 0) {
    config.log.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_logger_level") == 0) {
    config.log.level = strtoul(value, NULL, 10);
    setLogLevel(config.log.level);
    clearLogBuffer();
    webUI.wsUpdateWebLog("", "clr_log"); // clear log
  }
  if (strcmp(elementId, "cfg_logger_order") == 0) {
    config.log.order = strtoul(value, NULL, 10);
    webUI.wsUpdateWebLog("", "clr_log"); // clear log
    webReadLogBuffer();
  }
  if (strcmp(elementId, "p10_log_clr_btn") == 0) {
    clearLogBuffer();
    webUI.wsUpdateWebLog("", "clr_log"); // clear log
  }
  if (strcmp(elementId, "p10_log_refresh_btn") == 0) {
    webReadLogBuffer();
  }

  // ------------------------------------------------------------------
  // Control Example callback
  // ------------------------------------------------------------------

  // OPMODE-Example
  if (strcmp(elementId, "p02_opmode_man") == 0) {
    ESP_LOGI(TAG, "OPMODE: Manual");
    example.opmode = 0;
  } else if (strcmp(elementId, "p02_opmode_auto") == 0) {
    ESP_LOGI(TAG, "OPMODE: Auto");
    example.opmode = 1;
  }

  // Dropdown-Example
  if (strcmp(elementId, "p02_option") == 0) {
    ESP_LOGI(TAG, "Dropdown-Value: %s", value);
  }

  // Input-Example
  if (strcmp(elementId, "p02_number_1") == 0) {
    ESP_LOGI(TAG, "Input-Value: %s", value);
    example.setTemp = atoi(value);
  }

  // Range-Example
  if (strcmp(elementId, "p02_range_1_value") == 0) {
    ESP_LOGI(TAG, "Range-Value: %s", value);
    example.actTemp = atoi(value);
  }
}