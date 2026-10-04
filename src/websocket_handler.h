#ifndef WEBSOCKET_HANDLER_H
#define WEBSOCKET_HANDLER_H

#include "config.h"
#include <freertos/semphr.h>

extern SemaphoreHandle_t telemetryMutex;


String wsAllDataBase64();
void wsClear();
void notifyClients(const String &msg);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void ws_init();
void wsJsonSerial(const String &msg);
void wsJsonInverter(const String &msg);
void wsJsonControll(const String &msg);
void wsProcess();

#endif