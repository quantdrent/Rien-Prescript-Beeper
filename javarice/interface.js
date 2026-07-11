let currentTimer = null;

function updateCounters() {
    achievedSpan.textContent = `PASS: ${achieved}`;
    failedSpan.textContent = `FAILED: ${failed}`;
    totalSpan.textContent = `PRESCRIPTS: ${total}`;
}


function showPlayButton() {
    buttonContainer.innerHTML = `<button id="startBtn">Next</button>`;
    document.getElementById("startBtn").addEventListener("click", handlePlayClick);
}

function showResultText(text) {
    canResolve = false;
    scrambleReveal(text, 0.3, 0.8, t => display.textContent = t, showPlayButton);
}

function showResultTextIntro(text) {
    scrambleReveal(text, 0.3, 0.8,t => display.textContent = t,);
}

async function triggerPass() {
    if (!canResolve) return;
    if (currentTimer) clearInterval(currentTimer);
    
    showResultText("CLEAR.");
    if (typeof sendBleCommand === 'function') {
        await sendBleCommand("PASS");
    }
    
    if (typeof txCharacteristic !== 'undefined' && txCharacteristic) {
        setTimeout(() => {
            sendBleCommand("ADD_STATS", "1,0,1").catch(console.error);
        }, 1500);
    } else {
        achieved++; total++;
        updateCounters();
    }
}

async function triggerFail() {
    if (!canResolve) return;
    if (currentTimer) clearInterval(currentTimer);
    
    showResultText("FAILED.");
    if (typeof sendBleCommand === 'function') {
        await sendBleCommand("FAIL");
    }
    
    if (typeof txCharacteristic !== 'undefined' && txCharacteristic) {
        setTimeout(() => {
            sendBleCommand("ADD_STATS", "0,1,1").catch(console.error);
        }, 1500);
    } else {
        failed++; total++;
        updateCounters();
    }
}

function showResultButtons(duration) {
    canResolve = false; 

    let timeLeft = duration || 10;

    buttonContainer.innerHTML = `
        <button id="achievedBtn" disabled>Pass</button>
        <div id="timerDisplay">${timeLeft}s</div>
        <button id="failedBtn" disabled>Failed</button>
    `;

    document.getElementById("achievedBtn").onclick = triggerPass;
    document.getElementById("failedBtn").onclick = triggerFail;

    if (currentTimer) clearInterval(currentTimer);
    
    currentTimer = setInterval(() => {
        if (!canResolve) return;
        
        if (timeLeft > 0) {
            timeLeft--;
            const timerEl = document.getElementById("timerDisplay");
            if (timerEl) timerEl.textContent = `${timeLeft}s`;
        } else {
            clearInterval(currentTimer);
            if (typeof triggerFail === 'function') {
                triggerFail();
            }
        }
    }, 1000);
}

async function handlePlayClick() {
    if (typeof txCharacteristic === 'undefined' || !txCharacteristic) {
        let line = "";
        let dur = 10;
        if (customPrescripts.length > 0) {
            line = customPrescripts[Math.floor(Math.random() * customPrescripts.length)];
        } else if (defaultPrescripts.length > 0) {
            line = defaultPrescripts[Math.floor(Math.random() * defaultPrescripts.length)];
        } else {
            line = "10|NO DATA";
        }
        
        let parts = line.split("|");
        if (parts.length >= 2) {
            dur = parseInt(parts[0]);
            if (isNaN(dur)) dur = 10;
            display.textContent = parts[1];
        } else {
            display.textContent = line;
        }
        
        showResultButtons(dur);
        canResolve = true;
        document.getElementById("achievedBtn").disabled = false;
        document.getElementById("failedBtn").disabled = false;
    } else {
        sendBleCommand("NEXT");
    }
}
