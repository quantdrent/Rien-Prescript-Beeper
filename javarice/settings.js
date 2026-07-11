const settingsBtn = document.getElementById("settingsBtn");
const settingsModal = document.getElementById("settingsModal");
const closeSettings = document.getElementById("closeSettings");
const saveSettingsBtn = document.getElementById("saveSettingsBtn");

const textScaleInput = document.getElementById("textScaleInput");
const resetTextScale = document.getElementById("resetTextScale");

const scrambleFramesInput = document.getElementById("scrambleFramesInput");
const resetScrambleFrames = document.getElementById("resetScrambleFrames");

const scrambleDelayInput = document.getElementById("scrambleDelayInput");
const resetScrambleDelay = document.getElementById("resetScrambleDelay");

const revealDelayInput = document.getElementById("revealDelayInput");
const resetRevealDelay = document.getElementById("resetRevealDelay");

const timerPositionInput = document.getElementById("timerPositionInput");
const resetTimerPosition = document.getElementById("resetTimerPosition");

const timerScaleInput = document.getElementById("timerScaleInput");
const resetTimerScale = document.getElementById("resetTimerScale");

const pinToggle = document.getElementById("pinToggle");

let currentTextScale = 2;
let currentScrambleFrames = 30;
let currentScrambleDelay = 5;
let currentRevealDelay = 15;
let currentTimerPosition = 0;
let currentTimerScale = 0.5;
let currentBleRequirePin = false;

settingsBtn.addEventListener("click", () => {
    settingsModal.style.display = "block";
});

closeSettings.addEventListener("click", () => {
    settingsModal.style.display = "none";
});

window.addEventListener("click", (event) => {
    if (event.target == settingsModal) {
        settingsModal.style.display = "none";
    }
});

if (resetTextScale) resetTextScale.addEventListener("click", () => textScaleInput.value = 2);
if (resetScrambleFrames) resetScrambleFrames.addEventListener("click", () => scrambleFramesInput.value = 30);
if (resetScrambleDelay) resetScrambleDelay.addEventListener("click", () => scrambleDelayInput.value = 5);
if (resetRevealDelay) resetRevealDelay.addEventListener("click", () => revealDelayInput.value = 15);
if (resetTimerPosition) resetTimerPosition.addEventListener("click", () => timerPositionInput.value = 0);
if (resetTimerScale) resetTimerScale.addEventListener("click", () => timerScaleInput.value = 5);

function applySettings() {
    currentTextScale = parseInt(textScaleInput.value);
    currentScrambleFrames = parseInt(scrambleFramesInput.value);
    currentScrambleDelay = parseInt(scrambleDelayInput.value);
    currentRevealDelay = parseInt(revealDelayInput.value);
    currentTimerPosition = parseInt(timerPositionInput.value);
    currentTimerScale = parseFloat(timerScaleInput.value) / 10.0;
    
    if (isNaN(currentTextScale) || currentTextScale < 1) currentTextScale = 2;
    if (isNaN(currentScrambleFrames) || currentScrambleFrames < 1) currentScrambleFrames = 30;
    if (isNaN(currentScrambleDelay) || currentScrambleDelay < 1) currentScrambleDelay = 5;
    if (isNaN(currentRevealDelay) || currentRevealDelay < 1) currentRevealDelay = 15;
    if (isNaN(currentTimerPosition) || currentTimerPosition < 0 || currentTimerPosition > 6) currentTimerPosition = 0;
    if (isNaN(currentTimerScale) || currentTimerScale < 0.1) currentTimerScale = 0.5;
    
    currentBleRequirePin = pinToggle ? pinToggle.checked : false;
    
    if (typeof txCharacteristic !== 'undefined' && txCharacteristic) {
        let payload = `${currentTextScale},${currentScrambleFrames},${currentScrambleDelay},${currentRevealDelay},${currentTimerPosition},${currentTimerScale},${currentBleRequirePin ? 1 : 0}`;
        sendBleCommand("SET_SETTINGS", payload);
    }
    settingsModal.style.display = "none";
}

saveSettingsBtn.addEventListener("click", applySettings);

function updateSettingsUI(csvData) {
    let parts = csvData.split(",");
    if (parts.length >= 4) {
        currentTextScale = parseInt(parts[0]);
        currentScrambleFrames = parseInt(parts[1]);
        currentScrambleDelay = parseInt(parts[2]);
        currentRevealDelay = parseInt(parts[3]);
        
        if (parts.length >= 6) {
            currentTimerPosition = parseInt(parts[4]);
            currentTimerScale = parseFloat(parts[5]);
        }
        
        if (parts.length >= 7) {
            currentBleRequirePin = parseInt(parts[6]) !== 0;
        } else {
            currentBleRequirePin = false;
        }
        
        if (isNaN(currentTextScale) || currentTextScale < 1) currentTextScale = 2;
        if (isNaN(currentScrambleFrames) || currentScrambleFrames < 1) currentScrambleFrames = 30;
        if (isNaN(currentScrambleDelay) || currentScrambleDelay < 1) currentScrambleDelay = 5;
        if (isNaN(currentRevealDelay) || currentRevealDelay < 1) currentRevealDelay = 15;
        if (isNaN(currentTimerPosition) || currentTimerPosition < 0 || currentTimerPosition > 6) currentTimerPosition = 0;
        if (isNaN(currentTimerScale) || currentTimerScale < 0.1) currentTimerScale = 0.5;
        
        textScaleInput.value = currentTextScale;
        scrambleFramesInput.value = currentScrambleFrames;
        scrambleDelayInput.value = currentScrambleDelay;
        revealDelayInput.value = currentRevealDelay;
        timerPositionInput.value = currentTimerPosition;
        timerScaleInput.value = (currentTimerScale * 10).toString();
        
        if (pinToggle) {
            pinToggle.checked = currentBleRequirePin;
        }
    }
}
