function sendConfig() {
  const inputs = document.querySelectorAll('.setting-input input');
  const config = {};

  // เก็บค่าจาก input text
  inputs.forEach(input => {
    if (input.id && input.value !== "") {
      config[input.id] = input.value;
    }
  });

  // เก็บค่าจาก toggle switch
  config.wifi_mode = document.getElementById("wifiModeToggle").checked ? "1" : "0";
  config.ip_config = document.getElementById("ipConfigToggle").checked ? "1" : "0";

  // แก้ไข URL ตรงนี้จาก '/networkconfig' เป็น '/networkconfig.json' ให้ตรงกับระบบใหม่
  fetch('/networkconfig.json', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(config)
  })
    .then(response => {
      if (response.ok) {
        alert("Configuration saved successfully.");
        fetchToserver("espreset");
      } else {
        alert("Error saving configuration.");
      }
    })
    .catch(error => {
      console.error('Fetch error:', error);
      alert("Fetch failed.");
    });
  console.log(config);
}
function wifiMode(checkbox) {
  const mode = document.getElementById("wifi-mode");
  const status = checkbox.checked ? "STATION" : "ACCESS POINT";
  mode.textContent = status;
}

document.querySelectorAll('.icon-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    const targetId = btn.getAttribute('data-target');
    const input = document.getElementById(targetId);

    const isShown = input.type === 'text';
    input.type = isShown ? 'password' : 'text';

    if (!input) {
      console.error(`Element with id="${targetId}" not found`);
      return;
    }

    btn.setAttribute('aria-pressed', String(!isShown));
    btn.setAttribute('aria-label', isShown ? 'แสดงรหัสผ่าน' : 'ซ่อนรหัสผ่าน');
  });
});

/////////////////////////////////////////////////////////////////////////////////////////////////
// โหลด config ตอนเปิดหน้า
window.addEventListener("load", () => {
  loadConfig();
});

function loadConfig() {
  // เปลี่ยน URL ตรงนี้ให้ตรงกับชื่อไฟล์ (ซึ่งกลายเป็น Endpoint URL ของเราแล้ว)
  fetch("/networkconfig.json")
    .then(res => {
      // เพิ่มการตรวจจับ Error เผื่อในกรณีที่บอร์ดหาไฟล์ไม่เจอ หรือส่ง 404/500 กลับมา
      if (!res.ok) {
        throw new Error(`HTTP error! status: ${res.status}`);
      }
      return res.json();
    })
    .then(config => {
      console.log("Received config:", config);

      // Input field
      document.getElementById("wifi_ssid").value = config.wifi_ssid || "";
      document.getElementById("wifi_pass").value = config.wifi_pass || "";
      document.getElementById("mqtt_user").value = config.mqtt_user || "";
      document.getElementById("mqtt_pass").value = config.mqtt_pass || "";
      document.getElementById("mqtt_server").value = config.mqtt_server || "";
      document.getElementById("mqtt_port").value = config.mqtt_port || "";

      document.getElementById("ip_address").value = config.ip_address || "";
      document.getElementById("subnet_mask").value = config.subnet_mask || "";
      document.getElementById("default_gateway").value = config.default_gateway || "";

      // Toggle: WIFI MODE
      const wifiToggle = document.getElementById("wifiModeToggle");
      if (config.wifi_mode === "1") {
        wifiToggle.checked = true;
        document.getElementById("wifi-mode").textContent = "STATION";
      } else {
        wifiToggle.checked = false;
        document.getElementById("wifi-mode").textContent = "ACCESS POINT";
      }

      // Toggle: IP CONFIG
      const ipConfigToggle = document.getElementById("ipConfigToggle");
      if (config.ip_config === "1") {
        ipConfigToggle.checked = true;
        document.getElementById("ip-hide").style.display = "block";
      } else {
        ipConfigToggle.checked = false;
        document.getElementById("ip-hide").style.display = "none";
      }
    })
    .catch(err => console.error("Config load error:", err));
}

// เวลา toggle IP Config เปลี่ยน
function IPconfig(el) {
  if (el.checked) {
    document.getElementById("ip-hide").style.display = "block";
  } else {
    document.getElementById("ip-hide").style.display = "none";
  }
}

function toggleSetting(checkbox, settingName) {
  const status = checkbox.checked ? "1" : "0";
  const command = settingName + " " + status;
  console.log(command);
  //fetchToserver(command);
}

function fetchToserver(message) {
    console.log(`${message} to Server`);
    const formdata = new FormData();
    formdata.append("plain", message);
    const requestOptions = {
        method: "POST",
        body: formdata,
        redirect: "follow"
    };
    fetch("/terminalSet", requestOptions)
        .then((response) => response.text())
        .then((result) => console.log("Respond:", result))
        .catch((error) => console.error("Error:", error));
}