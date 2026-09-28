/* --------------------------------------------------------------------------
     1. WebSocket connection and device status
     -------------------------------------------------------------------------- */
const gateway = `ws://${window.location.hostname}/ws`;
let websocket;
let reconnectTimer;

function updateConnectionStatus(state) {
    const status = document.getElementById("connectionState");
    const label = document.getElementById("connectionLabel");
    if (!status || !label) return;

    status.classList.remove("is-connected", "is-disconnected");
    if (state === "connected") {
        status.classList.add("is-connected");
        label.textContent = "Connected";
    } else if (state === "disconnected") {
        status.classList.add("is-disconnected");
        label.textContent = "Reconnecting";
    } else {
        label.textContent = "Connecting";
    }
}

function getReadings() {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send("getReadings");
    }
}

function initWebSocket() {
    if (websocket && (websocket.readyState === WebSocket.OPEN || websocket.readyState === WebSocket.CONNECTING)) {
        return;
    }

    updateConnectionStatus("connecting");
    websocket = new WebSocket(gateway);

    websocket.onopen = () => {
        updateConnectionStatus("connected");
        getReadings();
    };

    websocket.onclose = () => {
        updateConnectionStatus("disconnected");
        clearTimeout(reconnectTimer);
        reconnectTimer = setTimeout(initWebSocket, 2000);
    };

    websocket.onerror = () => websocket.close();

    websocket.onmessage = (event) => {
        try {
            const decodedText = atob(event.data).replace(/[\x00-\x1F\x7F]/g, "");
            const readings = JSON.parse(decodedText);
            updateDeviceIp(readings["DIVICE_IP"]);
            appendReading("Serial", readings.Serial);
            appendReading("Inverter", readings.Inverter);
            appendReading("controll", readings.controll);
        } catch (error) {
            console.error("WebSocket message decode error:", error, event.data);
        }
    };
}

function updateDeviceIp(ipAddress) {
    if (!ipAddress) return;

    ["device_ip", "esp32-ip"].forEach((id) => {
        const element = document.getElementById(id);
        if (element) element.textContent = ipAddress;
    });
}

/* --------------------------------------------------------------------------
     2. Terminal output
     -------------------------------------------------------------------------- */
function appendToTerminal(message, type = "received") {
    const terminal = document.getElementById("terminal");
    if (!terminal) {
        console.log("Terminal Log:", message);
        return;
    }

    const emptyState = document.getElementById("terminalEmpty");
    if (emptyState) emptyState.remove();

    const line = document.createElement("div");
    line.className = `terminal-message ${type}`;
    line.textContent = message;
    terminal.appendChild(line);
    terminal.scrollTop = terminal.scrollHeight;
}

function appendReading(source, message) {
    if (message) appendToTerminal(`${source}: ${message}`, "received");
}

function clearTerminal() {
    const terminal = document.getElementById("terminal");
    if (!terminal) return;

    terminal.replaceChildren();
    const emptyState = document.createElement("p");
    emptyState.className = "terminal-empty";
    emptyState.id = "terminalEmpty";
    emptyState.textContent = "Waiting for device messages...";
    terminal.appendChild(emptyState);
}

/* --------------------------------------------------------------------------
     3. Inverter command input
     -------------------------------------------------------------------------- */
function sendCommand(event) {
    event.preventDefault();
    const input = document.getElementById("messageInput");
    const message = input ? input.value.trim() : "";
    if (!message) return;

    fetchToserver(message);
    appendToTerminal(`Sent: ${message}`, "sent");
    input.value = "";
    input.focus();
}

function fetchToserver(message) {
    const formData = new FormData();
    formData.append("plain", message);
    fetch("/terminalSet", { method: "POST", body: formData, redirect: "follow" })
        .then((response) => response.text())
        .then((result) => console.log("Server response:", result))
        .catch((error) => console.error("Server command error:", error));
}

/* --------------------------------------------------------------------------
     4. Page initialization
     -------------------------------------------------------------------------- */
function initializeInfoPage() {
    const commandForm = document.getElementById("commandForm");
    const clearButton = document.getElementById("clearBtn");

    if (commandForm) commandForm.addEventListener("submit", sendCommand);
    if (clearButton) clearButton.addEventListener("click", clearTerminal);
    initWebSocket();
}

window.addEventListener("load", initializeInfoPage);