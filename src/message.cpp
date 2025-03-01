#include <basics.h>
#include <message.h>
#include <telnet.h>

/* D E C L A R A T I O N S ****************************************************/
#define MSG_BUF_SIZE 1024 // buffer size for messaging

static const char *TAG = "MSG"; // LOG TAG
s_logdata logData;
esp_log_level_t logLevel = ESP_LOG_INFO;

static muTimer mainTimer = muTimer(); // timer for cyclic info


/**
 * *******************************************************************
 * @brief   set Log Level for ESP_LOG messages
 * @param   level
 * @return  none
 * *******************************************************************/
void setLogLevel(uint8_t level) {

  if (level == 1) {
    logLevel = ESP_LOG_ERROR;
    ESP_LOGI(TAG, "LogLevel: ESP_LOG_ERROR");
  } else if (level == 2) {
    logLevel = ESP_LOG_WARN;
    ESP_LOGI(TAG, "LogLevel: ESP_LOG_WARN");
  } else if (level == 3) {
    logLevel = ESP_LOG_INFO;
    ESP_LOGI(TAG, "LogLevel: ESP_LOG_INFO");
  } else {
    logLevel = ESP_LOG_DEBUG;
    ESP_LOGI(TAG, "LogLevel: ESP_LOG_DEBUG");
  }
  esp_log_level_set("*", logLevel);
  esp_log_level_set("ARDUINO", ESP_LOG_WARN);
}

/**
 * *******************************************************************
 * @brief   custom callback function for ESP_LOG messages
 * @param   format, args
 * @return  vprintf(format, args)
 * *******************************************************************/
int custom_vprintf(const char *format, va_list args) {
  // create a copy of va_list
  va_list args_copy;
  va_copy(args_copy, args);

  char raw_message[MAX_LOG_ENTRY];
  char cleaned_message[MAX_LOG_ENTRY];

  // copy message to raw_message
  vsnprintf(raw_message, sizeof(raw_message), format, args_copy);

  // remove timestamp from message
  const char *start = strchr(raw_message, '(');
  const char *end = strchr(raw_message, ')');
  if (start != NULL && end != NULL && end > start) {
    // copy the part before '('
    size_t prefix_len = start - raw_message;
    strncpy(cleaned_message, raw_message, prefix_len);
    cleaned_message[prefix_len] = '\0';

    // copy the part after ')'
    strncat(cleaned_message, end + 1, sizeof(cleaned_message) - prefix_len - 1);
  } else {
    // no timestamp found
    strncpy(cleaned_message, raw_message, sizeof(cleaned_message));
  }

  // add to log buffer
  addLogBuffer(cleaned_message);

  // forward to telnet stream
  if (telnetIF.serialStream) {
    telnet.printf("%s", cleaned_message);
    telnetShell();
  }

  // release copy of va_list
  va_end(args_copy);

  return vprintf(format, args);
}


/**
 * *******************************************************************
 * @brief   clear Logbuffer
 * @param   none
 * @return  none
 * *******************************************************************/
void clearLogBuffer() {
  logData.lastLine = 0;
  for (int i = 0; i < MAX_LOG_LINES; i++) {
    memset(logData.buffer[i], 0, sizeof(logData.buffer[i]));
  }
}

/**
 * *******************************************************************
 * @brief   add new entry to LogBuffer
 * @param   none
 * @return  none
 * *******************************************************************/
void addLogBuffer(const char *message) {
  if (strlen(message) != 0) {
    snprintf(logData.buffer[logData.lastLine], sizeof(logData.buffer[logData.lastLine]), "[%s]  %s", EspStrUtil::getDateTimeString(), message);
    logData.lastLine = (logData.lastLine + 1) % MAX_LOG_LINES; // update the lastLine index in a circular manner
  }
}

/**
 * *******************************************************************
 * @brief   Setup for Telegram bot
 * @param   none
 * @return  none
 * *******************************************************************/
void messageSetup() {

  // Enable serial port
  Serial.begin(115200);
  delay(100);

  setLogLevel(ESP_LOG_INFO);            // inital log level - will be changed after config setup
  esp_log_set_vprintf(&custom_vprintf); // set custom vprintf callback function

  clearLogBuffer();
}

/**
 * *******************************************************************
 * @brief   Message Cyclic Loop
 * @param   none
 * @return  none
 * *******************************************************************/
void messageCyclic() {

  // send cyclic infos
  if (mainTimer.cycleTrigger(10000) && !setupMode && mqttIsConnected()) {

    sendSysInfo();

    if (config.wifi.enable) {
      sendWiFiInfo();
    }

    if (config.eth.enable) {
      sendETHInfo();
    }
  }
}
