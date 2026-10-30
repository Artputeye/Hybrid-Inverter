document.addEventListener("DOMContentLoaded", () => {
  // Inverter voltage settings use decivolts (e.g. 24.0V = 240).
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
  const voltageSelects = {
    VoltagePointBacktoUtility: { "24": [210, 255], "48": [420, 510] },
    VoltagePointBacktoBattery: { "24": [240, 290], "48": [480, 580] }
  };

  const toggle = document.getElementById("battypeToggle");
  const battLabel = document.getElementById("battType");

  function setRangeAndLabel(inputId, labelId, voltType) {
    const input = document.getElementById(inputId);
    const label = document.getElementById(labelId);
    if (!input || !ranges[inputId]) return;
    const [min, max] = Array.isArray(ranges[inputId]) ? ranges[inputId] : ranges[inputId][voltType];
    input.min = min;
    input.max = max;
    if (label) label.textContent = `${min.toFixed(1)} - ${max.toFixed(1)} ${units[inputId] || ""}`;
  }

  function updateVoltageDropdowns(voltType) {
    Object.entries(voltageSelects).forEach(([id, limits]) => {
      const select = document.getElementById(id);
      if (!select) return;
      const previousValue = Number(select.value);
      const [min, max] = limits[voltType];
      const targetValue = previousValue
        ? Math.round(previousValue * (voltType === "48" ? 2 : 0.5) / 5) * 5
        : 0;

      select.replaceChildren();
      for (let value = min; value <= max; value += 5) {
        const option = document.createElement("option");
        option.value = String(value);
        option.textContent = (value / 10).toFixed(1);
        select.appendChild(option);
      }
      select.value = targetValue >= min && targetValue <= max ? String(targetValue) : String(min);
    });
  }

  function toggleSelect(checkbox) {
    const voltType = checkbox?.checked ? "48" : "24";
    if (battLabel) battLabel.textContent = voltType + "V";
    ["BulkChargingVoltage", "FloatingChargingVoltage", "LowBatteryCutoffVoltage", "BatteryEqualizationVoltage"]
      .forEach(id => setRangeAndLabel(id, id.replace("Voltage", "_Voltage"), voltType));
    ["BatteryEqualizationTime", "BatteryEqualizationTimeout", "BatteryEqualizationInterval"]
      .forEach(id => setRangeAndLabel(id, id.replace("BatteryEqualization", "BatteryEqualization_"), voltType));
    updateVoltageDropdowns(voltType);
  }

  document.querySelectorAll('.toggle-row input[type="checkbox"]').forEach(cb => {
    const settingName = cb.getAttribute("data-setting") || cb.id.replace(/\s+/g, "");
    cb.addEventListener("change", function() {
      if (this.id === "battypeToggle") {
        // 24V/48V is a local battery profile, not an inverter register.
        toggleSelect(this);
        submitAllSettings();
        return;
      }
      sendToServer(settingName, this.checked ? 1 : 0);
    });
  });

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
        inputElement.style.border = "2px solid red";
        if (errorMsg) {
          errorMsg.textContent = `กรุณาใส่ค่าในช่วง ${min} - ${max} ${units[settingName] || ""}`;
          errorMsg.style.display = "block";
        }
        return;
      }
      inputElement.style.border = "";
      if (errorMsg) errorMsg.style.display = "none";
      if (["BulkChargingVoltage", "FloatingChargingVoltage", "LowBatteryCutoffVoltage", "BatteryEqualizationVoltage"].includes(settingName)) {
        val = Math.round(val * 10);
      }
    }
    sendToServer(settingName, parseInt(val, 10));
  };

  function submitAllSettings() {
    const data = {};
    document.querySelectorAll('input[type="checkbox"]').forEach(i => {
      if (i.id) data[i.id] = i.checked ? "1" : "0";
    });
    document.querySelectorAll('input[type="number"]').forEach(i => {
      if (i.id) data[i.id] = i.value;
    });
    document.querySelectorAll("select").forEach(i => {
      if (i.id) data[i.id] = i.value;
    });
    fetch("/battery.json", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(data)
    }).catch(err => console.error("❌ [SAVE ERROR]:", err));
  }

  function sendToServer(settingName, value) {
    const payload = { setting: settingName, value: Number(value) };
    console.log("📤 [BATTERY SETTING]", payload);
    fetch("/invsetting", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    })
      .then(res => {
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        return res.text();
      })
      .then(response => {
        console.log("✅ [INVERTER RESPONSE]", response);
        submitAllSettings();
      })
      .catch(err => console.error("❌ [INVERTER SET ERROR]", err));
  }

  fetch("/battery.json")
    .then(res => {
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      return res.json();
    })
    .then(data => {
      if (toggle && Object.prototype.hasOwnProperty.call(data, "battypeToggle")) {
        toggle.checked = data.battypeToggle === "1" || data.battypeToggle === 1 || data.battypeToggle === true;
      }
      toggleSelect(toggle);
      Object.keys(data).forEach(key => {
        if (key === "battypeToggle") return;
        const el = document.getElementById(key);
        if (!el) return;
        if (el.tagName === "SELECT") el.value = String(data[key]);
        else if (el.type === "checkbox") el.checked = data[key] === "1" || data[key] === 1 || data[key] === true;
        else if (el.type === "number") el.value = data[key];
      });
      console.log("📥 [BATTERY RESTORE]", data);
    })
    .catch(err => {
      console.error("❌ [BATTERY RESTORE ERROR]", err);
      toggleSelect(toggle);
    });

  // Show the default profile immediately while saved settings are loading.
  toggleSelect(toggle);
});