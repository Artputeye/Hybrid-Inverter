const chartColors = { grid: "#e8f0ee", text: "#789195", blue: "#4d99df", pink: "#d978b0", yellow: "#f7b735" };
const history = { labels: [], solar: [], load: [], grid: [] };
const maxSamples = 24;

/* --------------------------------------------------------------------------
  Shared telemetry helpers and connection state
  -------------------------------------------------------------------------- */
function numberValue(value) {
  const number = Number.parseFloat(value);
  return Number.isFinite(number) ? number : null;
}

function formatPower(value, baseUnit = "W", scaledUnit = "kW") {
  const number = numberValue(value);
  if (number === null) return { value: "--", unit: baseUnit };
  if (Math.abs(number) > 1500) {
    return { value: (number / 1000).toFixed(2), unit: scaledUnit };
  }
  return { value: String(number), unit: baseUnit };
}

function setPower(valueId, unitId, value, baseUnit = "W", scaledUnit = "kW") {
  const formatted = formatPower(value, baseUnit, scaledUnit);
  setText(valueId, formatted.value);
  setText(unitId, formatted.unit);
}

function setText(id, value) {
  const element = document.getElementById(id);
  if (element) element.textContent = value;
}

function updateConnection(connected) {
  setText("connectionText", connected ? "Connected" : "Reconnecting");
  setLamp("connectionLamp", connected ? "normal" : "offline");
  const dot = document.getElementById("connectionDot");
  if (dot) dot.classList.toggle("offline", !connected);
}

function renderCurrentDate() {
  setText("currentDate", new Intl.DateTimeFormat("en-US", { weekday: "long", year: "numeric", month: "long", day: "numeric" }).format(new Date()));
}

function setLamp(id, state) {
  const lamp = document.getElementById(id);
  if (!lamp) return;
  lamp.classList.remove("offline", "fault");
  if (state !== "normal") lamp.classList.add(state);
}

function hasFault(value) {
  if (Array.isArray(value)) return value.length > 0;
  const text = String(value ?? "").trim().toLowerCase();
  return text !== "" && text !== "[]" && text !== "none" && text !== "normal";
}

/* --------------------------------------------------------------------------
  1. Live energy flow
  -------------------------------------------------------------------------- */
function updateLiveEnergyFlow(values) {
  const { pvRaw, activeRaw, gridRaw, batteryVoltage } = values;

  setPower("solarFlowValue", "solarFlowUnit", pvRaw);
  setPower("loadFlowValue", "loadFlowUnit", activeRaw);
  setPower("gridFlowValue", "gridFlowUnit", gridRaw);
  setText("batteryFlowValue", batteryVoltage === null ? "--" : batteryVoltage.toFixed(1));
  setText("batteryFlowUnit", "V");
}

/* --------------------------------------------------------------------------
  2. Live energy summary
  -------------------------------------------------------------------------- */
function updateEnergySummary(values) {
  const { gridDaily, gridMonthly, solarDaily, solarMonthly } = values;

  setText("gridEnergyDaily", gridDaily === null ? "--" : gridDaily.toFixed(3));
  setText("gridEnergyMonthly", gridMonthly === null ? "--" : gridMonthly.toFixed(3));
  setText("solarEnergyDaily", solarDaily === null ? "--" : solarDaily.toFixed(3));
  setText("solarEnergyMonthly", solarMonthly === null ? "--" : solarMonthly.toFixed(3));
}

/* --------------------------------------------------------------------------
  3. Load gauge and output electrical parameters
   -------------------------------------------------------------------------- */
function formatFixed(value, decimals) {
  const number = numberValue(value);
  return number === null ? "--" : number.toFixed(decimals);
}

function updateRing(circleId, percent, circumference) {
  const circle = document.getElementById(circleId);
  if (!circle) return;

  const boundedPercent = percent === null ? 0 : Math.min(100, Math.max(0, percent));
  circle.style.strokeDasharray = String(circumference);
  circle.style.strokeDashoffset = String(circumference - (boundedPercent / 100) * circumference);
}

function updateLoadAndOutput(data, values) {
  const { activeRaw, temperature } = values;
  const loadPercent = numberValue(data["Load Percent"]);
  const apparentPower = numberValue(data["Output Apparent Power"]);

  setText("loadPercent", loadPercent === null ? "--%" : `${loadPercent.toFixed(0)}%`);
  setText("loadRingValue", loadPercent === null ? "--%" : `${loadPercent.toFixed(0)}%`);
  setText("loadGaugePercent", loadPercent === null ? "--" : loadPercent.toFixed(0));
  setPower("activeLoadPower", "activeLoadPowerUnit", activeRaw);
  setText("coreTemp", temperature === null ? "--" : temperature.toFixed(1));
  setPower("activePowerMetric", "activePowerMetricUnit", activeRaw);
  setPower("apparentPowerMetric", "apparentPowerMetricUnit", apparentPower, "VA", "kVA");
  setText("outputVoltageMetric", formatFixed(data["Output Voltage"], 1));
  setText("outputCurrentMetric", formatFixed(data["Output Current"], 1));
  setText("outputFrequencyMetric", formatFixed(data["Output Frequency"], 1));
  setText("powerFactorMetric", formatFixed(data["Power Factor"], 2));
  updateRing("loadRingCircle", loadPercent, 106.81);
  updateRing("loadGaugeCircle", loadPercent, 263.89);
}

/* --------------------------------------------------------------------------
  4. System status and energy insights
   -------------------------------------------------------------------------- */
function updateSystemStatus(data, values) {
  const fault = hasFault(data["Inverter Faults"]);
  const inverterState = String(data["Inverter Status"] ?? "").trim();
  const inverterAvailable = inverterState !== "";
  const batteryAvailable = values.batteryVoltage !== null;
  const gridVoltage = numberValue(data["Grid Voltage"]);
  const hasTelemetry = values.pvRaw !== null || values.activeRaw !== null || batteryAvailable || inverterAvailable || gridVoltage !== null;

  setText("inverterStatus", fault ? (inverterState || "Fault reported") : (inverterState || "--"));
  setLamp("inverterLamp", fault ? "fault" : inverterAvailable ? "normal" : "offline");
  const inverterStatusValue = document.getElementById("inverterStatusValue");
  if (inverterStatusValue) {
    inverterStatusValue.classList.toggle("fault", fault);
    inverterStatusValue.classList.toggle("offline", !fault && !inverterAvailable);
  }

  setText("batteryStatus", batteryAvailable ? `Battery voltage ${values.batteryVoltage.toFixed(1)} V` : "Battery telemetry unavailable");
  setLamp("batteryLamp", batteryAvailable ? "normal" : "offline");
  const batteryStatusValue = document.getElementById("batteryStatusValue");
  if (batteryStatusValue) batteryStatusValue.classList.toggle("offline", !batteryAvailable);
  setText("gridLinkStatus", gridVoltage === null ? "-- V" : `${gridVoltage.toFixed(1)} V`);
  setText("systemStatusTitle", fault ? "Inverter fault reported" : hasTelemetry ? "System telemetry available" : "Waiting for telemetry");
  setText("systemStatusBadge", fault ? "FAULT" : hasTelemetry ? "LIVE DATA" : "NO DATA");
}

function updateEnergyInsights(data) {
  setText("pvCurrent", formatFixed(data["PV Current"], 1));
  setText("pvVoltage", formatFixed(data["PV Voltage"], 1));

  const selfSufficiency = numberValue(data.selfSufficiencyPct);
  const boundedPercent = selfSufficiency === null ? 0 : Math.min(100, Math.max(0, selfSufficiency));
  setText("selfSufficiencyValue", selfSufficiency === null ? "--" : `${boundedPercent.toFixed(1)}%`);
  const progressBar = document.getElementById("selfSufficiencyBar");
  if (progressBar) progressBar.style.width = `${boundedPercent}%`;
}

function updateExpenseSummary(data) {
  ["gridCostMonthly", "solarSavingsMonthly"].forEach((key) => {
    const amount = numberValue(data[key]);
    setText(key, amount === null ? "--" : amount.toLocaleString("en-US", { minimumFractionDigits: 2, maximumFractionDigits: 2 }));
  });
}

/* --------------------------------------------------------------------------
  5. Monthly energy benefits
  -------------------------------------------------------------------------- */
function updateEnergyBenefits(data) {
  const energySavings = numberValue(data.solarSavingsMonthly);
  const co2Reduction = numberValue(data.co2ReductionMonthlyKg);

  setText("benefitEnergySavings", energySavings === null ? "--" : `${energySavings.toLocaleString("en-US", { minimumFractionDigits: 2, maximumFractionDigits: 2 })} THB`);
  setText("benefitCo2Reduction", co2Reduction === null ? "--" : `${co2Reduction.toFixed(2)} kg`);
}

function applyTelemetry(data) {
  const pvRaw = numberValue(data["PV Power"]);
  const gridRaw = numberValue(data["Grid Power"]);
  const activeRaw = numberValue(data["Output Active Power"]);
  const batteryVoltage = numberValue(data["Battery Voltage"]);
  const temperature = numberValue(data.Temperature);
  const gridDaily = numberValue(data.energy_kWh ?? data["Energy Daily"]);
  const gridMonthly = numberValue(data.energy_m_kWh);
  const solarDaily = numberValue(data.solar_kWh);
  const solarMonthly = numberValue(data.solar_m_kWh);

  const values = { pvRaw, gridRaw, activeRaw, batteryVoltage, temperature, gridDaily, gridMonthly, solarDaily, solarMonthly };
  updateLiveEnergyFlow(values);
  updateEnergySummary(values);
  updateLoadAndOutput(data, values);
  updateSystemStatus(data, values);
  updateEnergyInsights(data);
  updateExpenseSummary(data);
  updateEnergyBenefits(data);
  setText("lastUpdate", new Date().toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" }));
  appendHistory(pvRaw, activeRaw, gridRaw);
}

/* --------------------------------------------------------------------------
  5. WebSocket connection and payload decoding
   -------------------------------------------------------------------------- */
function decodeTelemetry(message) {
  const binary = atob(message);
  const bytes = Uint8Array.from(binary, (character) => character.charCodeAt(0));
  return JSON.parse(new TextDecoder().decode(bytes).replace(/[\x00-\x1F\x7F]/g, ""));
}

let telemetrySocket;
let reconnectTimer;

function connectTelemetry() {
  if (!window.location.hostname || !window.WebSocket) return;
  if (telemetrySocket && (telemetrySocket.readyState === WebSocket.OPEN || telemetrySocket.readyState === WebSocket.CONNECTING)) return;

  telemetrySocket = new WebSocket(`ws://${window.location.host}/ws`);
  telemetrySocket.addEventListener("open", () => { updateConnection(true); telemetrySocket.send("getReadings"); });
  telemetrySocket.addEventListener("message", (event) => {
    try { applyTelemetry(decodeTelemetry(event.data)); } catch (error) { console.error("Telemetry decode error", error); }
  });
  telemetrySocket.addEventListener("close", () => {
    updateConnection(false);
    clearTimeout(reconnectTimer);
    reconnectTimer = setTimeout(connectTelemetry, 2000);
  });
  telemetrySocket.addEventListener("error", () => telemetrySocket.close());
}

/* --------------------------------------------------------------------------
  6. Live telemetry charts
   -------------------------------------------------------------------------- */
function appendHistory(pvPower, activePower, gridPower) {
  if ([pvPower, activePower, gridPower].some((value) => value === null)) return;
  history.labels.push(new Date().toLocaleTimeString([], { hour: "2-digit", minute: "2-digit", second: "2-digit" }));
  history.solar.push(pvPower / 1000);
  history.load.push(activePower / 1000);
  history.grid.push(Math.abs(gridPower) / 1000);
  Object.values(history).forEach((series) => { if (series.length > maxSamples) series.shift(); });
  renderCharts();
}

function drawChart(canvasId, datasets) {
  const canvas = document.getElementById(canvasId);
  if (!canvas) return;
  const context = canvas.getContext("2d");
  const ratio = window.devicePixelRatio || 1;
  canvas.width = canvas.clientWidth * ratio; canvas.height = canvas.clientHeight * ratio;
  context.scale(ratio, ratio);
  const width = canvas.clientWidth, height = canvas.clientHeight;
  const compact = width < 420;
  const padding = { top: 18, right: 12, bottom: 30, left: compact ? 28 : 36 };
  const plotWidth = width - padding.left - padding.right, plotHeight = height - padding.top - padding.bottom;
  const labels = history.labels, values = datasets.flatMap((dataset) => dataset.data);
  const maximum = Math.max(1, ...values) * 1.15;
  const xAt = (index) => padding.left + (labels.length === 1 ? 0.5 : index / (labels.length - 1)) * plotWidth;
  const yAt = (value) => padding.top + plotHeight - value / maximum * plotHeight;
  context.clearRect(0, 0, width, height);
  context.font = "500 10px DM Sans"; context.fillStyle = chartColors.text; context.textAlign = "right";
  for (let step = 0; step <= 4; step += 1) {
    const value = maximum * step / 4, y = yAt(value);
    context.strokeStyle = step === 0 ? "#cfdeda" : chartColors.grid;
    context.lineWidth = step === 0 ? 1.2 : 1;
    context.beginPath(); context.moveTo(padding.left, y); context.lineTo(width - padding.right, y); context.stroke();
    if (step < 4) context.fillText(value.toFixed(1), padding.left - 7, y + 3);
  }
  if (!labels.length) return;
  context.textAlign = "center";
  const labelStride = Math.max(1, Math.ceil(labels.length / (compact ? 3 : 5)));
  labels.forEach((label, index) => {
    if (index === 0 || index === labels.length - 1 || index % labelStride === 0) context.fillText(label, xAt(index), height - 8);
  });
  datasets.forEach((dataset) => {
    const color = dataset.color;
    const gradient = context.createLinearGradient(0, padding.top, 0, padding.top + plotHeight);
    gradient.addColorStop(0, `${color}33`);
    gradient.addColorStop(1, `${color}00`);
    context.beginPath();
    dataset.data.forEach((value, index) => index === 0 ? context.moveTo(xAt(index), yAt(value)) : context.lineTo(xAt(index), yAt(value)));
    context.lineTo(xAt(dataset.data.length - 1), padding.top + plotHeight);
    context.lineTo(xAt(0), padding.top + plotHeight);
    context.closePath(); context.fillStyle = gradient; context.fill();
    context.beginPath();
    dataset.data.forEach((value, index) => index === 0 ? context.moveTo(xAt(index), yAt(value)) : context.lineTo(xAt(index), yAt(value)));
    context.strokeStyle = color; context.lineWidth = 2.5; context.lineJoin = "round"; context.lineCap = "round"; context.stroke();
    dataset.data.forEach((value, index) => {
      context.beginPath(); context.arc(xAt(index), yAt(value), compact ? 2.5 : 3, 0, Math.PI * 2);
      context.fillStyle = "#ffffff"; context.fill(); context.strokeStyle = color; context.lineWidth = 1.8; context.stroke();
    });
  });
}

function renderCharts() {
  drawChart("solarChart", [{ data: history.solar, color: chartColors.yellow }]);
  drawChart("loadChart", [{ data: history.load, color: chartColors.blue }, { data: history.grid, color: chartColors.pink }]);
}

/**
   * ฟังก์ชันจัดการทิศทางเส้น Grid ตามค่าที่ส่งเข้ามา
   * @param {number} powerVal - ค่ากำลังไฟฟ้า (W) ถ้าติดลบจะวิ่งย้อนกลับไป Grid
   */

function setGridFlowValue(powerVal) {
  const gridValueElem = document.getElementById('gridFlowValue');
  const gridPathElem = document.querySelector('.grid-path');

  if (!gridValueElem || !gridPathElem) return;

  // แสดงผลตัวเลข (จะเอาเครื่องหมายลบออก หรือคงไว้ก็ได้)
  gridValueElem.textContent = powerVal;

  // ถ้าค่าติดลบ ให้เพิ่ม Class reverse-path เพื่อให้เส้นวิ่งย้อนกลับ (Home -> Grid)
  if (powerVal < 0) {
    gridPathElem.classList.add('reverse-path');
  } else {
    gridPathElem.classList.remove('reverse-path');
  }
}

/* --------------------------------------------------------------------------
  7. Dashboard initialization
   -------------------------------------------------------------------------- */
window.addEventListener("resize", renderCharts);
renderCurrentDate();
renderCharts();
connectTelemetry();
