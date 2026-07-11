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
        achieved++;
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
        failed++;
        updateCounters();
    }
}

function formatTime(totalSeconds) {
    if (totalSeconds < 60) return `${totalSeconds}s`;
    const h = Math.floor(totalSeconds / 3600);
    const m = Math.floor((totalSeconds % 3600) / 60);
    const s = totalSeconds % 60;
    let parts = [];
    if (h > 0) parts.push(`${h}h`);
    if (m > 0 || (h > 0 && s > 0)) parts.push(`${m}m`); // keep '0m' if hours exist and seconds exist? Or just omit if 0? "4h 20s" is fine. User asked "if has hours tthen 4h 3m 20s". Let's just omit 0 values.
    if (s > 0) parts.push(`${s}s`);
    return parts.join(" ");
}

function showResultButtons(duration, respond = true) {
    canResolve = false; 

    let timeLeft = duration || 10;

    if (respond) {
        buttonContainer.innerHTML = `
            <button id="achievedBtn" disabled>Pass</button>
            <div id="timerDisplay">${formatTime(timeLeft)}</div>
            <button id="failedBtn" disabled>Failed</button>
        `;
        document.getElementById("achievedBtn").onclick = triggerPass;
        document.getElementById("failedBtn").onclick = triggerFail;
    } else {
        buttonContainer.innerHTML = `
            <div id="timerDisplay" style="font-size: 1.5em; padding: 10px;">${formatTime(timeLeft)}</div>
        `;
    }

    if (currentTimer) clearInterval(currentTimer);
    
    currentTimer = setInterval(() => {
        if (!canResolve) return;
        
        if (timeLeft > 0) {
            timeLeft--;
            const timerEl = document.getElementById("timerDisplay");
            if (timerEl) timerEl.textContent = formatTime(timeLeft);
        } else {
            clearInterval(currentTimer);
            if (respond) {
                if (typeof triggerFail === 'function') {
                    triggerFail();
                }
            } else {
                showResultTextIntro("CLEAR.");
                showPlayButton();
            }
        }
    }, 1000);
}

async function handlePlayClick() {
    let dur = 10;
    let text = "NO DATA";
    let respond = true;
    let idx = -1;
    
    if (typeof pickMessage === 'function') {
        let picked = pickMessage();
        if (picked) {
            dur = picked.duration || 10;
            text = picked.text;
            respond = picked.respond !== false;
            
            if (typeof customPrescripts !== 'undefined') {
                idx = customPrescripts.findIndex(p => p.text === text && p.duration === dur);
            }
        }
    }
    
    if (typeof txCharacteristic !== 'undefined' && txCharacteristic && idx !== -1) {
        sendBleCommand("SHOW_IDX:" + idx);
    }
    
    if (typeof scrambleReveal === 'function') {
        scrambleReveal(text, scrambleDuration, revealDuration, t => display.textContent = t);
    } else {
        display.textContent = text;
    }
    
    showResultButtons(dur, respond);
    canResolve = true;
    if (respond) {
        document.getElementById("achievedBtn").disabled = false;
        document.getElementById("failedBtn").disabled = false;
    }
    
    total++;
    updateCounters();
}

function triggerManualPrescript(text, duration = 10, respond = true) {
    if (typeof sendBleMessage === 'function') {
        sendBleMessage(text, duration, respond);
    }
    
    if (typeof scrambleReveal === 'function') {
        scrambleReveal(text, scrambleDuration, revealDuration, t => display.textContent = t);
    } else {
        display.textContent = text;
    }
    
    showResultButtons(duration, respond);
    canResolve = true;
    if (respond) {
        document.getElementById("achievedBtn").disabled = false;
        document.getElementById("failedBtn").disabled = false;
    }
    
    total++;
    updateCounters();
}

if (startBtn) {
    startBtn.addEventListener("click", handlePlayClick);
}
