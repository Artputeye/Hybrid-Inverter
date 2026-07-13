#include "http_server.h"
const char *PARAM_MESSAGE PROGMEM = "plain";


void initWebRoutes()
{
  //setupOTAUpload();
  staticRoot();
  JsonSetting();
  cmdSetting();
  getSetting();
  saveSetting();
  getbatSetting();
  savebatSetting();
  getNetwork();
  saveNetwork();
  notfoundRoot();
}

String getContentType(String filename)
{
  if (filename.endsWith(".html"))
    return "text/html";
  if (filename.endsWith(".css"))
    return "text/css";
  if (filename.endsWith(".js"))
    return "application/javascript";
  if (filename.endsWith(".png"))
    return "image/png";
  if (filename.endsWith(".jpg"))
    return "image/jpeg";
  if (filename.endsWith(".ico"))
    return "image/x-icon";
  if (filename.endsWith(".svg"))
    return "image/svg+xml";
  if (filename.endsWith(".json"))
    return "application/json";
  return "text/plain";
}

////////////////////////////////////// STATIC ROOT /////////////////////////////////////////
void staticRoot()
{
  // เสิร์ฟทุกไฟล์ใน LittleFS เช่น index.html, info.css, app.js
  server.serveStatic("/", LittleFS, "/")
      .setDefaultFile("index.html")
      .setCacheControl("max-age=86400"); // cache 1 วัน (ลดการโหลดซ้ำ)

  // สำหรับ path พิเศษ เช่น /set, /ota → map ไปยัง .html โดยตรง
  const char *pages[] = {"/set", "/ota", "/batt", "/device", "/filelist", "/info", "/monitor", "/network"};
  for (auto &p : pages)
  {
    server.on(p, HTTP_GET, [p](AsyncWebServerRequest *request)
              {
      String filepath = String(p) + ".html";
      if (LittleFS.exists(filepath))
      {
        //Serial.println("Request to '" + String(p) + "': Serving " + filepath);
        request->send(LittleFS, filepath, "text/html");
      }
      else
      {
        request->send(404, "text/plain", "Page not found");
      } });
  }
}

///////////////////////////////////// PARAMETER SETTING ////////////////////////////////////
void JsonSetting() // Control Route
{
  server.on("/setting", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error)
      {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }
      String setting   = doc["setting"].is<String>()     ? doc["setting"].as<String>()     : "";
      uint16_t value   = doc["value"].is<uint16_t>()     ? doc["value"].as<uint16_t>()     : 0;
      gridCutOff   = doc["gridCutOff"].is<int>()     ? doc["gridCutOff"].as<int>()     : -1;
      gridStart    = doc["gridStart"].is<int>()      ? doc["gridStart"].as<int>()      : -1;

      if (setting != "")
      {
        inv.valueToinv(setting, value);
        Serial.printf("📥 Setting: %s = %d\n", setting.c_str(), value);
      }

      if (setting == "Grid Tie Auto"&&value==1 )
      {
        inv.gridOpr = true;
        Serial.printf("📥 Grid Tie Auto: %s = %d\n", setting.c_str(), value);
      }

      if (setting == "Grid Tie Auto"&&value==0 )
      {
        inv.gridOpr = false;
        Serial.printf("📥 Grid Tie Auto: %s = %d\n", setting.c_str(), value);
      }

      String response = "{\"status\":\"ok\"";
      if (setting != "") response += ",\"setting\":\"" + setting + "\",\"value\":" + String(value);
      if (gridCutOff != -1) response += ",\"gridCutOff\":" + String(gridCutOff);
      if (gridStart  != -1) response += ",\"gridStart\":"  + String(gridStart);
      response += "}";
      request->send(200, "application/json", response); });
}

///////////////////////////////////// COMMAND SETTING //////////////////////////////////////
void cmdSetting() // control route
{
  server.on("/cmd", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    String message;
    if (request->hasParam(PARAM_MESSAGE, true)) {
      message = request->getParam(PARAM_MESSAGE, true)->value();
    } else {
      message = "No message sent";
      Serial.println("No message sent");
    }
    inv.cmd_inv(message);
    Serial.println("POST client: " + message);
    request->send(200, "text/plain", "POST: " + message); });
}

///////////////////////////////////// GENERAL SETTING //////////////////////////////////////

void getSetting() // API: ดึง JSON จาก littleFS แล้วส่ง Setting.json ไปยัง Client
{
  server.on("/getsetting", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (!LittleFS.exists("/setting.json")) {
      request->send(404, "application/json", "{\"error\":\"setting.json not found\"}");
      return;
    }

    File file = LittleFS.open("/setting.json", "r");
    if (!file) {
      request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
      return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
      request->send(500, "application/json", "{\"error\":\"Failed to parse JSON\"}");
      return;
    }

    inv.gridOpr = atoi(doc["Grid Tie Auto"] | "0"); // grid tie auto status
    Serial.println("Grid Operate from \"setting.json : " + String(inv.gridOpr));

    String jsonResponse;
    serializeJson(doc, jsonResponse);
    Serial.println("/get setting : " + jsonResponse);

    request->send(200, "application/json", jsonResponse); });
}

///////////////////////////////////// SAVE SETTING /////////////////////////////////////////
void saveSetting() // API: รับ JSON จาก Client แล้วบันทึกไฟล์ Setting.json ไปยัง littleFS
{
  server.on("/savesetting", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);
    if (error) {
      Serial.println("JSON parse failed!");
      request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    // Debug: แสดงค่าที่ได้รับ
    for (JsonPair kv : doc.as<JsonObject>()) {
      String key = kv.key().c_str();
      String value = kv.value().as<String>();
      //Serial.printf("Received setting: %s = %s\n", key.c_str(), value.c_str());
    }
    // เปิดไฟล์เพื่อเขียนทับ
    File file = LittleFS.open("/setting.json", "w");
    if (!file) {
      request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
      return;
    }
    // เขียน JSON ลงไฟล์
    if (serializeJson(doc, file) == 0) {
      request->send(500, "application/json", "{\"error\":\"Failed to write JSON\"}");
      file.close();
      return;
    }
    file.close();
    request->send(200, "application/json", "{\"status\":\"ok\"}"); });

  // ESP.restart();
}

///////////////////////////////////// BATTERY SETTING //////////////////////////////////////
void getbatSetting() // API: ดึง JSON จาก littleFS แล้วส่ง battery.json ไปยัง Client
{
  server.on("/getbattsetting", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (!LittleFS.exists("/battery.json")) {
      request->send(404, "application/json", "{\"error\":\"battery.json not found\"}");
      return;
    }

    File file = LittleFS.open("/battery.json", "r");
    if (!file) {
      request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
      return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
      request->send(500, "application/json", "{\"error\":\"Failed to parse JSON\"}");
      return;
    }

    String jsonResponse;
    serializeJson(doc, jsonResponse);
    Serial.println("/get battsetting : " + jsonResponse);

    request->send(200, "application/json", jsonResponse); });
}

///////////////////////////////////// BATTERY SETTING //////////////////////////////////////
void savebatSetting() // API: รับ JSON จาก Client แล้วบันทึกไฟล์ battery.json ไปยัง littleFS
{
  server.on("/battsetting", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);
    if (error) {
      Serial.println("JSON parse failed!");
      request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    // Debug: แสดงค่าที่ได้รับ
    for (JsonPair kv : doc.as<JsonObject>()) {
      String key = kv.key().c_str();
      String value = kv.value().as<String>();
      //Serial.printf("Received setting: %s = %s\n", key.c_str(), value.c_str());
    }
    // เปิดไฟล์เพื่อเขียนทับ
    File file = LittleFS.open("/battery.json", "w");
    if (!file) {
      request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
      return;
    }
    // เขียน JSON ลงไฟล์
    if (serializeJson(doc, file) == 0) {
      request->send(500, "application/json", "{\"error\":\"Failed to write JSON\"}");
      file.close();
      return;
    }
    file.close();
    request->send(200, "application/json", "{\"status\":\"ok\"}"); });
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// NETWORK SETTING LOAD /////////////////////////////////

void getNetwork() // API: ดึง JSON จาก littleFS แล้วส่ง networkconfig.json ไปยัง Client
{
  server.on("/getnetworkconfig", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (!LittleFS.exists("/networkconfig.json")) {
      request->send(404, "application/json", "{\"error\":\"networkconfig.json not found\"}");
      return;
    }

    File file = LittleFS.open("/networkconfig.json", "r");
    if (!file) {
      request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
      return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
      request->send(500, "application/json", "{\"error\":\"Failed to parse JSON\"}");
      return;
    }

    String jsonResponse;
    serializeJson(doc, jsonResponse);
    Serial.println("/get networkconfig : " + jsonResponse);

    request->send(200, "application/json", jsonResponse); });
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// NETWORK SETTING SAVE /////////////////////////////////
void saveNetwork() // API: รับ JSON จาก Client แล้วบันทึกไฟล์ battery.json ไปยัง littleFS
{
  server.on("/networkconfig", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);
    if (error) {
      Serial.println("JSON parse failed!");
      request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    // Debug: แสดงค่าที่ได้รับ
    for (JsonPair kv : doc.as<JsonObject>()) {
      String key = kv.key().c_str();
      String value = kv.value().as<String>();
      Serial.printf("Received setting: %s = %s\n", key.c_str(), value.c_str());
    }
    // เปิดไฟล์เพื่อเขียนทับ
    File file = LittleFS.open("/networkconfig.json", "w");
    if (!file) {
      request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
      return;
    }
    // เขียน JSON ลงไฟล์
    if (serializeJson(doc, file) == 0) {
      request->send(500, "application/json", "{\"error\":\"Failed to write JSON\"}");
      file.close();
      return;
    }
    file.close();
    request->send(200, "application/json", "{\"status\":\"ok\"}"); });
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// NOT FUOND ///////////////////////////////////////////
void notfoundRoot()
{
  server.onNotFound([](AsyncWebServerRequest *request)
                    {
    String path = request->url();
    Serial.println("404 Not Found: " + path);

    // ถ้าเจอไฟล์ใน LittleFS → เสิร์ฟตาม MIME type
    if (LittleFS.exists(path))
    {
      String contentType = getContentType(path);
      request->send(LittleFS, path, contentType);
    }
    else
    {
      // ถ้าไม่เจอไฟล์เลย → คืนค่า 404
      request->send(404, "text/plain", "File Not Found\n\nPath: " + path);
    } });
}
