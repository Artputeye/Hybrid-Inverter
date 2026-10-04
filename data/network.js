/* --------------------------------------------------------------------------
   1. Load and save network configuration
   -------------------------------------------------------------------------- */
async function sendConfig() {
  const config = {};

  document.querySelectorAll(".network-input").forEach((input) => {
    if (input.id && input.value !== "") {
      config[input.id] = input.value;
    }
  });

  config.wifi_mode = document.getElementById("wifiModeToggle").checked ? "1" : "0";

  const deviceName = document.getElementById("device_name");
  const hostname = document.getElementById("hostname");
  if (deviceName) config.device_name = deviceName.value.trim();
  if (hostname) config.hostname = hostname.value.trim();
  config.ip_config = document.getElementById("ipConfigToggle").checked ? "1" : "0";

  try {
    const response = await fetch("/networkconfig.json", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(config)
    });

    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }

    alert("Configuration saved successfully.");
    fetchToserver("espreset");
  } catch (error) {
    console.error("Configuration save error:", error);
    alert("Error saving configuration.");
  }
}

async function loadConfig() {
  try {
    const response = await fetch("/networkconfig.json");
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }

    const config = await response.json();
    document.querySelectorAll(".network-input").forEach((input) => {
      input.value = config[input.id] || "";
    });

    document.getElementById("wifiModeToggle").checked = isStationMode(config.wifi_mode);
    document.getElementById("ipConfigToggle").checked = isStaticIpMode(config.ip_config);
    updateWifiMode();
    updateIpConfig();
  } catch (error) {
    console.error("Configuration load error:", error);
  }
}

/* --------------------------------------------------------------------------
   2. Connection mode toggles
   -------------------------------------------------------------------------- */
function isStationMode(value) {
  return value === true || value === 1 || value === "1" || String(value).toLowerCase() === "true";
}

function isStaticIpMode(value) {
  return value === true || value === 1 || value === "1" || String(value).toLowerCase() === "true";
}

function updateWifiMode() {
  const isStation = document.getElementById("wifiModeToggle").checked;
  document.getElementById("wifi-mode").textContent = isStation ? "STATION" : "ACCESS POINT";

  const stationStatus = document.getElementById("station-status");
  if (stationStatus) stationStatus.hidden = !isStation;
}

function updateIpConfig() {
  const isStatic = document.getElementById("ipConfigToggle").checked;
  document.getElementById("ipConfig").textContent = isStatic ? "STATIC IP" : "DHCP";
  document.getElementById("ip-hide").hidden = !isStatic;
}

/* --------------------------------------------------------------------------
   3. Password visibility controls
   -------------------------------------------------------------------------- */
function initializePasswordToggles() {
  document.querySelectorAll(".password-toggle").forEach((button) => {
    button.addEventListener("click", () => {
      const input = document.getElementById(button.dataset.target);
      if (!input) return;

      const isVisible = input.type === "text";
      input.type = isVisible ? "password" : "text";
      button.setAttribute("aria-pressed", String(!isVisible));
      button.setAttribute("aria-label", `${isVisible ? "Show" : "Hide"} ${input.id.replace("_", " ")}`);
      button.title = isVisible ? "Show password" : "Hide password";
    });
  });
}

/* --------------------------------------------------------------------------
   4. Page initialization and server command
   -------------------------------------------------------------------------- */
function initializeNetworkPage() {
  document.getElementById("wifiModeToggle").addEventListener("change", updateWifiMode);
  document.getElementById("ipConfigToggle").addEventListener("change", updateIpConfig);
  document.getElementById("save-network-config").addEventListener("click", sendConfig);
  initializePasswordToggles();
  updateWifiMode();
  updateIpConfig();
  loadConfig();
}

/* --------------------------------------------------------------------------
   5. Live Station status via WebSocket
   -------------------------------------------------------------------------- */
let networkWebSocket;
let networkReconnectTimer;

function formatUptime(value) {
  if (!value) return "--";
  const bootTime = new Date(value);
  if (Number.isNaN(bootTime.getTime())) return value;

  const seconds = Math.max(0, Math.floor((Date.now() - bootTime.getTime()) / 1000));
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  const secs = seconds % 60;

  return `${days}d ${String(hours).padStart(2, "0")}:${String(minutes).padStart(2, "0")}:${String(secs).padStart(2, "0")}`;
}

function updateStationStatus(readings) {
  const wifiToggle = document.getElementById("wifiModeToggle");
  if (!wifiToggle || wifiToggle.checked !== true) return;

  const ip = readings["DIVICE_IP"];
  const mac = readings["MAC Address"];
  const uptime = readings["Uptime"];
  const rssi = readings["WiFi Signal"];

  if (ip !== undefined) document.getElementById("ws-ip-address").textContent = ip || "--";
  if (mac !== undefined) document.getElementById("ws-mac-address").textContent = mac || "--";
  if (uptime !== undefined) document.getElementById("ws-uptime").textContent = formatUptime(uptime);
  if (rssi !== undefined) document.getElementById("ws-wifi-signal").textContent = `${rssi} dBm`;
}

function connectNetworkWebSocket() {
  if (!window.WebSocket) return;
  if (networkWebSocket &&
      (networkWebSocket.readyState === WebSocket.OPEN ||
       networkWebSocket.readyState === WebSocket.CONNECTING)) return;

  networkWebSocket = new WebSocket(`ws://${window.location.host}/ws`);

  networkWebSocket.addEventListener("open", () => networkWebSocket.send("getReadings"));
  networkWebSocket.addEventListener("message", (event) => {
    try {
      const binary = atob(event.data);
      const bytes = Uint8Array.from(binary, (character) => character.charCodeAt(0));
      const readings = JSON.parse(new TextDecoder().decode(bytes).replace(/[\x00-\x1F\x7F]/g, ""));
      updateStationStatus(readings);
    } catch (error) {
      console.error("Network WebSocket decode error:", error);
    }
  });
  networkWebSocket.addEventListener("close", () => {
    clearTimeout(networkReconnectTimer);
    networkReconnectTimer = setTimeout(connectNetworkWebSocket, 2000);
  });
  networkWebSocket.addEventListener("error", () => networkWebSocket.close());
}

document.addEventListener("DOMContentLoaded", () => {
  initializeNetworkPage();
  connectNetworkWebSocket();
});

function fetchToserver(message) {
  const formData = new FormData();
  formData.append("plain", message);
  fetch("/terminalSet", { method: "POST", body: formData, redirect: "follow" })
    .then((response) => response.text())
    .then((result) => console.log("Server response:", result))
    .catch((error) => console.error("Server command error:", error));
}