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

    document.getElementById("wifiModeToggle").checked = config.wifi_mode === "1";
    document.getElementById("ipConfigToggle").checked = config.ip_config === "1";
    updateWifiMode();
    updateIpConfig();
  } catch (error) {
    console.error("Configuration load error:", error);
  }
}

/* --------------------------------------------------------------------------
   2. Connection mode toggles
   -------------------------------------------------------------------------- */
function updateWifiMode() {
  const isStation = document.getElementById("wifiModeToggle").checked;
  document.getElementById("wifi-mode").textContent = isStation ? "STATION" : "ACCESS POINT";
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

document.addEventListener("DOMContentLoaded", initializeNetworkPage);

function fetchToserver(message) {
  const formData = new FormData();
  formData.append("plain", message);
  fetch("/terminalSet", { method: "POST", body: formData, redirect: "follow" })
    .then((response) => response.text())
    .then((result) => console.log("Server response:", result))
    .catch((error) => console.error("Server command error:", error));
}