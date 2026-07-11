const UART_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
const UART_RX_CHARACTERISTIC_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
const UART_TX_CHARACTERISTIC_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

let bleDevice = null;
let bleServer = null;
let uartService = null;
let rxCharacteristic = null;
let txCharacteristic = null;

let incomingBuffer = "";

async function connectToBeeper() {
    if (bleDevice && rxCharacteristic) {
        try {
            bleDevice.gatt.disconnect();
        } catch (e) {
            console.warn("Disconnect error:", e);
            onDisconnected();
        }
        return;
    }
    const statusEl = document.getElementById("deviceStatus");
    try {
        if (statusEl) statusEl.textContent = "_Searching..._";
        if (statusEl) statusEl.style.color = "#ffff99";

        console.log("Requesting Bluetooth Device...");
        bleDevice = await navigator.bluetooth.requestDevice({
            filters: [{ name: "Beeper" }, { services: [UART_SERVICE_UUID] }],
            optionalServices: [UART_SERVICE_UUID]
        });

        if (statusEl) statusEl.textContent = "_Connecting..._";
        bleDevice.addEventListener('gattserverdisconnected', onDisconnected);

        // Retry GATT connection up to 3 times with delays
        let connected = false;
        for (let attempt = 1; attempt <= 3; attempt++) {
            try {
                console.log(`GATT connect attempt ${attempt}...`);
                bleServer = await bleDevice.gatt.connect();
                await new Promise(r => setTimeout(r, 300));

                console.log("Getting UART Service...");
                uartService = await bleServer.getPrimaryService(UART_SERVICE_UUID);
                await new Promise(r => setTimeout(r, 100));

                console.log("Getting RX Characteristic...");
                rxCharacteristic = await uartService.getCharacteristic(UART_RX_CHARACTERISTIC_UUID);
                await new Promise(r => setTimeout(r, 100));

                console.log("Getting TX Characteristic...");
                txCharacteristic = await uartService.getCharacteristic(UART_TX_CHARACTERISTIC_UUID);
                await txCharacteristic.startNotifications();
                txCharacteristic.addEventListener('characteristicvaluechanged', handleIncomingBLEData);

                connected = true;
                break;
            } catch (retryError) {
                console.warn(`Attempt ${attempt} failed:`, retryError.message);
                if (attempt < 3) {
                    if (statusEl) statusEl.textContent = `_Retrying (${attempt}/3)..._`;
                    await new Promise(r => setTimeout(r, 500));
                    try { bleServer = await bleDevice.gatt.connect(); } catch(e) {}
                } else {
                    throw retryError;
                }
            }
        }

        console.log("Connected to Beeper successfully!");
        document.getElementById('bleConnectBtn').classList.add("connected");
        document.getElementById('bleConnectBtn').textContent = "_Disconnect_";
        document.getElementById('factoryResetBtn').removeAttribute('disabled');
        document.getElementById('customsBtn').removeAttribute('disabled');
        if (statusEl) statusEl.textContent = "_Authenticating..._";
        
        // Just send a ping command first to see if auth is required
        await sendBleCommand("GET_SETTINGS");
        
        return true;
    } catch (error) {
        console.error("Connection failed!", error);
        bleDevice = null;
        bleServer = null;
        rxCharacteristic = null;
        txCharacteristic = null;
        if (statusEl) {
            statusEl.textContent = "_Disconnected_";
            statusEl.style.color = "#ff6b6b";
        }
        alert("Failed to connect to Beeper. Try again.");
        return false;
    }
}

function onDisconnected() {
    console.log("Device disconnected.");
    document.getElementById('bleConnectBtn').classList.remove("connected");
    document.getElementById('bleConnectBtn').textContent = "_Pair Device_";
    document.getElementById('factoryResetBtn').setAttribute('disabled', 'true');
    const statusEl = document.getElementById("deviceStatus");
    if (statusEl) {
        statusEl.textContent = "_Disconnected_";
        statusEl.style.color = "#ff6b6b";
    }
    
    achieved = 0;
    failed = 0;
    total = 0;
    if (typeof updateCounters === 'function') updateCounters();
    
    bleDevice = null;
    bleServer = null;
    uartService = null;
    rxCharacteristic = null;
    txCharacteristic = null;
}

function handleIncomingBLEData(event) {
    let value = event.target.value;
    let decoder = new TextDecoder('utf-8');
    let str = decoder.decode(value);
    incomingBuffer += str;
    let newlineIndex = incomingBuffer.indexOf('\n');
    while (newlineIndex !== -1) {
        let message = incomingBuffer.substring(0, newlineIndex).trim();
        incomingBuffer = incomingBuffer.substring(newlineIndex + 1);
        
        if (message.length > 0) {
            parseBleMessage(message);
        }
        
        newlineIndex = incomingBuffer.indexOf('\n');
    }
}

function parseBleMessage(message) {
    console.log("Received from device:", message);
    
    if (message === "RES:AUTH_REQUIRED") {
        document.getElementById("authModal").style.display = "flex";
        document.getElementById("authPinInput").value = "";
        document.getElementById("authPinInput").focus();
        return;
    } else if (message === "RES:AUTH_OK") {
        document.getElementById("authModal").style.display = "none";
        const statusEl = document.getElementById("deviceStatus");
        if (statusEl) {
            statusEl.textContent = "_Connected_";
            statusEl.style.color = "#99ff99";
        }
        if (typeof showResultTextIntro === 'function') {
            showResultTextIntro("Connected.");
        }
        sendBleCommand("GET_CUSTOMS");
        sendBleCommand("GET_STATS");
        return;
    } else if (message === "RES:AUTH_FAIL") {
        document.getElementById("authModal").style.display = "none";
        alert("Incorrect PIN!");
        onDisconnected();
        return;
    }

    if (message === "EVT:PASS") {
        if (typeof triggerPass === 'function') triggerPass(true);
    } else if (message === "EVT:FAIL") {
        if (typeof triggerFail === 'function') triggerFail(true);
    } else if (message.startsWith("EVT:PRESCRIPT|")) {
        let idxMatch = message.match(/IDX:(\d+)/);
        if (idxMatch) {
            let idx = parseInt(idxMatch[1]);
            if (typeof customPrescripts !== 'undefined' && idx >= 0 && idx < customPrescripts.length) {
                let prescript = customPrescripts[idx];
                let dur = prescript.duration;
                let text = prescript.text;
                let respond = prescript.respond !== false;
                if (typeof scrambleReveal === 'function') {
                    scrambleReveal(text, scrambleDuration, revealDuration, t => display.textContent = t);
                }
                if (typeof showResultButtons === 'function') {
                    showResultButtons(dur, respond);
                    canResolve = true;
                    if (respond) {
                        document.getElementById("achievedBtn").disabled = false;
                        document.getElementById("failedBtn").disabled = false;
                    }
                }
            } else {
                console.warn("Received prescript index out of bounds or customPrescripts not loaded.");
            }
        }
    } else if (message.startsWith("RES:CUSTOMS|")) {
        try {
            let jsonStr = message.split("|MSG:")[1];
            let customs = JSON.parse(jsonStr);
            if (typeof handleDeviceCustomsSync === 'function') {
                handleDeviceCustomsSync(customs);
            } else if (typeof updateCustomPrescriptsList === 'function') {
                updateCustomPrescriptsList(customs);
            }
        } catch (e) {
            console.error("Failed to parse custom prescripts JSON", e);
        }
    } else if (message.startsWith("RES:STATS|")) {
        try {
            let parts = message.split("|MSG:")[1].split(",");
            if (parts.length === 3) {
                if (typeof achieved !== 'undefined') {
                    achieved = parseInt(parts[0]);
                    failed = parseInt(parts[1]);
                    total = parseInt(parts[2]);
                    
                    if (typeof updateCounters === 'function') updateCounters();
                }
            }
        } catch (e) {
            console.error("Failed to parse stats", e);
        }
    } else if (message.startsWith("RES:SETTINGS|")) {
        try {
            const statusEl = document.getElementById("deviceStatus");
            if (statusEl && statusEl.textContent === "_Authenticating..._") {
                statusEl.textContent = "_Connected_";
                statusEl.style.color = "#99ff99";
                if (typeof showResultTextIntro === 'function') {
                    showResultTextIntro("Connected.");
                }
                sendBleCommand("GET_CUSTOMS");
                sendBleCommand("GET_STATS");
            }
            
            let sleepVal = message.split("|MSG:")[1];
            if (typeof updateSettingsUI === 'function') {
                updateSettingsUI(sleepVal);
            }
        } catch (e) {
            console.error("Failed to parse settings", e);
        }
    }
}

async function sendBleMessage(text, duration = "10", respond = true) {
    if (!rxCharacteristic) {
        console.log("Cannot send message. BLE device not connected.");
        return;
    }

    try {
        const fullMessage = `CMD:SHOW|DUR:${duration}|RES:${respond ? 1 : 0}|MSG:${text}\n`;
        
        let encoder = new TextEncoder('utf-8');
        let messageArray = encoder.encode(fullMessage);

        const CHUNK_SIZE = 20;
        for (let i = 0; i < messageArray.length; i += CHUNK_SIZE) {
            let chunk = messageArray.slice(i, i + CHUNK_SIZE);
            await rxCharacteristic.writeValue(chunk);
            await new Promise(r => setTimeout(r, 10)); 
        }

        console.log("Message sent to Beeper: " + fullMessage.trim());
    } catch (error) {
        console.error("Failed to send message", error);
    }
}

async function sendBleCommand(cmd, msg = "") {
    if (!rxCharacteristic) return;
    try {
        let fullMessage = `CMD:${cmd}`;
        if (msg) fullMessage += `|MSG:${msg}`;
        fullMessage += "\n";
        
        let encoder = new TextEncoder('utf-8');
        let messageArray = encoder.encode(fullMessage);

        const CHUNK_SIZE = 20;
        for (let i = 0; i < messageArray.length; i += CHUNK_SIZE) {
            let chunk = messageArray.slice(i, i + CHUNK_SIZE);
            await rxCharacteristic.writeValue(chunk);
            await new Promise(r => setTimeout(r, 10)); 
        }
    } catch (error) {
        console.error("Failed to send command", error);
    }
}

document.addEventListener('DOMContentLoaded', () => {
    let connectBtn = document.getElementById('bleConnectBtn');
    if (connectBtn) {
        connectBtn.addEventListener('click', connectToBeeper);
    }

    let authSubmitBtn = document.getElementById('authSubmitBtn');
    let authPinInput = document.getElementById('authPinInput');
    if (authSubmitBtn) {
        authSubmitBtn.addEventListener('click', () => {
            let pin = authPinInput.value;
            if (pin.length > 0) {
                sendBleCommand("AUTH", pin);
            }
        });
    }
    if (authPinInput) {
        authPinInput.addEventListener('keydown', (e) => {
            if (e.key === 'Enter' && authPinInput.value.length > 0) {
                sendBleCommand("AUTH", authPinInput.value);
            }
        });
    }

    const resetBtn = document.getElementById('factoryResetBtn');
    if (resetBtn) {
        resetBtn.addEventListener('click', () => {
            if (typeof txCharacteristic === 'undefined' || !txCharacteristic) return;
            if (confirm("Are you sure you want to completely factory reset the device? This will restore default Prescripts and reset stats.")) {
                sendBleCommand("FACTORY_RESET");

                setTimeout(() => {
                    if (typeof defaultPrescriptsList !== 'undefined') {
                        if (typeof updateCustomPrescriptsList === 'function') {
                            updateCustomPrescriptsList([...defaultPrescriptsList]);
                        }
                        if (typeof syncCustomsToDevice === 'function') {
                            syncCustomsToDevice().then(() => {
                                sendBleCommand("GET_CUSTOMS");
                                sendBleCommand("GET_STATS");
                                sendBleCommand("GET_SETTINGS");
                            });
                        }
                    } else {
                        sendBleCommand("GET_CUSTOMS");
                        sendBleCommand("GET_STATS");
                        sendBleCommand("GET_SETTINGS");
                    }
                }, 1500);
            }
        });
    }
});
