document.addEventListener("DOMContentLoaded", function () {
  fetch('/getsetting')
    .then(response => response.json())
    .then(data => {
      // ✅ โหลดค่ากลับให้ select
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

      // ✅ โหลดค่ากลับให้ checkbox
      for (const key in data) {
        const checkbox = document.getElementById(key);
        if (checkbox && checkbox.type === "checkbox") {
          checkbox.checked = data[key] === "1";
        }
      }

      // ✅ โหลดค่ากลับให้ input number (gridCutOff, gridStart)
      for (const key in data) {
        const numberInput = document.getElementById(key);
        if (numberInput && numberInput.type === "number") {
          numberInput.value = data[key];
        }
      }

      // ✅ เพิ่มส่วนตรวจสอบ Grid Tie Auto
      const gridTieAuto = document.getElementById("Grid Tie Auto");
      const gridTieOp = document.getElementById("Grid Tie Operation");
      if (gridTieAuto && gridTieOp) {
        const autoEnabled = data["Grid Tie Auto"] === "1";
        gridTieOp.disabled = autoEnabled; // ปิดการใช้งานปุ่มเมื่อ auto = 1
        gridTieOp.parentElement.classList.toggle("disabled", autoEnabled);
      }
    })
    .catch(error => {
      console.error("Error fetching toggle data:", error);
    });
});

function toggleSetting(checkbox, settingName) {
  const status = checkbox.checked ? 1 : 0;
  console.log(`${settingName} ${status}`);

  // ✅ ถ้าเป็น Grid Tie Auto → ปิด/เปิดการใช้งานปุ่ม Grid Tie Operation
  if (settingName === "Grid Tie Auto") {
    const gridTieOp = document.getElementById("Grid Tie Operation");
    if (gridTieOp) {
      gridTieOp.disabled = checkbox.checked;
      gridTieOp.parentElement.classList.toggle("disabled", checkbox.checked);
    }
  }
  settingToserver(settingName, status);
}

// ส่งค่า Toggle ไปยังเซิร์ฟเวอร์
function settingToserver(settingName, state) {
  console.log(`${settingName} has been toggled to ${state}`);

  fetch('/setting', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      setting: settingName,   // ✅ ไม่มีเลขติดชื่อ
      value: state
    }),
  })
    .then(response => response.text())
    .then(result => {
      console.log(`✅ Toggle ${settingName} updated to ${state}`);
      console.log("Respond:", result);
    })
    .catch(error => {
      console.error("❌ Error:", error);
    });

  submitAllSettings();
}

//ส่งค่า dropdown ไปยังเซิร์ฟเวอร์
function sendSetting(data) {
  const container = data.closest('.setting-dropdown');
  const settingType = container.getAttribute('data-setting');
  const value = container.querySelector('select').value;

  fetch('/setting', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ setting: settingType, value }),
  })
    .then(response => response.text())
    .then(data => {
      console.log(`✅ Setting ${settingType} updated to ${value}`);
    })
    .catch(error => {
      console.error('❌ Error:', error);
    });
  submitAllSettings();
}

function submitAllSettings() {
  const data = {};

  // เก็บค่าจาก toggle switch ทั้งหมด
  document.querySelectorAll('input[type="checkbox"]').forEach(input => {
    const id = input.id;
    if (id) {
      data[id] = input.checked ? "1" : "0";
    }
  });

  // เก็บค่าจาก input ทั้งหมด
    document.querySelectorAll('input[type="number"]').forEach(input => {
    const id = input.id;
    if (id) {
      data[id] = input.value;
    }
  });

  // เก็บค่าจาก dropdown ทั้งหมด
  document.querySelectorAll('select').forEach(select => {
    const id = select.id;
    if (id) {
      data[id] = select.value;   // <-- ใช้ value ไม่ใช่ text
    }
  });

  console.log("Successed:", data);

  // ส่ง JSON ไปยัง ESP32
  fetch('/savesetting', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(data)
  })
    .then(response => response.text())
    .then(result => {
      console.log("Successed:", result);
    })
    .catch(error => {
      console.error("Error:", error);
    });
}

function restoreDefaults() {
  console.log("⚠️ Restoring defaults...");

  // รีเซ็ต dropdown → ค่าแรก
  document.querySelectorAll('select').forEach(select => {
    if (select.options.length > 0) {
      select.selectedIndex = 0;
    }
  });

  // รีเซ็ต toggle switch → 0
  document.querySelectorAll('input[type="checkbox"]').forEach(cb => {
    cb.checked = false;
  });

  // ส่งค่าที่รีเซ็ตไปยัง ESP32
  settingToserver("RestoreDefaults", 1)
  submitAllSettings();
  alert("✅ Settings restored to defaults.");
}


function GridCutToServer() {
  const gridCutOff = parseInt(document.getElementById('gridCutOff').value);
  const gridStart = parseInt(document.getElementById('gridStart').value);

  // ✅ ตรวจสอบว่าค่าถูกต้องและอยู่ในช่วง 1–31
  if (isNaN(gridCutOff) || isNaN(gridStart)) {
    alert('กรุณากรอกค่าทั้งสองช่อง');
    return;
  }
  if (gridCutOff < 1 || gridCutOff > 31 || gridStart < 1 || gridStart > 31) {
    alert('ค่าต้องอยู่ระหว่าง 1 ถึง 31 เท่านั้น');
    return;
  }
  console.log(`📤 ส่งค่าไป server: gridCutOff=${gridCutOff}, gridStart=${gridStart}`);
  fetch('/setting', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ gridCutOff, gridStart })
  })
    .then(res => res.text())
    .then(result => {
      console.log('✅ Respond:', result);
      alert('บันทึกค่าเรียบร้อย');
    })
    .catch(err => {
      console.error('❌ Error:', err);
      alert('เกิดข้อผิดพลาดในการเชื่อมต่อเซิร์ฟเวอร์');
    });
      submitAllSettings();
}