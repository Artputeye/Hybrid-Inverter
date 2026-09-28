document.addEventListener("DOMContentLoaded", () => {
  // --------------------------------------------------------------------------
  // 1. WebSocket connection
  // --------------------------------------------------------------------------
  const gateway = `ws://${window.location.hostname}/ws`;
  let websocket;

  function connectToDevice() {
    websocket = new WebSocket(gateway);
    websocket.addEventListener("open", requestDeviceStatus);
    websocket.addEventListener("message", handleDeviceMessage);
    websocket.addEventListener("close", reconnectToDevice);
    websocket.addEventListener("error", error => {
      console.error("Device status WebSocket error:", error);
    });
  }

  function reconnectToDevice() {
    window.setTimeout(connectToDevice, 2000);
  }

  // --------------------------------------------------------------------------
  // 2. Device status requests
  // --------------------------------------------------------------------------
  function requestDeviceStatus() {
    sendCommand("QPIRI");
    window.setTimeout(() => sendCommand("QPIWS"), 500);
  }

  function sendCommand(command) {
    const formData = new FormData();
    formData.append("plain", command);

    fetch("/terminalSet", {
      method: "POST",
      body: formData,
      redirect: "follow"
    }).catch(error => console.error("Device status request error:", error));
  }

  // --------------------------------------------------------------------------
  // 3. Device status rendering
  // --------------------------------------------------------------------------
  function handleDeviceMessage(event) {
    try {
      const jsonText = atob(event.data).replace(/[\x00-\x1F\x7F]/g, "");
      const readings = JSON.parse(jsonText);

      Object.entries(readings).forEach(([key, value]) => {
        const element = document.getElementById(key);
        if (!element) return;

        const displayValue = key === "Output Rating Current" && !isNaN(value)
          ? parseFloat(value).toFixed(1)
          : value;
        element.textContent = displayValue;
      });

      updateFaultState(readings["Inverter Faults"]);
    } catch (error) {
      console.error("Device status decode error:", error);
    }
  }

  function updateFaultState(fault) {
    const faultElement = document.getElementById("Inverter Faults");
    if (!faultElement || fault === undefined) return;

    faultElement.closest(".status-row")?.classList.toggle("has-fault", fault !== "Normal");
  }

  // --------------------------------------------------------------------------
  // 4. Page initialization
  // --------------------------------------------------------------------------
  connectToDevice();
});
