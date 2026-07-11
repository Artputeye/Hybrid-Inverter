var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

// Init web socket when the page loads
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
    fetchToserver("QPIRI");
}

function getReadings() {
    websocket.send("getReadings");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

// When websocket is established, call the getReadings() function
function onOpen(event) {
    console.log('Connection opened');
    getReadings();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
    try {
        // event.data เป็น Base64 string ตรง ๆ
        const base64Text = event.data;

        // Decode Base64 → JSON string
        let jsonText = atob(base64Text);

        // 🔹 ลบ control characters ที่ไม่ใช่ ASCII ปกติ
        jsonText = jsonText.replace(/[\x00-\x1F\x7F]/g, "");

        // Parse JSON
        const myObj = JSON.parse(jsonText);

        // อัปเดตค่าเข้า HTML
        Object.keys(myObj).forEach((key) => {
            const el = document.getElementById(key);
            if (el) {
                el.innerHTML = myObj[key];
            }
        });

    } catch (err) {
        console.error("Decode/Parse error", err, event.data);
    }
}

function updateFaults(faults) {
  const faultDiv = document.getElementById("inverter fault");
  if (faults.length === 0) {
    faultDiv.textContent = "OK";
    faultDiv.style.color = "green";
  } else {
    faultDiv.textContent = faults.join(", ");
    faultDiv.style.color = "red";
  }
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
    fetch("/cmd", requestOptions)
        .then((response) => response.text())
        .then((result) => console.log("Respond:", result))
        .catch((error) => console.error("Error:", error));
}
