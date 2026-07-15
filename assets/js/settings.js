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

const timerFormatInput = document.getElementById("timerFormatInput");
const resetTimerFormat = document.getElementById("resetTimerFormat");

const pinToggle = document.getElementById("pinToggle");
const fontToggle = document.getElementById("fontToggle");
const textColorPicker = document.getElementById("textColorPicker");
const resetTextColor = document.getElementById("resetTextColor");

let currentTextScale = 1;
let currentScrambleFrames = 20;
let currentScrambleDelay = 20;
let currentRevealDelay = 20;
let currentTimerPosition = 0;
let currentTimerScale = 0.5;
let currentBleRequirePin = false;
let currentUseProportionalFont = false;
let currentTextColor = 48991;
let currentTimerFormatLong = false;

function hexToRgb565(hex) {
    hex = hex.replace('#', '');
    let r = parseInt(hex.substring(0, 2), 16) >> 3;
    let g = parseInt(hex.substring(2, 4), 16) >> 2;
    let b = parseInt(hex.substring(4, 6), 16) >> 3;
    return (r << 11) | (g << 5) | b;
}

function rgb565ToHex(rgb565) {
    let r = ((rgb565 >> 11) & 0x1F) << 3;
    let g = ((rgb565 >> 5) & 0x3F) << 2;
    let b = (rgb565 & 0x1F) << 3;
    r = r | (r >> 5);
    g = g | (g >> 6);
    b = b | (b >> 5);
    return '#' + (r.toString(16).padStart(2, '0') + g.toString(16).padStart(2, '0') + b.toString(16).padStart(2, '0')).toLowerCase();
}

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

if (resetTextScale) resetTextScale.addEventListener("click", () => textScaleInput.value = 1);
if (resetScrambleFrames) resetScrambleFrames.addEventListener("click", () => scrambleFramesInput.value = 20);
if (resetScrambleDelay) resetScrambleDelay.addEventListener("click", () => scrambleDelayInput.value = 20);
if (resetRevealDelay) resetRevealDelay.addEventListener("click", () => revealDelayInput.value = 20);
if (resetTimerPosition) resetTimerPosition.addEventListener("click", () => timerPositionInput.value = 0);
if (resetTimerScale) resetTimerScale.addEventListener("click", () => timerScaleInput.value = 5);
if (resetTimerFormat) resetTimerFormat.addEventListener("click", () => timerFormatInput.value = 0);
if (resetTextColor) resetTextColor.addEventListener("click", () => textColorPicker.value = "#b8e8f8");

function applySettings() {
    currentTextScale = parseInt(textScaleInput.value);
    currentScrambleFrames = parseInt(scrambleFramesInput.value);
    currentScrambleDelay = parseInt(scrambleDelayInput.value);
    currentRevealDelay = parseInt(revealDelayInput.value);
    currentTimerPosition = parseInt(timerPositionInput.value);
    currentTimerScale = parseFloat(timerScaleInput.value) / 10.0;
    currentTimerFormatLong = parseInt(timerFormatInput.value) !== 0;

    if (isNaN(currentTextScale) || currentTextScale < 1) currentTextScale = 1;
    if (isNaN(currentScrambleFrames) || currentScrambleFrames < 1) currentScrambleFrames = 20;
    if (isNaN(currentScrambleDelay) || currentScrambleDelay < 1) currentScrambleDelay = 5;
    if (isNaN(currentRevealDelay) || currentRevealDelay < 1) currentRevealDelay = 15;
    if (isNaN(currentTimerPosition) || currentTimerPosition < 0 || currentTimerPosition > 6) currentTimerPosition = 0;
    if (isNaN(currentTimerScale) || currentTimerScale < 0.1) currentTimerScale = 0.5;

    currentBleRequirePin = pinToggle ? pinToggle.checked : false;
    currentUseProportionalFont = fontToggle ? fontToggle.checked : false;
    currentTextColor = textColorPicker ? hexToRgb565(textColorPicker.value) : 48991;

    if (typeof txCharacteristic !== 'undefined' && txCharacteristic) {
        let payload = `${currentTextScale},${currentScrambleFrames},${currentScrambleDelay},${currentRevealDelay},${currentTimerPosition},${currentTimerScale},${currentBleRequirePin ? 1 : 0},${currentUseProportionalFont ? 1 : 0},${currentTextColor},${currentTimerFormatLong ? 1 : 0}`;
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
            if (parts.length >= 8) {
                currentUseProportionalFont = parseInt(parts[7]) !== 0;
                if (parts.length >= 9) {
                    currentTextColor = parseInt(parts[8]);
                    if (parts.length >= 10) {
                        currentTimerFormatLong = parseInt(parts[9]) !== 0;
                    } else {
                        currentTimerFormatLong = false;
                    }
                } else {
                    currentTextColor = 48991;
                    currentTimerFormatLong = false;
                }
            } else {
                currentUseProportionalFont = false;
                currentTextColor = 48991;
                currentTimerFormatLong = false;
            }
        } else {
            currentBleRequirePin = false;
            currentUseProportionalFont = false;
            currentTextColor = 48991;
            currentTimerFormatLong = false;
        }

        if (isNaN(currentTextScale) || currentTextScale < 1) currentTextScale = 1;
        if (isNaN(currentScrambleFrames) || currentScrambleFrames < 1) currentScrambleFrames = 20;
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
        if (timerFormatInput) timerFormatInput.value = currentTimerFormatLong ? 1 : 0;

        if (pinToggle) {
            pinToggle.checked = currentBleRequirePin;
        }
        if (fontToggle) {
            fontToggle.checked = currentUseProportionalFont;
        }
        if (textColorPicker) {
            textColorPicker.value = rgb565ToHex(currentTextColor);
        }
    }
}
