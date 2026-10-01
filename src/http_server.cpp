#include "http_server.h"
const char *PARAM_MESSAGE PROGMEM = "plain";

void initWebRoutes()
{
  setupRouteAPIs();
  staticRoot();
  inverterSetting();
  terminalSetting();

  notfoundRoot();
}

void setupRouteAPIs()
{
  String configFiles[] = {
      "/networkconfig.json",
      "/setting.json",
      "/expense.json",
      "/battery.json"};
  int fileCount = sizeof(configFiles) / sizeof(configFiles[0]);
  for (int i = 0; i < fileCount; i++)
  {
    routeSettingAPI(configFiles[i], "rw");
  }
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
void inverterSetting() // Control Route
{
  server.on("/invsetting", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error)
      {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }

      String response = processInverterSetting(doc);
      request->send(200, "application/json", response); });
}

///////////////////////////////////// COMMAND SETTING //////////////////////////////////////
void terminalSetting() // control route
{
  server.on("/terminalSet", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    String message;
    if (request->hasParam(PARAM_MESSAGE, true)) {
      message = request->getParam(PARAM_MESSAGE, true)->value();
    } else {
      message = "No message sent";
      Serial.println("No message sent");
    }
    inv.sendCommand(message);
    Serial.println("POST client: " + message);
    request->send(200, "text/plain", "POST: " + message); });
}

/**
 * @brief ฟังก์ชันลงทะเบียน API สำหรับจัดการไฟล์เดี่ยวตามที่กำหนด
 * @param filename ชื่อไฟล์ใน LittleFS (เช่น "/networkconfig.json")
 * @param mode โหมดการทำงาน:
 *             "r"  = สร้างเฉพาะ API สำหรับดึงข้อมูล (GET)
 *             "w"  = สร้างเฉพาะ API สำหรับบันทึกข้อมูล (POST)
 *             "rw" = สร้างทั้งคู่ (ทั้งดึงข้อมูลและบันทึกข้อมูล)
 */
void routeSettingAPI(String filename, String mode)
{
  while (filename.startsWith("/"))
  {
    filename = filename.substring(1);
  }
  String fsPath = "/" + filename;
  String urlPath = "/" + filename;

  // ==========================================
  // [จุดสำคัญ] เช็กและดักสร้างไฟล์ตั้งแต่บอร์ด Boot
  // หากไม่มีไฟล์ ให้ใช้โหมด "w+" เพื่อสร้างไฟล์ใหม่แกะกล่องทันที
  // ==========================================
  if (!LittleFS.exists(fsPath))
  {
    Serial.printf("[LittleFS] %s not found. Force creating with write permits...\n", fsPath.c_str());
    File initFile = LittleFS.open(fsPath, "w+");
    if (initFile)
    {
      initFile.print("{}");
      initFile.close();
      Serial.printf("[LittleFS] Created template structure for %s\n", fsPath.c_str());
    }
    else
    {
      Serial.printf("[LittleFS] Critical Error: Cannot create %s\n", fsPath.c_str());
    }
  }

  // ==========================================
  // ส่วนที่ 1: API สำหรับดึงข้อมูล [GET]
  // ==========================================
  if (mode == "r" || mode == "rw")
  {
    server.on(urlPath.c_str(), HTTP_GET, [fsPath](AsyncWebServerRequest *request)
              {
      // เปิดไฟล์อ่านแบบทนทาน (Safe Read)
      File file = LittleFS.open(fsPath, "r");
      if (!file) {
        request->send(200, "application/json", "{}"); // คืนค่าเซ็ตเปล่าป้องกันเว็บพัง
        return;
      }

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, file);
      file.close();

      if (error) {
        request->send(200, "application/json", "{}");
        return;
      }

      String jsonResponse;
      serializeJson(doc, jsonResponse);
      request->send(200, "application/json", jsonResponse); });
  }

  // ==========================================
  // ส่วนที่ 2: API สำหรับบันทึกข้อมูล [POST]
  // ==========================================
  if (mode == "w" || mode == "rw")
  {
    server.on(urlPath.c_str(), HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [fsPath](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
              {
        static String jsonBuffer = "";
        if (index == 0) { jsonBuffer = ""; }

        for (size_t i = 0; i < len; i++) {
          jsonBuffer += (char)data[i];
        }

        if (index + len == total) {
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, jsonBuffer);
          
          if (error) {
            Serial.println("JSON parse failed!");
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
          }

          // เปิดเขียนทับด้วยมิติความปลอดภัยสูงสุด
          File file = LittleFS.open(fsPath, "w");
          if (!file) {
            request->send(500, "application/json", "{\"error\":\"Failed to open file for writing\"}");
            return;
          }

          if (serializeJson(doc, file) == 0) {
            request->send(500, "application/json", "{\"error\":\"Failed to write JSON\"}");
            file.close();
            return;
          }
          
          file.close();
          if (fsPath == "/expense.json") {
            loadExpenseSettings();
          }
          Serial.printf("POST %s : Saved successfully\n", fsPath.c_str());
          request->send(200, "application/json", "{\"status\":\"success\"}");
        } });
  }
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
