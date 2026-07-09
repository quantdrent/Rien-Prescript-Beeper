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


let currentTextScale = 2;
let currentScrambleFrames = 30;
let currentScrambleDelay = 5;
let currentRevealDelay = 15;

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

saveSettingsBtn.addEventListener("click", () => {
    const newScale = parseInt(textScaleInput.value) || 2;
    const newSFrames = parseInt(scrambleFramesInput.value) || 60;
    const newSDelay = parseInt(scrambleDelayInput.value) || 5;
    const newRDelay = parseInt(revealDelayInput.value) || 10;

    currentTextScale = newScale;
    currentScrambleFrames = newSFrames;
    currentScrambleDelay = newSDelay;
    currentRevealDelay = newRDelay;

    if (typeof sendBleCommand === "function") {
        sendBleCommand("SET_SETTINGS", `${currentTextScale},${currentScrambleFrames},${currentScrambleDelay},${currentRevealDelay}`);
    }
    settingsModal.style.display = "none";
});

function updateSettingsUI(csvData) {
    let parts = csvData.split(",");
    
    currentTextScale = parseInt(parts[0]) || 2;
    if (textScaleInput) textScaleInput.value = currentTextScale;

    if (parts.length > 1) {
        currentScrambleFrames = parseInt(parts[1]) || 60;
        if (scrambleFramesInput) scrambleFramesInput.value = currentScrambleFrames;
    }
    if (parts.length > 2) {
        currentScrambleDelay = parseInt(parts[2]) || 5;
        if (scrambleDelayInput) scrambleDelayInput.value = currentScrambleDelay;
    }
    if (parts.length > 3) {
        currentRevealDelay = parseInt(parts[3]) || 10;
        if (revealDelayInput) revealDelayInput.value = currentRevealDelay;
    }
}
