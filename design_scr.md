   
1. # แยก code ด้านล่างเอาไปเฉพาะใส้ใน ที่อยู่ใน function inverterSetting() ไปไว้ใน สร้างฟังชันในไฟล์ inv_control.cpp ในส่วน http post  ยังคงไว้เหมือนเดิมและยังคงมี return อยู่เหมือนเดิมใน http_server.cpp เพื่อให้เป็นหมวดหมู่ 
    # แยก code ให้เป็นหมวดหมู่ และใส่คอมเม้นของแต่ล่ะหัวข้อให้ง่ายต่อการแก้ไข

2. ปรับปรุง inv_control.cpp
    # เพิ่มตัวแปรและเก็บค่าให้คล้ายกับ energy_kWh += ((inv.data.gridPower * dt) / 3600000.0); สำหรับ 3 ค่าดังนี้
    - ค่า energy รายเดือน 
    - ค่า solar energy รายวัน พลังงานไฟฟ้าคิดจาก inv.data.pvPower
    - ค่า solar energy รายเดือน พลังงานไฟฟ้าคิดจาก inv.data.pvPower

    # โดยค่า reset รายเดือนให้คิดจากค่ากลางระหว่าง gridCutOff gridStart 
    # และเพิ่มค่า energy ทั้งหมดเข้ากับฟังก์ชัน loadEnergyFromFile(),saveEnergyToFile(), clearEnergyFile() ใน storage_manager.cpp

3. # ปรับปรุง นำค่า energy ทั้งหมดเพิ่มเข้าไปยัง websocket_handler.cpp เพื่อส่งค่าไปยังหน้าเว็บ
   # ปรับปรุง นำค่า energy ทั้งหมดเพิ่มเข้าไปยัง ha_integration.cpp เพื่อส่งค่าไปยัง homeassistant
    
        energy_kWh 
        energy_m_kWh
        solar_kWh 
        solar_m_kWh 

4. # ปรับปรุง websocket_handler.cpp
    ลดการใช้คำสั่ง String() เพื่อประสิทธิภาพในการทำงาน ของ ESP32

5. # ให้ประกาศตัวแปรสำหรับโหลดค่า Expense setting ไว้ใน inv_control.cpp 
    # โหลดค่า expense setting ที่ส่งมาจาก setting.html จาก /expense.json ใน FS ด้วยฟังชัน loadAllSettings()/storage_manager.cpp

    # เพิ่มฟังชัน คำนวนค่าไฟ ที่ดึงจากการไฟฟ้า,และค่าที่ได้จากโซล่าเซลล์
    # นำค่าไฟ ส่งไปยัง websocket_handler.cpp เพื่อแสดงค่าไปยังหน้าเว็บ
    # นำค่าไฟ ส่งไปยัง ha_integration.cpp เพื่อแสดงค่าไปยัง homeassistant
    # แยก code ให้เป็นหมวดหมู่ และใส่คอมเม้นของแต่ล่ะหัวข้อให้ง่ายต่อการแก้ไข

6. # แยกฟังชันการโหลด expense ออกจาก bool loadAllSettings() มาสร้างฟังชันใหม่
   # แยก code ให้เป็นหมวดหมู่ และใส่คอมเม้นของแต่ล่ะหัวข้อให้ง่ายต่อการแก้ไข

7. # ในส่วนของ class="metrics-grid" ตัด SOLAR PRODUCTION,BATTERY STORAGE,HOME LOAD ออก
 # แยก code ให้เป็นหมวดหมู่ และใส่คอมเม้นของแต่ล่ะหัวข้อให้ง่ายต่อการแก้ไข
 # ในส่วนของ class="load-stats" HEADROOM ใน dashboard ให้เปลี่ยนเป็น CORETEMP ที่มาจาก Temperature ตัด MAX_CAP และ PEAK_DAY ออก

8. # เชื่อมโยง Energy benefits ถ้ายังไม่มีค่าไหนให้สร้างฟังชันคำนวนไว้ใน inv_control.cpp แล้วส่ง websocket ไปยัง dashboard
   # แยก code ให้เป็นหมวดหมู่ และใส่คอมเม้นของแต่ล่ะหัวข้อให้ง่ายต่อการแก้ไข

9. # แก้ไขชื่อทั้งใน card ทั้งหมด ให้สอดคล้องแลัวดูดี
      <!-- 3. Load and output performance -->
      <section class="load-output-section">
   # แยก code ให้เป็นหมวดหมู่ และใส่คอมเม้นของแต่ล่ะหัวข้อให้ง่ายต่อการแก้ไข

10. # แก้ไขให้ class="flow-path solar-path" ให้มุมเป็นเส้นโค้ง
    <section class="panel flow-panel" aria-labelledby="flow-title">

11. # เก็บค่า history เป็น json array โดยมีค่า พลังงานไฟฟ้า วัน,เดือน,ปี พลังงานโซล่า วัน,เดือน,ปี โดยทำต่อจาก energy_tracker.cpp
    # array /วันเก็บ 24 ค่าหรือทุกชั่วโมง/เดือนเก็บ 30 ค่าหรือทุกวัน/ปีเก็บ 12 ค่าหรือทุกเดือน// ค่าล่าสุดค่อยๆเก็บไปเรื่องทุก 15 นาที เพื่อให้การฟค่อยแสดงขี้น ไม่รอแสดงครบ เดือน หรือปี ทีเดียว
    # ค่าพลังงานดึงมาจาก energy_kWh energy_m_kWh solar_kWh solar_m_kWh 
    # ค่าพลังงานเก็บไว้ใน energy_history
    
12. # แก้ไข ha_integration.cpp ให้ดึงค่าจาก DEVICE_NAME มาใส่ใน entity โดยแสดงใน homeasiatan แบบเช่น sensor."DEVICE_NAME"_output_current
    # แยก code ให้เป็นหมวดหมู่ และใส่คอมเม้นของแต่ล่ะหัวข้อให้ง่ายต่อการแก้ไข

13. # 