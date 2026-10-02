document.addEventListener("DOMContentLoaded", () => {
  // --------------------------------------------------------------------------
  // 1. WebSocket connection
  // --------------------------------------------------------------------------
  const gateway = `ws://${window.location.hostname}/ws`;
  let websocket;
  let reconnectTimer;

  function connectToDevice() {
    if (websocket && (websocket.readyState === WebSocket.OPEN || websocket.readyState === WebSocket.CONNECTING)) return;
    websocket = new WebSocket(gateway);
    websocket.addEventListener("open", requestDeviceStatus);
    websocket.addEventListener("message", handleDeviceMessage);
    websocket.addEventListener("close", () => {
      clearTimeout(reconnectTimer);
      reconnectTimer = window.setTimeout(connectToDevice, 2000);
    });
    websocket.addEventListener("error", () => websocket.close());
  }

  // --------------------------------------------------------------------------
  // 2. Device status requests
  // --------------------------------------------------------------------------
  function requestDeviceStatus() {
    websocket?.send("getReadings");
  }

  // --------------------------------------------------------------------------
  // 3. Payload decode — same method as dashboard.js (Uint8Array + TextDecoder)
  // --------------------------------------------------------------------------
  function decodeTelemetry(message) {
    const binary = atob(message);
    const bytes = Uint8Array.from(binary, (ch) => ch.charCodeAt(0));
    return JSON.parse(new TextDecoder().decode(bytes).replace(/[\x00-\x1F\x7F]/g, ""));
  }

  // --------------------------------------------------------------------------
  // 4. Device status rendering
  // --------------------------------------------------------------------------
  function handleDeviceMessage(event) {
    try {
      const readings = decodeTelemetry(event.data);

      Object.entries(readings).forEach(([key, value]) => {
        // "Inverter Faults" จัดการโดย updateFaultState() เพื่อแปลง "Normal" → "N/A"
        if (key === "Inverter Faults") return;

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

  // --------------------------------------------------------------------------
  // 5. Fault state — toggle has-fault / has-warning on the .status-row element
  // --------------------------------------------------------------------------
  function updateFaultState(fault) {
    const faultElement = document.getElementById("Inverter Faults");
    if (!faultElement || fault === undefined) return;

    const faultText = (Array.isArray(fault) ? fault.join(", ") : String(fault ?? "")).trim();
    const normalizedFault = faultText.toLowerCase();
    const isClear = normalizedFault === "" || normalizedFault === "normal" || normalizedFault === "none" || normalizedFault === "[]";
    const isWarning = !isClear && /\b(warning|alarm)\b/i.test(faultText);
    const faultRow = faultElement.closest(".status-row");

    // แสดง faultList โดยตรง: "Normal" → "Normal", มี fault → ชื่อ fault จาก QPIWS
    // "N/A" สงวนไว้เมื่อ map bit แล้วไม่มีชื่อตรงกัน (faultText ว่าง)
    faultElement.textContent = faultText || "N/A";

    faultRow?.classList.toggle("has-fault", !isClear && !isWarning);
    faultRow?.classList.toggle("has-warning", isWarning);
  }

  // --------------------------------------------------------------------------
  // 6. Page initialization
  // --------------------------------------------------------------------------
  connectToDevice();
});
