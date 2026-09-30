document.addEventListener("DOMContentLoaded", () => {
  // Switch between inverter parameters and expense settings.
  const viewOptions = document.querySelectorAll("[data-settings-view]");
  const settingsViews = {
    inverter: document.getElementById("inverterSettingsView"),
    expense: document.getElementById("expenseSettingsView")
  };

  viewOptions.forEach(option => {
    option.addEventListener("click", () => {
      const selectedView = option.dataset.settingsView;
      viewOptions.forEach(item => {
        const isSelected = item === option;
        item.classList.toggle("active", isSelected);
        item.setAttribute("aria-selected", String(isSelected));
      });
      Object.entries(settingsViews).forEach(([name, view]) => {
        if (view) view.hidden = name !== selectedView;
      });
    });
  });

  // --------------------------------------------------------------------------
  // 1. Restore inverter settings
  // --------------------------------------------------------------------------
  fetch('/setting.json')
    .then(res => res.json())
    .then(data => {
      console.log("📥 [RESTORE] Loaded settings from /setting.json:", data);
      
      // Restore select, checkbox, and number controls.
      Object.keys(data).forEach(id => {
        const el = document.getElementById(id);
        if (!el) return;
        if (el.tagName === "SELECT") el.value = data[id];
        else if (el.type === "checkbox") el.checked = data[id] === "1" || data[id] === 1;
        else if (el.type === "number") el.value = data[id];
      });

      // Update the dependent Grid Tie Operation control and expense tier headers.
      const gridTieAuto = document.getElementById("Grid Tie Auto");
      if (gridTieAuto) updateGridTieUI(gridTieAuto.checked);
      updateTierLabelsUI();
    })
    .catch(err => console.error("❌ [RESTORE ERROR] Fetching settings:", err));
});

// --------------------------------------------------------------------------
// 2. Shared server communication
// --------------------------------------------------------------------------
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

// --------------------------------------------------------------------------
// 3. Selection and schedule settings
// --------------------------------------------------------------------------
function sendSetting(element) {
  const container = element.closest('.setting-form, .setting-card');
  if (!container) return;

  const settingType = container.getAttribute('data-setting') || element.id;
  const inputEl = container.querySelector('select, input');
  if (!inputEl) return;

  // แปลงเป็น Number เสมอ
  const numericValue = parseInt(inputEl.value, 10);
  const finalValue = isNaN(numericValue) ? 0 : numericValue;

  console.log(`🔘 [ACTION] Inverter setting change triggered`);

  postJSON('/invsetting', {
    setting: settingType,
    value: finalValue
  }).then(() => {
    submitAllSettings();
  });
}

// --------------------------------------------------------------------------
// 4. Inverter mode toggles
// --------------------------------------------------------------------------
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

// --------------------------------------------------------------------------
// 5. Grid schedule and dependent controls
// --------------------------------------------------------------------------
function GridCutToServer() {
  const gridCutOff = parseInt(document.getElementById("gridCutOff")?.value, 10) || 0;
  const gridStart = parseInt(document.getElementById("gridStart")?.value, 10) || 0;

  console.log(`🔘 [ACTION] Grid Cutoff / Start Setting triggered`);

  postJSON('/invsetting', {
    gridCutOff: gridCutOff,
    gridStart: gridStart
  }).then(() => submitAllSettings());
}

function restoreDefaults() {
  if (!confirm("Restore inverter defaults?")) return;

  postJSON('/invsetting', {
    setting: 'RestoreDefaults',
    value: 1
  }).then(() => window.location.reload());
}

function updateGridTieUI(isAuto) {
  const gridTieOp = document.getElementById("Grid Tie Operation");
  if (gridTieOp) {
    gridTieOp.disabled = isAuto;
    gridTieOp.parentElement.classList.toggle("disabled", isAuto);
  }
}

// --------------------------------------------------------------------------
// 6. Expense settings handler
// --------------------------------------------------------------------------
function saveExpenseSettings() {
  const expenseData = {
    UnitCost1: document.getElementById("UnitCost1")?.value || "200",
    PriceCost1: document.getElementById("PriceCost1")?.value || "3.0000",
    UnitCost2: document.getElementById("UnitCost2")?.value || "400",
    PriceCost2: document.getElementById("PriceCost2")?.value || "4.1584",
    PriceCost3: document.getElementById("PriceCost3")?.value || "4.3583",
    UnitSolar: document.getElementById("UnitSolar")?.value || "450",
    ft: document.getElementById("ft")?.value || "0.3972",
    ServiceFee: document.getElementById("ServiceFee")?.value || "38.22",
    VatRate: document.getElementById("VatRate")?.value || "7"
  };

  console.log("💰 [ACTION] Saving Expense Settings:", expenseData);
  submitAllSettings();
  alert("Expense settings saved successfully!");
}

/**
 * คำนวณค่าไฟฟ้าฐานแบบอัตราก้าวหน้า (Progressive Tier Rates)
 * @param {number} totalUnits จำนวนหน่วยไฟฟ้าที่ใช้ทั้งหมด (kWh)
 * @param {Object} cfg ข้อมูลโครงสร้างอัตราค่าไฟฟ้า
 * @returns {number} ค่าไฟฟ้าฐานรวม (บาท)
 */
function calculateProgressiveGridCost(totalUnits, cfg = {}) {
  const tier1Limit = parseFloat(cfg.UnitCost1 || 200);
  const tier1Price = parseFloat(cfg.PriceCost1 || 3.0000);
  const tier2Limit = parseFloat(cfg.UnitCost2 || 400);
  const tier2Price = parseFloat(cfg.PriceCost2 || 4.1584);
  const tier3Price = parseFloat(cfg.PriceCost3 || 4.3583);

  let cost = 0;
  if (totalUnits <= 0) return 0;

  // Tier 1: 1 - Tier 1 Limit (เช่น 1-200 หน่วยแรก)
  const tier1Units = Math.min(totalUnits, tier1Limit);
  cost += tier1Units * tier1Price;

  // Tier 2: Tier 1 Limit + 1 - Tier 2 Limit (เช่น 201-400 หน่วย)
  if (totalUnits > tier1Limit) {
    const tier2Units = Math.min(totalUnits - tier1Limit, tier2Limit - tier1Limit);
    cost += tier2Units * tier2Price;
  }

  // Tier 3: เกินกว่า Tier 2 Limit ขึ้นไป (เช่น > 400 หน่วย)
  if (totalUnits > tier2Limit) {
    const tier3Units = totalUnits - tier2Limit;
    cost += tier3Units * tier3Price;
  }

  return cost;
}

/**
 * อัปเดตข้อความหัวข้อ Tier Ranges Dynamic ในหน้า Expense Settings
 */
function updateTierLabelsUI() {
  const tier1Val = parseInt(document.getElementById("UnitCost1")?.value || document.getElementById("UnitCost1")?.placeholder || 200, 10);
  const tier2Val = parseInt(document.getElementById("UnitCost2")?.value || document.getElementById("UnitCost2")?.placeholder || 400, 10);

  const t1RangeText = document.getElementById("tier1RangeText");
  const t2StartText = document.getElementById("tier2StartText");
  const t2EndText = document.getElementById("tier2EndText");
  const t3StartText = document.getElementById("tier3StartText");

  if (t1RangeText) t1RangeText.textContent = tier1Val;
  if (t2StartText) t2StartText.textContent = tier1Val + 1;
  if (t2EndText) t2EndText.textContent = tier2Val;
  if (t3StartText) t3StartText.textContent = tier2Val;
}

// --------------------------------------------------------------------------
// 7. Save the complete settings state
// --------------------------------------------------------------------------
function submitAllSettings() {
  const data = {};
  document.querySelectorAll('input[type="checkbox"]').forEach(el => el.id && (data[el.id] = el.checked ? "1" : "0"));
  document.querySelectorAll('input[type="number"]').forEach(el => el.id && (data[el.id] = el.value));
  document.querySelectorAll('select').forEach(el => el.id && (data[el.id] = el.value));

  console.log("💾 [SYNCING] Saving full state to /setting.json...");
  postJSON('/setting.json', data);
}
