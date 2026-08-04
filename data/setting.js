document.addEventListener("DOMContentLoaded", () => {
  // 1. ดึงค่าคอนฟิกเริ่มต้นจาก Server
  fetch('/setting.json')
    .then(res => res.json())
    .then(data => {
      console.log("📥 [RESTORE] Loaded settings from /setting.json:", data);
      
      // Restore Select, Checkbox, Number
      Object.keys(data).forEach(id => {
        const el = document.getElementById(id);
        if (!el) return;
        if (el.tagName === "SELECT") el.value = data[id];
        else if (el.type === "checkbox") el.checked = data[id] === "1" || data[id] === 1;
        else if (el.type === "number") el.value = data[id];
      });

      // Update UI State สำหรับ Grid Tie Auto
      const gridTieAuto = document.getElementById("Grid Tie Auto");
      if (gridTieAuto) updateGridTieUI(gridTieAuto.checked);
    })
    .catch(err => console.error("❌ [RESTORE ERROR] Fetching settings:", err));
});

// Helper สำหรับส่ง JSON ไปยัง Endpoints ต่างๆ พร้อม Console Log
async function postJSON(url, payload) {
  // 🔍 LOG: แสดงข้อมูลที่จะส่งออกไปหา Server
  console.log(`📤 [POST REQUEST] Destination: ${url}`);
  console.log("📦 [PAYLOAD DATA]:", payload);
  console.log("📄 [JSON STRING]:", JSON.stringify(payload));

  try {
    const res = await fetch(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    const responseText = await res.text();
    console.log(`✅ [RESPONSE] Status: ${res.status} | Body:`, responseText);
    return responseText;
  } catch (err) {
    console.error(`❌ [FETCH ERROR] (${url}):`, err);
  }
}

// ฟังก์ชันหลักสำหรับส่ง Setting ไปหา ESP32
function sendSetting(element) {
  const container = element.closest('.setting-dropdown, .card-main');
  if (!container) return;

  const settingType = container.getAttribute('data-setting') || element.id;
  const inputEl = container.querySelector('select, input');
  if (!inputEl) return;

  // แปลงเป็น Number เสมอ
  const numericValue = parseInt(inputEl.value, 10);
  const finalValue = isNaN(numericValue) ? 0 : numericValue;

  console.log(`🔘 [ACTION] Output Priority / Single Setting Change triggered`);

  postJSON('/invsetting', {
    setting: settingType,
    value: finalValue
  }).then(() => {
    submitAllSettings();
  });
}

// ฟังก์ชันสำหรับ Toggle Checkbox
function toggleSetting(checkbox, settingName) {
  const status = checkbox.checked ? 1 : 0;

  console.log(`🔘 [ACTION] Toggle Switch: ${settingName} -> ${status}`);

  if (settingName === "Grid Tie Auto") {
    updateGridTieUI(checkbox.checked);
  }

  postJSON('/invsetting', {
    setting: settingName,
    value: status
  }).then(() => submitAllSettings());
}

// ฟังก์ชันส่ง Grid Cutoff & Start พร้อมกัน
function GridCutToServer() {
  const gridCutOff = parseInt(document.getElementById("gridCutOff")?.value, 10) || 0;
  const gridStart = parseInt(document.getElementById("gridStart")?.value, 10) || 0;

  console.log(`🔘 [ACTION] Grid Cutoff / Start Setting triggered`);

  postJSON('/invsetting', {
    gridCutOff: gridCutOff,
    gridStart: gridStart
  }).then(() => submitAllSettings());
}

function updateGridTieUI(isAuto) {
  const gridTieOp = document.getElementById("Grid Tie Operation");
  if (gridTieOp) {
    gridTieOp.disabled = isAuto;
    gridTieOp.parentElement.classList.toggle("disabled", isAuto);
  }
}

// บันทึกสถานะรวมลง /setting.json
function submitAllSettings() {
  const data = {};
  document.querySelectorAll('input[type="checkbox"]').forEach(el => el.id && (data[el.id] = el.checked ? "1" : "0"));
  document.querySelectorAll('input[type="number"]').forEach(el => el.id && (data[el.id] = el.value));
  document.querySelectorAll('select').forEach(el => el.id && (data[el.id] = el.value));

  console.log("💾 [SYNCING] Saving full state to /setting.json...");
  postJSON('/setting.json', data);
}