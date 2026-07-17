var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getReadings() {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send("getReadings");
    }
}

function initWebSocket() {
    websocket = new WebSocket(gateway);

    websocket.onopen = () => {
        console.log("WebSocket Opened");
        getReadings();
    };

    websocket.onclose = () => {
        console.log("WebSocket Closed");
        setTimeout(initWebSocket, 2000);
    };

    websocket.onmessage = (event) => {
        const ts = new Date().toLocaleTimeString();
        try {
            // event.data เป็น Base64 string
            const rawText = atob(event.data);

            // ลบอักขระควบคุมที่ไม่ใช่ printable ASCII (0x20-0x7E)
            const jsonText = rawText.replace(/[\x00-\x1F\x7F]/g, "");
            const obj = JSON.parse(jsonText);

            console.log("Decoded JSON:", obj);

            // 🔹 แก้ไขจุดผิด: เปลี่ยนจาก data เป็น obj และตรวจเช็ก Element บนหน้าเว็บ
            const ipElem = document.getElementById("device_ip");
            if (ipElem && obj["DIVICE_IP"]) {
                ipElem.textContent = obj["DIVICE_IP"];
            }

            // 🔹 เผื่อหน้า network.html มีการใช้ id เป็น esp32-ip
            const espIpElem = document.getElementById("esp32-ip");
            if (espIpElem && obj["DIVICE_IP"]) {
                espIpElem.textContent = obj["DIVICE_IP"];
            }

            if (obj["Serial"]) {
                console.log(`Serial : ${obj["Serial"]}`);
                appendToTerminal(`Serial: ${obj["Serial"]}`);
            }
            if (obj["Inverter"]) {
                console.log(`Inverter: ${obj["Inverter"]}`);
                appendToTerminal(`Inverter: ${obj["Inverter"]}`);
            }
            if (obj["controll"]) {
                console.log(`controll: ${obj["controll"]}`);
                appendToTerminal(`controll: ${obj["controll"]}`);
            }

        } catch (err) {
            console.error("Decode error:", err, event.data);
        }
    };
}

// 🔹 ดักเช็กปุ่ม Send ก่อนผูก Event (มีเฉพาะใน info.html)
const sendBtn = document.getElementById("sendBtn");
if (sendBtn) {
    sendBtn.addEventListener("click", () => {
        const ts = new Date().toLocaleTimeString();
        const msgInput = document.getElementById("messageInput");
        if (msgInput) {
            const msg = msgInput.value.trim();
            if (msg) {
                fetchToserver(msg);
                appendToTerminal(`Sent : ${msg}`);
                msgInput.value = "";
            }
        }
    });
}

// 🔹 ดักเช็กปุ่ม Clear ก่อนผูก Event (มีเฉพาะใน info.html)
const clearBtn = document.getElementById("clearBtn");
if (clearBtn) {
    clearBtn.addEventListener("click", () => {
        const termElem = document.getElementById("terminal");
        if (termElem) termElem.innerHTML = "";
    });
}

// 🔹 ดักเช็กกล่องข้อความก่อนผูก Event กด Enter (มีเฉพาะ in info.html)
const messageInput = document.getElementById("messageInput");
if (messageInput) {
    messageInput.addEventListener("keydown", (e) => {
        const ts = new Date().toLocaleTimeString();
        const msg = messageInput.value.trim();
        if (e.key === 'Enter') {
            if (msg) {
                fetchToserver(msg);
                appendToTerminal(`Sent : ${msg}`);
                messageInput.value = "";
            }
        }
    });
}

// 🔹 ปรับปรุงฟังก์ชัน Terminal ให้ปลอดภัย ตรวจสอบโครงสร้างก่อนต่อ Element
function appendToTerminal(message) {
    const termElem = document.getElementById("terminal");
    if (termElem) {
        const div = document.createElement("div");
        div.textContent = message;
        termElem.appendChild(div);
        termElem.scrollTop = termElem.scrollHeight;
    } else {
        // หากไม่มีหน้าจอ Terminal บนหน้า HTML นั้น ให้บันทึกความเคลื่อนไหวลงใน Console แทน
        console.log("Terminal Log:", message);
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
    fetch("/terminalSet", requestOptions)
        .then((response) => response.text())
        .then((result) => console.log("Respond:", result))
        .catch((error) => console.error("Error:", error));
}