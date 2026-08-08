/**
 * VoltFlow Telemetry & WebSocket Controller
 * ARTTECH Inverter Dashboard Integration
 */

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

// Initialize layout & WebSocket on window load
window.addEventListener('load', onload);

function onload(event) {
  initWebSocket();
  initPlotlyChart();
  updateLiveTimestamp();
  setInterval(updateLiveTimestamp, 1000);
}

function getReadings() {
  if (websocket && websocket.readyState === WebSocket.OPEN) {
    websocket.send("getReadings");
  }
}

function initWebSocket() {
  console.log('Trying to open a WebSocket connection…');
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
}

function onOpen(event) {
  console.log('Connection opened');
  getReadings();
  fetchToserver("QPIRI");

  setTimeout(() => {
    fetchToserver("QPIWS");
  }, 500);
}

function onClose(event) {
  console.log('Connection closed');
  setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
  try {
    // Decode incoming Base64 string directly
    const base64Text = event.data;
    let jsonText = atob(base64Text);

    // Strip out non-printable ASCII control characters
    jsonText = jsonText.replace(/[\x00-\x1F\x7F]/g, "");

    // Parse telemetry payload
    const myObj = JSON.parse(jsonText);
    console.log("Telemetry Received:", myObj);

    // Dynamic Element Binding by key
    Object.keys(myObj).forEach((key) => {
      const el = document.getElementById(key);
      if (el) {
        let val = myObj[key];

        // บังคับให้ Output Current เป็นทศนิยม 1 ตำแหน่ง
        if (key === "Output Current" && !isNaN(val)) {
          val = parseFloat(val).toFixed(1);
        }

        el.innerHTML = val;
      }
    });

    // Handle special gauge visual update
    if (myObj["Load Percent"] !== undefined) {
      updateGauge(parseFloat(myObj["Load Percent"]));
    }

  } catch (err) {
    console.error("Decode/Parse error:", err, event.data);
  }
}

/**
 * Send terminal/server execution commands
 */
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

/**
 * Updates the SVG circular progress offset based on percentage (0-100%)
 */
function updateGauge(percentage) {
  const circle = document.getElementById("gaugeProgress");
  if (!circle) return;

  const radius = circle.r.baseVal.value;
  const circumference = 2 * Math.PI * radius; // Approx 408px
  
  const clamped = Math.min(Math.max(percentage, 0), 100);
  const offset = circumference - (clamped / 100) * circumference;
  
  circle.style.strokeDashoffset = offset;
}

/**
 * Live Clock Header Sync Generator
 */
function updateLiveTimestamp() {
  const now = new Date();
  const timeStr = now.toTimeString().split(' ')[0] + '.' + String(now.getMilliseconds()).padStart(2, '0').slice(0, 2);
  const syncEl = document.getElementById("last-sync-time");
  if (syncEl) {
    syncEl.textContent = timeStr;
  }
}

/**
 * Render Advanced Energy Flow Trace Chart with Plotly.js
 */
function initPlotlyChart() {
  const traceFeedIn = {
    x: ['00:00', '03:00', '06:00', '09:00', '12:00', '15:00', '18:00', '21:00'],
    y: [0.2, 0.4, 0.9, 1.6, 1.5, 1.1, 0.5, 0.3],
    name: 'FEED-IN',
    type: 'bar',
    marker: {
      color: '#00b4d8',
      borderRadius: 4
    }
  };

  const traceConsumption = {
    x: ['00:00', '03:00', '06:00', '09:00', '12:00', '15:00', '18:00', '21:00'],
    y: [0.8, 0.7, 0.5, 0.5, 0.7, 0.9, 1.2, 1.0],
    name: 'CONSUMPTION',
    type: 'bar',
    marker: {
      color: '#10b981',
      borderRadius: 4
    }
  };

  const layout = {
    barmode: 'group',
    bargap: 0.3,
    bargroupgap: 0.1,
    margin: { l: 30, r: 10, t: 10, b: 30 },
    paper_bgcolor: 'transparent',
    plot_bgcolor: 'transparent',
    showlegend: false,
    xaxis: {
      color: '#64748b',
      font: { family: 'JetBrains Mono', size: 10 },
      showgrid: false
    },
    yaxis: {
      color: '#64748b',
      font: { family: 'JetBrains Mono', size: 10 },
      gridcolor: '#e2e8f0',
      zerolinecolor: '#e2e8f0'
    }
  };

  const config = { responsive: true, displayModeBar: false };

  Plotly.newPlot('energyTraceChart', [traceFeedIn, traceConsumption], layout, config);
}