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
    scrambleReveal(text, 0.3, 0.8, t => display.textContent = t);
}

function showResultTextIntro(text) {
    scrambleReveal(text, 0.3, 0.8,t => display.textContent = t,);
}

async function triggerPass(fromDevice = false) {
    if (!canResolve) return;
    canResolve = false;
    if (currentTimer) clearInterval(currentTimer);
    
    showResultText("CLEAR.");
    showPlayButton();
    if (typeof sendBleCommand === 'function' && fromDevice !== true) {
        await sendBleCommand("PASS");
    }
    
    if (fromDevice !== true) {
        achieved++; total++;
        updateCounters();
    }
}

async function triggerFail(fromDevice = false) {
    if (!canResolve) return;
    canResolve = false;
    if (currentTimer) clearInterval(currentTimer);
    
    showResultText("FAILED.");
    showPlayButton();
    if (typeof sendBleCommand === 'function' && fromDevice !== true) {
        await sendBleCommand("FAIL");
    }
    
    if (fromDevice !== true) {
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
    let line = "";
    let dur = 10;
    let idx = -1;
    if (typeof customPrescripts !== 'undefined' && customPrescripts.length > 0) {
        idx = Math.floor(Math.random() * customPrescripts.length);
        line = customPrescripts[idx];
    } else if (typeof defaultPrescripts !== 'undefined' && defaultPrescripts.length > 0) {
        idx = Math.floor(Math.random() * defaultPrescripts.length);
        line = defaultPrescripts[idx];
    } else {
        line = "10|NO DATA";
    }
    
    let parts = line.split("|");
    let text = line;
    if (parts.length >= 2) {
        dur = parseInt(parts[0]);
        if (isNaN(dur)) dur = 10;
        text = parts[1];
    }
    
    if (typeof txCharacteristic !== 'undefined' && txCharacteristic && idx !== -1) {
        sendBleCommand("SHOW_IDX:" + idx);
    }
    
    if (typeof scrambleReveal === 'function') {
        scrambleReveal(text, 0.1, 0.2, t => display.textContent = t);
    } else {
        display.textContent = text;
    }
    
    showResultButtons(dur);
    canResolve = true;
    document.getElementById("achievedBtn").disabled = false;
    document.getElementById("failedBtn").disabled = false;
}
