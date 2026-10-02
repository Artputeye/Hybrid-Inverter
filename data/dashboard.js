const chartColors = { grid: "#e8f0ee", text: "#789195", blue: "#4d99df", yellow: "#f7b735" };
const historyRefreshInterval = 15 * 60 * 1000;
let energyHistoryData = null;
let energyHistoryRange = "hourly";
const energyHistorySeries = { grid: true, solar: true };
let energyHistoryTooltipState = null;
let energyHistoryGeometry = null;

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
  lamp.classList.remove("offline", "warning", "fault");
  if (state !== "normal" && state !== "online") lamp.classList.add(state);
}

function faultLevel(value) {
  const text = (Array.isArray(value) ? value.join(" ") : String(value ?? "")).trim().toLowerCase();
  if (text === "" || text === "[]" || text === "none" || text === "normal") return "clear";
  return /\b(warning|alarm)\b/.test(text) ? "warning" : "fault";
}

function setStatusValueState(valueId, lampId, state) {
  const value = document.getElementById(valueId);
  if (value) {
    value.classList.remove("online", "warning", "offline", "fault");
    value.classList.add(state);
  }
  setLamp(lampId, state);
}

/* --------------------------------------------------------------------------
  1. Live energy flow
  -------------------------------------------------------------------------- */
function updateLiveEnergyFlow(values) {
  const { pvRaw, activeRaw, gridRaw, batteryVoltage } = values;

  setPower("solarFlowValue", "solarFlowUnit", pvRaw);
  setPower("loadFlowValue", "loadFlowUnit", activeRaw);
  setPower("gridFlowValue", "gridFlowUnit", gridRaw);
  if (gridRaw !== null) setGridFlowValue(gridRaw);
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

/* --------------------------------------------------------------------------
  Load and output severity thresholds
  -------------------------------------------------------------------------- */
function classifyLoadLevel(percent) {
  if (percent === null) return "offline";
  if (percent > 74) return "fault";
  if (percent > 49) return "warning";
  return "normal";
}

function classifyVoltage(value) {
  if (value === null) return "offline";
  if (value > 245) return "fault";
  if (value > 240 || value < 215) return "warning";
  return "normal";
}

function classifyFrequency(value) {
  if (value === null) return "offline";
  const deviation = Math.abs(value - 50);
  if (deviation > 2) return "fault";
  if (deviation > 1) return "warning";
  return "normal";
}

function classifyPowerFactor(value) {
  if (value === null) return "offline";
  if (value < 0.5) return "fault";
  if (value < 0.85) return "warning";
  return "normal";
}

function setOutputCardState(cardId, state) {
  const card = document.getElementById(cardId);
  if (!card) return;
  card.classList.remove("normal", "warning", "fault", "offline");
  card.classList.add(state);
}

function updateLoadAndOutput(data, values) {
  const { activeRaw, temperature } = values;
  const reportedLoadPercent = numberValue(data["Load Percent"]);
  const ratedActivePower = numberValue(data["Output Rating Active Power"]);
  const loadPercent = activeRaw !== null && ratedActivePower !== null && ratedActivePower > 0
    ? Math.min(100, Math.max(0, Math.abs(activeRaw) / ratedActivePower * 100))
    : reportedLoadPercent;
  const apparentPower = numberValue(data["Output Apparent Power"]);
  const outputVoltage = numberValue(data["Output Voltage"]);
  const outputCurrent = numberValue(data["Output Current"]);
  const outputFrequency = numberValue(data["Output Frequency"]);
  const powerFactor = numberValue(data["Power Factor"]);
  const loadState = classifyLoadLevel(loadPercent);

  const loadGauge = document.getElementById("loadGaugeCircle");
  if (loadGauge) {
    loadGauge.classList.remove("normal", "warning", "fault", "offline");
    loadGauge.classList.add(loadState);
  }
  setOutputCardState("activePowerCard", activeRaw === null ? "offline" : loadState);
  setOutputCardState("apparentPowerCard", apparentPower === null ? "offline" : loadState);
  setOutputCardState("outputVoltageCard", classifyVoltage(outputVoltage));
  setOutputCardState("outputCurrentCard", outputCurrent === null ? "offline" : loadState);
  setOutputCardState("outputFrequencyCard", classifyFrequency(outputFrequency));
  setOutputCardState("powerFactorCard", classifyPowerFactor(powerFactor));

  setText("loadPercent", loadPercent === null ? "--%" : `${loadPercent.toFixed(0)}%`);
  setText("loadRingValue", loadPercent === null ? "--%" : `${loadPercent.toFixed(0)}%`);
  setText("loadGaugePercent", loadPercent === null ? "--" : loadPercent.toFixed(0));
  setText("ratedActivePower", ratedActivePower === null ? "--" : (ratedActivePower / 1000).toFixed(2));
  setText("ratedActivePowerUnit", "kW");
  setText("coreTemp", temperature === null ? "--" : temperature.toFixed(1));
  setPower("activePowerMetric", "activePowerMetricUnit", activeRaw);
  setPower("apparentPowerMetric", "apparentPowerMetricUnit", apparentPower, "VA", "kVA");
  setText("outputVoltageMetric", outputVoltage === null ? "--" : outputVoltage.toFixed(1));
  setText("outputCurrentMetric", outputCurrent === null ? "--" : outputCurrent.toFixed(1));
  setText("outputFrequencyMetric", outputFrequency === null ? "--" : outputFrequency.toFixed(1));
  setText("powerFactorMetric", powerFactor === null ? "--" : powerFactor.toFixed(2));
  updateRing("loadRingCircle", loadPercent, 106.81);
  updateRing("loadGaugeCircle", loadPercent, 263.89);
}

/* --------------------------------------------------------------------------
  4. System status and energy insights
   -------------------------------------------------------------------------- */
function updateSystemStatus(data, values) {
  const inverterFaultLevel = faultLevel(data["Inverter Faults"]);
  const inverterState = String(data["Inverter Status"] ?? "").trim();
  const inverterAvailable = inverterState !== "";
  const batteryAvailable = values.batteryVoltage !== null;
  const gridVoltage = numberValue(data["Grid Voltage"]);
  const gridRatingVoltage = numberValue(data["Grid Rating Voltage"]);
  const batteryUnderVoltage = numberValue(data["Battery Under Voltage"]);

  const inverterHealth = !inverterAvailable
    ? "offline"
    : inverterFaultLevel === "clear" ? "online" : inverterFaultLevel;

  let gridHealth = gridVoltage === null ? "offline" : "online";
  if (gridVoltage !== null && gridRatingVoltage !== null && gridRatingVoltage > 0) {
    const deviation = Math.abs(gridVoltage - gridRatingVoltage) / gridRatingVoltage;
    if (deviation > 0.15) gridHealth = "fault";
    else if (deviation > 0.10) gridHealth = "warning";
  }

  let batteryHealth = batteryAvailable ? "online" : "offline";
  if (batteryAvailable && batteryUnderVoltage !== null && batteryUnderVoltage > 0) {
    if (values.batteryVoltage <= batteryUnderVoltage) batteryHealth = "fault";
    else if (values.batteryVoltage <= batteryUnderVoltage * 1.05) batteryHealth = "warning";
  }

  const systemFault = [inverterHealth, gridHealth, batteryHealth].includes("fault");
  const systemWarning = [inverterHealth, gridHealth, batteryHealth].includes("warning");
  const hasTelemetry = values.pvRaw !== null || values.activeRaw !== null || batteryAvailable || inverterAvailable || gridVoltage !== null;

  setText("inverterStatus", inverterHealth === "fault" ? "Fault" : inverterHealth === "warning" ? "Warning" : inverterState || "--");
  setStatusValueState("inverterStatusValue", "inverterLamp", inverterHealth);

  const batteryText = !batteryAvailable
    ? "Telemetry unavailable"
    : batteryHealth === "fault" ? "Below cutoff"
      : batteryHealth === "warning" ? "Near cutoff"
        : `${values.batteryVoltage.toFixed(1)} V`;
  setText("batteryStatus", batteryText);
  setStatusValueState("batteryStatusValue", "batteryLamp", batteryHealth);

  setText("gridLinkStatus", gridVoltage === null ? "-- V" : `${gridVoltage.toFixed(1)} V`);
  setStatusValueState("gridStatusValue", "gridLamp", gridHealth);
  setText("systemStatusTitle", systemFault ? "System fault reported" : systemWarning ? "System warning" : hasTelemetry ? "System telemetry available" : "Waiting for telemetry");
  const statusBadge = document.getElementById("systemStatusBadge");
  if (statusBadge) {
    statusBadge.textContent = systemFault ? "FAULT" : systemWarning ? "WARNING" : hasTelemetry ? "LIVE DATA" : "NO DATA";
    statusBadge.classList.toggle("fault", systemFault);
    statusBadge.classList.toggle("warning", !systemFault && systemWarning);
    statusBadge.classList.toggle("online", !systemFault && !systemWarning && hasTelemetry);
    statusBadge.classList.toggle("offline", !systemFault && !systemWarning && !hasTelemetry);
  }
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
  setText("lastUpdate", new Date().toLocaleTimeString("en-GB", { hour: "2-digit", minute: "2-digit", hourCycle: "h23" }));
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
  CHARTS: ENERGY HISTORY
  -------------------------------------------------------------------------- */
// Draw square-bottom bars with softly rounded top corners.
function drawRoundedTopBar(context, x, y, width, height, color) {
  const radius = Math.min(4, width / 2, height);
  const baseline = y + height;

  context.beginPath();
  context.moveTo(x, baseline);
  context.lineTo(x, y + radius);
  context.quadraticCurveTo(x, y, x + radius, y);
  context.lineTo(x + width - radius, y);
  context.quadraticCurveTo(x + width, y, x + width, y + radius);
  context.lineTo(x + width, baseline);
  context.closePath();

  context.fillStyle = color;
  context.globalAlpha = 0.28;
  context.fill();
  context.globalAlpha = 1;
  context.strokeStyle = color;
  context.lineWidth = 1;
  context.stroke();
}

// Energy history renderer: grouped bars compare Grid and Solar per period.
function drawEnergyHistoryBars(canvasId, datasets, chartLabels) {
  const canvas = document.getElementById(canvasId);
  if (!canvas) return;

  const context = canvas.getContext("2d");
  const ratio = window.devicePixelRatio || 1;
  canvas.width = canvas.clientWidth * ratio;
  canvas.height = canvas.clientHeight * ratio;
  context.scale(ratio, ratio);

  const width = canvas.clientWidth;
  const height = canvas.clientHeight;
  const compact = width < 420;
  const padding = { top: 28, right: 12, bottom: 30, left: compact ? 38 : 46 };
  const plotWidth = width - padding.left - padding.right;
  const plotHeight = height - padding.top - padding.bottom;
  const values = datasets.flatMap((dataset) => dataset.data);
  const rawMaximum = Math.max(1, ...values);
  const targetStep = rawMaximum / 6;
  const magnitude = 10 ** Math.floor(Math.log10(targetStep));
  const normalizedStep = targetStep / magnitude;
  const stepFactor = normalizedStep <= 1 ? 1 : normalizedStep <= 2 ? 2 : normalizedStep <= 5 ? 5 : 10;
  const tickStep = stepFactor * magnitude;
  const tickCount = Math.ceil(rawMaximum / tickStep);
  const maximum = tickCount * tickStep;
  const yAt = (value) => padding.top + plotHeight - value / maximum * plotHeight;
  const baseline = padding.top + plotHeight;

  context.clearRect(0, 0, width, height);
  context.font = "500 10px DM Sans";
  context.fillStyle = chartColors.text;
  context.textAlign = "right";

  context.textAlign = "left";
  context.fillText("kWh", padding.left, 14);
  context.textAlign = "right";

  const labelStride = Math.max(1, Math.ceil(chartLabels.length / (compact ? 4 : 6)));
  for (let step = 0; step <= tickCount; step += 1) {
    const value = tickStep * step;
    const y = yAt(value);
    context.strokeStyle = step === 0 ? "#cfdeda" : chartColors.grid;
    context.lineWidth = step === 0 ? 1.2 : 1;
    context.beginPath();
    context.moveTo(padding.left, y);
    context.lineTo(width - padding.right, y);
    context.stroke();
    context.fillText(value.toFixed(tickStep < 1 ? 1 : 0), padding.left - 8, y + 3);
  }

  if (!chartLabels.length) return;

  const categoryWidth = plotWidth / chartLabels.length;
  energyHistoryGeometry = { paddingLeft: padding.left, categoryWidth };
  const seriesCount = Math.max(1, datasets.length);
  const barGap = seriesCount > 1 ? 2 : 0;
  const barWidth = Math.max(2, Math.min(18, (categoryWidth * 0.72 - barGap * (seriesCount - 1)) / seriesCount));
  const groupWidth = barWidth * seriesCount + barGap * (seriesCount - 1);

  // Add the vertical guides at the same intervals as the visible x-axis labels.
  context.strokeStyle = chartColors.grid;
  context.lineWidth = 1;
  for (let index = 0; index < chartLabels.length; index += labelStride) {
    const x = padding.left + (index + 0.5) * categoryWidth;
    context.beginPath();
    context.moveTo(x, padding.top);
    context.lineTo(x, baseline);
    context.stroke();
  }

  context.textAlign = "center";
  chartLabels.forEach((label, index) => {
    if (index === 0 || index === chartLabels.length - 1 || index % labelStride === 0) {
      const x = padding.left + (index + 0.5) * categoryWidth;
      context.fillText(label, x, height - 8);
    }
  });

  datasets.forEach((dataset, seriesIndex) => {
    dataset.data.forEach((rawValue, index) => {
      const value = Math.max(0, numberValue(rawValue) ?? 0);
      if (value === 0) return;
      const barHeight = baseline - yAt(value);
      const groupStart = padding.left + index * categoryWidth + (categoryWidth - groupWidth) / 2;
      const x = groupStart + seriesIndex * (barWidth + barGap);
      drawRoundedTopBar(context, x, baseline - barHeight, barWidth, barHeight, dataset.color);
    });
  });
}

function historyLabels(range, count) {
  const now = new Date();
  return Array.from({ length: count }, (_, index) => {
    const stepsBack = count - index - 1;
    const date = new Date(now);
    if (range === "hourly") {
      date.setHours(date.getHours() - stepsBack, 0, 0, 0);
      return date.toLocaleTimeString("en-GB", { hour: "2-digit", minute: "2-digit", hourCycle: "h23" });
    }
    if (range === "daily") {
      date.setDate(date.getDate() - stepsBack);
      return date.toLocaleDateString("en-US", { month: "short", day: "numeric" });
    }
    date.setDate(1);
    date.setMonth(date.getMonth() - stepsBack);
    return date.toLocaleDateString("en-US", { month: "short" });
  });
}

function renderCharts() {
  hideEnergyHistoryTooltip();
  const rangeSettings = {
    hourly: { grid: "grid_hourly", solar: "solar_hourly", count: 24 },
    daily: { grid: "grid_daily", solar: "solar_daily", count: 30 },
    monthly: { grid: "grid_monthly", solar: "solar_monthly", count: 12 }
  }[energyHistoryRange];
  const labels = historyLabels(energyHistoryRange, rangeSettings.count);
  const gridValues = energyHistoryData?.[rangeSettings.grid];
  const solarValues = energyHistoryData?.[rangeSettings.solar];

  if (Array.isArray(gridValues) && Array.isArray(solarValues)) {
    setText("energyHistoryStatus", "Saved every 15 min");
    energyHistoryTooltipState = {
      labels,
      grid: energyHistorySeries.grid ? gridValues : null,
      solar: energyHistorySeries.solar ? solarValues : null
    };
    const datasets = [];
    if (energyHistorySeries.grid) datasets.push({ data: gridValues, color: chartColors.blue });
    if (energyHistorySeries.solar) datasets.push({ data: solarValues, color: chartColors.yellow });
    drawEnergyHistoryBars("solarChart", datasets, labels);
  } else {
    energyHistoryTooltipState = null;
    drawEnergyHistoryBars("solarChart", [], []);
  }
}

// Show the selected period’s values in a floating tooltip on hover or touch.
function showEnergyHistoryTooltip(event) {
  const canvas = document.getElementById("solarChart");
  const chartWrap = canvas?.closest(".chart-wrap");
  const tooltip = document.getElementById("energyHistoryTooltip");
  const cursor = document.getElementById("energyHistoryCursor");
  if (!canvas || !chartWrap || !tooltip || !cursor || !energyHistoryTooltipState || !energyHistoryGeometry) return;

  const canvasRect = canvas.getBoundingClientRect();
  const wrapRect = chartWrap.getBoundingClientRect();
  const pointerX = event.clientX - canvasRect.left;
  const categoryIndex = Math.floor((pointerX - energyHistoryGeometry.paddingLeft) / energyHistoryGeometry.categoryWidth);
  if (categoryIndex < 0 || categoryIndex >= energyHistoryTooltipState.labels.length) {
    hideEnergyHistoryTooltip();
    return;
  }

  const gridValue = energyHistoryTooltipState.grid?.[categoryIndex];
  const solarValue = energyHistoryTooltipState.solar?.[categoryIndex];
  const gridRow = document.getElementById("energyHistoryTooltipGrid");
  const solarRow = document.getElementById("energyHistoryTooltipSolar");
  const formattedGrid = gridValue === undefined ? null : numberValue(gridValue);
  const formattedSolar = solarValue === undefined ? null : numberValue(solarValue);
  if (gridRow) gridRow.hidden = formattedGrid === null;
  if (solarRow) solarRow.hidden = formattedSolar === null;

  setText("energyHistoryTooltipDate", energyHistoryTooltipState.labels[categoryIndex]);
  setText("energyHistoryTooltipGridValue", formattedGrid === null ? "--" : `${formattedGrid.toFixed(2)} kWh`);
  setText("energyHistoryTooltipSolarValue", formattedSolar === null ? "--" : `${formattedSolar.toFixed(2)} kWh`);
  const total = (formattedGrid ?? 0) + (formattedSolar ?? 0);
  setText("energyHistoryTooltipTotal", `${total.toFixed(2)} kWh`);

  tooltip.hidden = false;
  const pointerInWrapX = canvasRect.left - wrapRect.left + pointerX;
  const pointerInWrapY = event.clientY - wrapRect.top;
  const tooltipLeft = Math.max(8, Math.min(pointerInWrapX + 12, chartWrap.clientWidth - tooltip.offsetWidth - 8));
  const tooltipTop = Math.max(8, pointerInWrapY - tooltip.offsetHeight - 12);
  tooltip.style.left = `${tooltipLeft}px`;
  tooltip.style.top = `${tooltipTop}px`;

  cursor.hidden = false;
  cursor.style.left = `${canvasRect.left - wrapRect.left + energyHistoryGeometry.paddingLeft + (categoryIndex + 0.5) * energyHistoryGeometry.categoryWidth}px`;
}

function hideEnergyHistoryTooltip() {
  const tooltip = document.getElementById("energyHistoryTooltip");
  const cursor = document.getElementById("energyHistoryCursor");
  if (tooltip) tooltip.hidden = true;
  if (cursor) cursor.hidden = true;
}

function setupEnergyHistoryTooltip() {
  const chartWrap = document.querySelector("#solarChart")?.closest(".chart-wrap");
  if (!chartWrap) return;
  chartWrap.addEventListener("pointermove", showEnergyHistoryTooltip);
  chartWrap.addEventListener("pointerdown", showEnergyHistoryTooltip);
  chartWrap.addEventListener("pointerleave", hideEnergyHistoryTooltip);
  chartWrap.addEventListener("pointercancel", hideEnergyHistoryTooltip);
}

// Load persistent energy arrays directly from LittleFS; the WebSocket stays lightweight.
async function fetchEnergyHistory() {
  try {
    const response = await fetch(`/energy_history.json?t=${Date.now()}`, { cache: "no-store" });
    if (!response.ok) throw new Error(`History request failed: ${response.status}`);
    energyHistoryData = await response.json();
    renderCharts();
  } catch (error) {
    setText("energyHistoryStatus", "History unavailable");
    renderCharts();
  }
}

function setupEnergyHistoryControls() {
  document.querySelectorAll("[data-energy-range]").forEach((button) => {
    button.addEventListener("click", () => {
      energyHistoryRange = button.dataset.energyRange;
      document.querySelectorAll("[data-energy-range]").forEach((option) => {
        const active = option === button;
        option.classList.toggle("active", active);
        option.setAttribute("aria-pressed", String(active));
      });
      renderCharts();
    });
  });

  document.querySelectorAll("[data-energy-series]").forEach((button) => {
    button.addEventListener("click", () => {
      const series = button.dataset.energySeries;
      energyHistorySeries[series] = !energyHistorySeries[series];
      const active = energyHistorySeries[series];
      button.classList.toggle("active", active);
      button.setAttribute("aria-pressed", String(active));
      renderCharts();
    });
  });
}

/**
   * ฟังก์ชันจัดการทิศทางเส้น Grid ตามค่าที่ส่งเข้ามา
   * @param {number} powerVal - ค่ากำลังไฟฟ้า (W) ถ้าติดลบจะวิ่งย้อนกลับไป Grid
   */

function setGridFlowValue(powerVal) {
  const gridPath = document.getElementById("gridFlowRoute");
  if (!gridPath) return;

  // Reverse the solid path and its motion particle while exporting to Grid.
  gridPath.setAttribute("d", powerVal < 0 ? "M 62.5,75 L 12.5,75" : "M 12.5,75 L 62.5,75");
}

// Compensate for the flow SVG's non-uniform scaling so motion markers stay circular.
function updateFlowParticleShape() {
  const flowSvg = document.querySelector(".flow-wiring");
  if (!flowSvg) return;

  const bounds = flowSvg.getBoundingClientRect();
  if (bounds.width === 0 || bounds.height === 0) return;

  const verticalRadius = 0.675 * bounds.width / bounds.height;
  flowSvg.querySelectorAll(".flow-particle").forEach((particle) => {
    particle.setAttribute("ry", verticalRadius.toFixed(3));
  });
}

/* --------------------------------------------------------------------------
  7. Dashboard initialization
   -------------------------------------------------------------------------- */
window.addEventListener("resize", renderCharts);
window.addEventListener("resize", updateFlowParticleShape);
renderCurrentDate();
updateFlowParticleShape();
setupEnergyHistoryControls();
setupEnergyHistoryTooltip();
fetchEnergyHistory();
window.setInterval(fetchEnergyHistory, historyRefreshInterval);
renderCharts();
connectTelemetry();
