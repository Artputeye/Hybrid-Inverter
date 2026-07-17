#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H
#include "config.h"

void initWebRoutes();
String getContentType(String filename);
void staticRoot();
void notfoundRoot();
void JsonSetting();
void terminalSetting();
void getSetting();
void saveSetting();
void getbatSetting();
void savebatSetting();
void getNetwork();
void saveNetwork();

#endif