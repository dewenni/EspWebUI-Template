#include <WiFi.h>
#include <basics.h>
#include <language.h>
#include <message.h>
#include <mqtt.h>
#include <mqttDiscovery.h>
#include <queue>

#define MAX_MQTT_CMD 20

/* D E C L A R A T I O N S ****************************************************/
#define PAYLOAD_LEN 512
struct s_MqttMessage {
  char topic[512];
  char payload[PAYLOAD_LEN];
  int len;
};

std::queue<s_MqttMessage> mqttCmdQueue;
static void processMqttMessage();
static AsyncMqttClient mqtt_client;
static bool bootUpMsgDone, setupDone = false;
static const char *TAG = "MQTT"; // LOG TAG
static char lastError[64] = "---";
static int mqtt_retry = 0;
static muTimer mqttReconnectTimer;

/**
 * *******************************************************************
 * @brief   add message to mqtt command buffer
 * @param   topic, payload, len
 * @return  none
 * *******************************************************************/
void addMqttCmd(const char *topic, const char *payload, int len) {
  if (mqttCmdQueue.size() < MAX_MQTT_CMD) {
    s_MqttMessage message;
    strncpy(message.topic, topic, sizeof(message.topic) - 1);
    message.topic[sizeof(message.topic) - 1] = '\0';

    strncpy(message.payload, payload, sizeof(message.payload) - 1);
    message.payload[sizeof(message.payload) - 1] = '\0';

    message.len = len;

    mqttCmdQueue.push(message);
    ESP_LOGD(TAG, "add msg to buffer: %s, %s", topic, payload);
  } else {
    ESP_LOGE(TAG, "too many commands within too short time");
  }
}

/**
 * *******************************************************************
 * @brief   mqtt publish wrapper
 * @param   topic, payload, retained
 * @return  none
 * *******************************************************************/
void mqttPublish(const char *topic, const char *payload, boolean retained) { mqtt_client.publish(topic, 0, retained, payload); }

/**
 * *******************************************************************
 * @brief   helper function to add subject to mqtt topic
 * @param   none
 * @return  none
 * *******************************************************************/
const char *addTopic(const char *suffix) {
  static char newTopic[256];
  snprintf(newTopic, sizeof(newTopic), "%s%s", config.mqtt.topic, suffix);
  return newTopic;
}

/**
 * *******************************************************************
 * @brief   helper function to add subject to mqtt topic
 * @param   none
 * @return  none
 * *******************************************************************/
const char *addCfgCmdTopic(const char *suffix) {
  static char newTopic[256];
  snprintf(newTopic, sizeof(newTopic), "%s/setvalue/%s", config.mqtt.topic, suffix);
  return newTopic;
}

/**
 * *******************************************************************
 * @brief   MQTT callback function for incoming message
 * @param   topic, payload
 * @return  none
 * *******************************************************************/
void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {

  s_MqttMessage msgCpy;

  msgCpy.len = len;

  if (topic == NULL) {
    msgCpy.topic[0] = '\0';
  } else {
    strncpy(msgCpy.topic, topic, sizeof(msgCpy.topic) - 1);
    msgCpy.topic[sizeof(msgCpy.topic) - 1] = '\0';
  }
  if (payload == NULL) {
    msgCpy.payload[0] = '\0';
  } else if (len > 0 && len < PAYLOAD_LEN) {
    memcpy(msgCpy.payload, payload, len);
    msgCpy.payload[len] = '\0';
  }

  addMqttCmd(msgCpy.topic, msgCpy.payload, msgCpy.len);

  ESP_LOGI(TAG, "msg received | topic: %s | payload: %s", msgCpy.topic, msgCpy.payload);
}

/**
 * *******************************************************************
 * @brief   callback function if MQTT gets connected
 * @param   none
 * @return  none
 * *******************************************************************/
void onMqttConnect(bool sessionPresent) {
  mqtt_retry = 0;
  ESP_LOGI(TAG, "MQTT connected");
  // Once connected, publish an announcement...
  sendWiFiInfo();
  // ... and resubscribe
  mqtt_client.subscribe(addTopic("/cmd/#"), 0);
  mqtt_client.subscribe(addTopic("/setvalue/#"), 0);
  mqtt_client.subscribe("homeassistant/status", 0);
}

/**
 * *******************************************************************
 * @brief   callback function if MQTT gets disconnected
 * @param   none
 * @return  none
 * *******************************************************************/
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {

  switch (reason) {
  case AsyncMqttClientDisconnectReason::TCP_DISCONNECTED:
    snprintf(lastError, sizeof(lastError), "TCP DISCONNECTED");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION:
    snprintf(lastError, sizeof(lastError), "MQTT UNACCEPTABLE PROTOCOL VERSION");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED:
    snprintf(lastError, sizeof(lastError), "MQTT IDENTIFIER REJECTED");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE:
    snprintf(lastError, sizeof(lastError), "MQTT SERVER UNAVAILABLE");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS:
    snprintf(lastError, sizeof(lastError), "MQTT MALFORMED CREDENTIALS");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED:
    snprintf(lastError, sizeof(lastError), "MQTT NOT AUTHORIZED");
    break;
  case AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT:
    snprintf(lastError, sizeof(lastError), "TLS BAD FINGERPRINT");
    break;
  default:
    snprintf(lastError, sizeof(lastError), "UNKNOWN ERROR");
    break;
  }
}

/**
 * *******************************************************************
 * @brief   is MQTT connected
 * @param   none
 * @return  none
 * *******************************************************************/
bool mqttIsConnected() { return mqtt_client.connected(); }

const char *mqttGetLastError() { return lastError; }

/**
 * *******************************************************************
 * @brief   Basic MQTT setup
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttSetup() {

  mqtt_client.onConnect(onMqttConnect);
  mqtt_client.onDisconnect(onMqttDisconnect);
  mqtt_client.onMessage(onMqttMessage);
  mqtt_client.setServer(config.mqtt.server, config.mqtt.port);
  mqtt_client.setClientId(config.wifi.hostname);
  mqtt_client.setCredentials(config.mqtt.user, config.mqtt.password);
  mqtt_client.setWill(addTopic("/status"), 0, true, "offline");
  mqtt_client.setKeepAlive(10);
  mqtt_client.connected();

  ESP_LOGI(TAG, "MQTT setup done!");
}

/**
 * *******************************************************************
 * @brief   MQTT cyclic function
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttCyclic() {

  // process incoming messages
  if (!mqttCmdQueue.empty()) {
    processMqttMessage();
  }

  // call setup when connection is established
  if (config.mqtt.enable && !setupMode && !setupDone && (eth.connected || wifi.connected)) {
    mqttSetup();
    setupDone = true;
  }

  // automatic reconnect to mqtt broker if connection is lost - try 5 times, then reboot
  if (!mqtt_client.connected() && (wifi.connected || eth.connected)) {
    if (mqtt_retry == 0) {
      mqtt_retry++;
      mqtt_client.connect();
      ESP_LOGI(TAG, "MQTT - connection attempt: 1/5");
    } else if (mqttReconnectTimer.delayOnTrigger(true, MQTT_RECONNECT)) {
      mqttReconnectTimer.delayReset();
      if (mqtt_retry < 5) {
        mqtt_retry++;
        mqtt_client.connect();
        ESP_LOGI(TAG, "MQTT - connection attempt: %i/5", mqtt_retry);
      } else {
        ESP_LOGI(TAG, "MQTT connection not possible, esp rebooting...");
        EspSysUtil::RestartReason::saveLocal("no mqtt connection");
        yield();
        delay(1000);
        yield();
        ESP.restart();
      }
    }
  }

  // send bootup messages after restart and established mqtt connection
  if (!bootUpMsgDone && mqtt_client.connected()) {
    bootUpMsgDone = true;
    ESP_LOGI(TAG, "ESP restarted (%s)", EspSysUtil::RestartReason::get());

    if (config.mqtt.ha_enable) {
      mqttDiscoverySetup(false);
    }
  }
}

/**
 * *******************************************************************
 * @brief   MQTT callback function for incoming message
 * @param   topic, payload
 * @return  none
 * *******************************************************************/
void processMqttMessage() {

  s_MqttMessage msgCpy = mqttCmdQueue.front();

  ESP_LOGD(TAG, "process msg from buffer: %s, %s", msgCpy.topic, msgCpy.payload);

  // restart ESP command
  if (strcasecmp(msgCpy.topic, addTopic("/cmd/restart")) == 0) {
    EspSysUtil::RestartReason::saveLocal("mqtt command");
    yield();
    delay(1000);
    yield();
    ESP.restart();

    // reconfigure
  } else if (strcasecmp(msgCpy.topic, addTopic("/cmd/reconfigure")) == 0) {
    mqttDiscoverySetup(true);
    yield();
    delay(1000);
    yield();
    mqttDiscoverySetup(false);

    // homeassistant/status
  } else if (strcmp(msgCpy.topic, "homeassistant/status") == 0) {
    if (config.mqtt.ha_enable && strcmp(msgCpy.payload, "online") == 0) {
      mqttDiscoverySetup(false); // send actual discovery configuration
    }

  } else {
    mqttPublish(addTopic("/message"), "unknown topic", false);
    ESP_LOGI(TAG, "unknown topic received");
  }

  mqttCmdQueue.pop(); // next entry in Queue
}
