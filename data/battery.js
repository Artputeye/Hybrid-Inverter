document.addEventListener("DOMContentLoaded", () => {
  // --------------------------------------------------------------------------
  // 1. Battery ranges and units
  // --------------------------------------------------------------------------
  const ranges = {
    BulkChargingVoltage: { "24": [25.0, 31.5], "48": [48.0, 61.0] },
    FloatingChargingVoltage: { "24": [25.0, 31.5], "48": [48.0, 61.0] },
    LowBatteryCutoffVoltage: { "24": [20.0, 24.0], "48": [40.0, 48.0] },
    BatteryEqualizationVoltage: { "24": [25.0, 31.5], "48": [48.0, 61.0] },
    BatteryEqualizationTime: [5, 900],
    BatteryEqualizationTimeout: [5, 900],
    BatteryEqualizationInterval: [1, 90]
  };

  const units = {
    BulkChargingVoltage: "V", FloatingChargingVoltage: "V",
    LowBatteryCutoffVoltage: "V", BatteryEqualizationVoltage: "V",
    BatteryEqualizationTime: "min", BatteryEqualizationTimeout: "min",
    BatteryEqualizationInterval: "day"
  };

  const toggle = document.getElementById("battypeToggle");
  const battLabel = document.getElementById("battType");

  // --------------------------------------------------------------------------
  // 2. Battery mode toggles
  // --------------------------------------------------------------------------
  function setRangeAndLabel(inputId, labelId, voltType) {
    const input = document.getElementById(inputId);
    const label = document.getElementById(labelId);
    if (!input || !ranges[inputId]) return;

    const [min, max] = Array.isArray(ranges[inputId]) ? ranges[inputId] : ranges[inputId][voltType];
    input.min = min;
    input.max = max;
    if (label) label.textContent = `${min} - ${max} ${units[inputId] || ""}`;
  }

  function toggleSelect(checkbox) {
    const voltType = checkbox?.checked ? "48" : "24";
    if (battLabel) battLabel.textContent = voltType + "V";

    ["BulkChargingVoltage", "FloatingChargingVoltage", "LowBatteryCutoffVoltage", "BatteryEqualizationVoltage"]
      .forEach(id => setRangeAndLabel(id, id.replace("Voltage", "_Voltage"), voltType));

    ["BatteryEqualizationTime", "BatteryEqualizationTimeout", "BatteryEqualizationInterval"]
      .forEach(id => setRangeAndLabel(id, id.replace("BatteryEqualization", "BatteryEqualization_"), voltType));
  }

  // --------------------------------------------------------------------------
  // 3. Battery selection settings
  // --------------------------------------------------------------------------
  document.querySelectorAll('.toggle-row input[type="checkbox"]').forEach(cb => {
    const settingName = cb.getAttribute("data-setting") || cb.id.replace(/\s+/g, "");
    cb.addEventListener("change", function() {
      const val = this.checked ? 1 : 0;
      console.log(`🔘 [BATT TOGGLE] ${settingName} = ${val}`);
      sendToServer(settingName, val);
      if (this.id === "battypeToggle") toggleSelect(this);
    });
  });

  // --------------------------------------------------------------------------
  // 4. Battery voltage settings
  // --------------------------------------------------------------------------
  window.sendSetting = function(button) {
    const container = button.closest(".form-row, .card-main");
    if (!container) return;

    const inputElement = container.querySelector("select, input");
    if (!inputElement) return;

    const settingName = inputElement.id;
    let val = parseFloat(inputElement.value);

    if (inputElement.type === "number") {
      const min = parseFloat(inputElement.min);
      const max = parseFloat(inputElement.max);
      const errorMsg = document.getElementById(settingName + "_error");

      if (isNaN(val) || val < min || val > max) {
        console.warn(`⚠️ [VALIDATION FAILED] ${settingName} value ${val} out of range [${min} - ${max}]`);
        inputElement.style.border = "2px solid red";
        if (errorMsg) {
          errorMsg.textContent = `กรุณาใส่ค่าในช่วง ${min} - ${max} ${units[settingName] || ""}`;
          errorMsg.style.display = "block";
        }
        return;
      } else {
        inputElement.style.border = "";
        if (errorMsg) errorMsg.style.display = "none";
      }

      // ถ้าเป็นแรงดัน คูณ 10 เพื่อแปลงเป็น Integer
      if (["BulkChargingVoltage", "FloatingChargingVoltage", "LowBatteryCutoffVoltage", "BatteryEqualizationVoltage"].includes(settingName)) {
        val = Math.round(val * 10);
      }
    }

    const finalVal = parseInt(val, 10);
    console.log(`🔘 [BATT SETTING] Sending ${settingName} = ${finalVal}`);
    sendToServer(settingName, finalVal);
  };

  // --------------------------------------------------------------------------
  // 5. Restore saved settings
  // --------------------------------------------------------------------------
  fetch('/battery.json')
    .then(res => res.json())
    .then(data => {
      console.log("📥 [RESTORE] Loaded battery settings from /battery.json:", data);
      Object.keys(data).forEach(key => {
        const el = document.getElementById(key);
        if (!el) return;
        if (el.tagName === "SELECT") el.value = data[key];
        else if (el.type === "checkbox") el.checked = data[key] === "1" || data[key] === 1;
        else if (el.type === "number") el.value = data[key];
      });
      toggleSelect(toggle);
    })
    .catch(err => console.error("❌ [RESTORE ERROR] Fetching battery settings:", err));

  // --------------------------------------------------------------------------
  // 6. Server communication and state sync
  // --------------------------------------------------------------------------
  function sendToServer(settingName, value) {
    const payload = { setting: settingName, value: Number(value) };

    // 🔍 LOG: รายละเอียดการส่งของ Battery
    console.log(`📤 [POST REQUEST] Destination: /invsetting`);
    console.log(`📦 [PAYLOAD DATA]:`, payload);
    console.log(`📄 [JSON STRING]:`, JSON.stringify(payload));
    console.log(`💡 [DATA TYPE OF VALUE]:`, typeof payload.value);

    fetch('/invsetting', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    })
    .then(res => res.text())
    .then((resText) => {
      console.log(`✅ [RESPONSE] Status OK | Body:`, resText);
      submitAllSettings();
    })
    .catch(err => console.error("❌ [FETCH ERROR]:", err));
  }

  function submitAllSettings() {
    const data = {};
    document.querySelectorAll('input[type="checkbox"]').forEach(i => i.id && (data[i.id] = i.checked ? "1" : "0"));
    document.querySelectorAll('input[type="number"]').forEach(i => i.id && (data[i.id] = i.value));
    document.querySelectorAll('select').forEach(i => i.id && (data[i.id] = i.value));

    console.log("💾 [SYNCING] Saving full battery state to /battery.json...");
    fetch('/battery.json', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(data)
    });
  }
});