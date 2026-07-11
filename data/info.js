var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getReadings() {
    websocket.send("getReadings");
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

            // 🔹 ลบอักขระควบคุมที่ไม่ใช่ printable ASCII (0x20-0x7E)
            const jsonText = rawText.replace(/[\x00-\x1F\x7F]/g, "");

            const obj = JSON.parse(jsonText);

            console.log("Decoded JSON:", obj);

            if (obj["Serial"]) {
                console.log(`Serial : ${obj["Serial"]}`);
                appendToTerminal(`${ts} Serial : ${obj["Serial"]}`);
            }
            if (obj["Inverter"]) {
                console.log(`Inverter: ${obj["Inverter"]}`);
                appendToTerminal(`${ts} Inverter : ${obj["Inverter"]}`);
            }

        } catch (err) {
            console.error("Decode error:", err, event.data);
        }
    };

}

document.getElementById("sendBtn").addEventListener("click", () => {
    const ts = new Date().toLocaleTimeString();
    const msg = document.getElementById("messageInput").value.trim();
    if (msg) {
        fetchToserver(msg);
        appendToTerminal(`${ts} Sent : ${msg}`);
        document.getElementById("messageInput").value = "";
    }
});

document.getElementById("clearBtn").addEventListener("click", () => {
    terminal.innerHTML = "";
});

messageInput.addEventListener("keydown", (e) => {
    const ts = new Date().toLocaleTimeString();
    const msg = document.getElementById("messageInput").value.trim();
    if (e.key === 'Enter') {
        if (msg) {
            fetchToserver(msg);
            appendToTerminal(`${ts} Sent : ${msg}`);
            document.getElementById("messageInput").value = "";
        }
    }
});

function appendToTerminal(message) {
    const div = document.createElement("div");
    div.textContent = message;
    terminal.appendChild(div);
    terminal.scrollTop = terminal.scrollHeight;
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
