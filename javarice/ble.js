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
            filters: [{ services: [UART_SERVICE_UUID] }]
        });

        if (statusEl) statusEl.textContent = "_Connecting..._";
        bleDevice.addEventListener('gattserverdisconnected', onDisconnected);

        console.log("Connecting to GATT Server...");
        bleServer = await bleDevice.gatt.connect();

        console.log("Getting UART Service...");
        uartService = await bleServer.getPrimaryService(UART_SERVICE_UUID);

        console.log("Getting RX Characteristic...");
        rxCharacteristic = await uartService.getCharacteristic(UART_RX_CHARACTERISTIC_UUID);
        
        console.log("Getting TX Characteristic...");
        txCharacteristic = await uartService.getCharacteristic(UART_TX_CHARACTERISTIC_UUID);
        await txCharacteristic.startNotifications();
        txCharacteristic.addEventListener('characteristicvaluechanged', handleIncomingBLEData);

        console.log("Connected to Beeper successfully!");
        document.getElementById('bleConnectBtn').classList.add("connected");
        document.getElementById('bleConnectBtn').textContent = "_Disconnect_";
        document.getElementById('factoryResetBtn').removeAttribute('disabled');
        document.getElementById('settingsBtn').removeAttribute('disabled');
        if (statusEl) statusEl.textContent = "_Loading data..._";
        await sendBleCommand("GET_CUSTOMS");
        await sendBleCommand("GET_STATS");
        await sendBleCommand("GET_SETTINGS");
        
        if (statusEl) {
            statusEl.textContent = "_Connected_";
            statusEl.style.color = "#99ff99";
        }
        
        if (typeof showResultTextIntro === 'function') {
            showResultTextIntro("Connected.");
        }
        
        return true;
    } catch (error) {
        console.error("Connection failed!", error);
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
    document.getElementById('settingsBtn').setAttribute('disabled', 'true');
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
        isSendingBle = false;
        if (bleCommandQueue.length > 0) {
            setTimeout(processBleQueue, 0); 
        }
        
        if (message.length > 0) {
            parseBleMessage(message);
        }
        
        newlineIndex = incomingBuffer.indexOf('\n');
    }
}

function parseBleMessage(message) {
    console.log("Received from device:", message);
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
                if (typeof scrambleReveal === 'function') {
                    scrambleReveal(text, 0.1, 0.2, t => display.textContent = t);
                }
                if (typeof showResultButtons === 'function') {
                    showResultButtons(dur);
                    canResolve = true;
                    document.getElementById("achievedBtn").disabled = false;
                    document.getElementById("failedBtn").disabled = false;
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
            let sleepVal = message.split("|MSG:")[1];
            if (typeof updateSettingsUI === 'function') {
                updateSettingsUI(sleepVal);
            }
        } catch (e) {
            console.error("Failed to parse settings", e);
        }
    }
}

async function sendBleMessage(text, duration = "10") {
    if (!rxCharacteristic) {
        console.log("Cannot send message. BLE device not connected.");
        return;
    }

    try {
        const fullMessage = `CMD:SHOW|DUR:${duration}|MSG:${text}\n`;
        
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

document.addEventListener("DOMContentLoaded", () => {
    const connectBtn = document.getElementById('bleConnectBtn');
    if (connectBtn) {
        connectBtn.addEventListener('click', connectToBeeper);
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
