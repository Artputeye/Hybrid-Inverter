document.addEventListener("DOMContentLoaded", () => {

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
    BulkChargingVoltage: "V",
    FloatingChargingVoltage: "V",
    LowBatteryCutoffVoltage: "V",
    BatteryEqualizationVoltage: "V",
    BatteryEqualizationTime: "min",
    BatteryEqualizationTimeout: "min",
    BatteryEqualizationInterval: "day"
  };

  const toggle = document.getElementById("battypeToggle");
  const battLabel = document.getElementById("battType");

  // ฟังก์ชันตั้ง min/max และ label
  function setRangeAndLabel(inputId, labelId, voltType) {
    const input = document.getElementById(inputId);
    const label = document.getElementById(labelId);
    if (!input || !ranges[inputId]) return;

    let min, max;
    if (Array.isArray(ranges[inputId])) {
      [min, max] = ranges[inputId];
    } else {
      [min, max] = ranges[inputId][voltType];
    }

    input.min = min;
    input.max = max;

    const unit = units[inputId] || "";
    if (label) label.textContent = `${min} - ${max} ${unit}`;
  }

  // ฟังก์ชัน toggle 24/48V
  function toggleSelect(checkbox) {
    const voltType = checkbox.checked ? "48" : "24";
    if (battLabel) battLabel.textContent = voltType + "V";

    // dependent
    setRangeAndLabel("BulkChargingVoltage", "BulkCharging_Voltage", voltType);
    setRangeAndLabel("FloatingChargingVoltage", "FloatingCharging_Voltage", voltType);
    setRangeAndLabel("LowBatteryCutoffVoltage", "LowBatteryCutoff_Voltage", voltType);
    setRangeAndLabel("BatteryEqualizationVoltage", "BatteryEqualization_Voltage", voltType);

    // fixed
    setRangeAndLabel("BatteryEqualizationTime", "BatteryEqualization_Time", voltType);
    setRangeAndLabel("BatteryEqualizationTimeout", "BatteryEqualization_Timeout", voltType);
    setRangeAndLabel("BatteryEqualizationInterval", "BatteryEqualization_Interval", voltType);
  }

  // ฟังก์ชันส่งค่าปุ่ม Set
  window.sendSetting = function(button) {
    const container = button.closest(".form-row");
    if (!container) return;

    const inputElement = container.querySelector("select, input");
    if (!inputElement) return;

    const settingName = inputElement.id;
    let value = inputElement.value;

    if (inputElement.type === "number") {
      let min = parseFloat(inputElement.min);
      let max = parseFloat(inputElement.max);
      let val = parseFloat(value);
      let errorMsg = document.getElementById(settingName + "_error");
      const unit = units[settingName] || "";

      if (val < min || val > max || isNaN(val)) {
        inputElement.style.border = "2px solid red";
        if (errorMsg) {
          errorMsg.textContent = `กรุณาใส่ค่าในช่วง ${min} - ${max} ${unit}`;
          errorMsg.style.display = "block";
        }
        return;
      } else {
        inputElement.style.border = "";
        if (errorMsg) errorMsg.style.display = "none";
      }

      const voltageKeys = [
        "BulkChargingVoltage",
        "FloatingChargingVoltage",
        "LowBatteryCutoffVoltage",
        "BatteryEqualizationVoltage"
      ];
      if (voltageKeys.includes(settingName)) {
        value = Math.round(val * 10);
      } else {
        value = val;
      }
    }

    settingToserver(settingName, value);
    console.log(`📤 ส่งค่าไป server: ${settingName} = ${value}`);
  };

  // ฟังก์ชันส่ง toggle checkbox
  function toggleSetting(checkbox, settingName) {
    const status = checkbox.checked ? "1" : "0";
    settingToserver(settingName, status);
    console.log(`ส่งค่า: ${settingName} = ${status}`);
  }

  // bind toggle checkbox
  document.querySelectorAll('.toggle-row input[type="checkbox"]').forEach(cb => {
    const settingName = cb.getAttribute("data-setting") || cb.id.replace(/\s+/g, "");
    cb.addEventListener("change", function() {
      toggleSetting(this, settingName);
      if (this.id === "battypeToggle") toggleSelect(this);
    });
  });

  // โหลดค่าจาก server
  fetch('/getbattsetting')
    .then(response => response.json())
    .then(data => {
      console.log(data);

      // เติม <select>
      for (const key in data) {
        const selectElement = document.getElementById(key);
        if (selectElement && selectElement.tagName === "SELECT") {
          const valueToSelect = data[key];
          for (const option of selectElement.options) {
            if (option.value.toLowerCase() === valueToSelect.toLowerCase()) {
              selectElement.value = option.value;
              break;
            }
          }
        }
      }

      // เติม <checkbox>
      for (const key in data) {
        const checkbox = document.getElementById(key);
        if (checkbox && checkbox.type === "checkbox") {
          checkbox.checked = data[key] === "1";
        }
      }

      // เติม <input type="number">
      for (const key in data) {
        const inputNumber = document.getElementById(key);
        if (inputNumber && inputNumber.tagName === "INPUT" && inputNumber.type === "number") {
          inputNumber.value = data[key];
        }
      }

      // 🔹 ปรับ min/max หลังเติมค่า
      toggleSelect(toggle);
    })
    .catch(error => console.error("Error fetching battery settings:", error));

  // ส่งค่าไป server
  function settingToserver(settingName, state) {
    fetch('/setting', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ setting: settingName, value: state })
    })
    .then(response => response.text())
    .then(result => console.log(`✅ ${settingName} updated: ${state}`, result))
    .catch(error => console.error("❌ Error:", error));

    submitAllSettings();
  }

  // เก็บค่าและส่งทั้งหมด
  function submitAllSettings() {
    const data = {};

    document.querySelectorAll('input[type="checkbox"]').forEach(input => {
      const id = input.id;
      if (id) data[id] = input.checked ? "1" : "0";
    });

    document.querySelectorAll('input[type="number"]').forEach(input => {
      const id = input.id;
      if (id) data[id] = input.value;
    });

    document.querySelectorAll('select').forEach(select => {
      const id = select.id;
      if (id) data[id] = select.value;
    });

    fetch('/battsetting', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(data)
    })
    .then(response => response.text())
    .then(result => console.log("Successed:", result))
    .catch(error => console.error("Error:", error));
  }

});
