#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H
#include "config.h"

void setupRouteAPIs();
void initWebRoutes();
String getContentType(String filename);
void staticRoot();
void notfoundRoot();
void inverterSetting();
void terminalSetting();
void routeSettingAPI(String filename, String mode) ;
void notfoundRoot();

#endif