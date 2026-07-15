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
    scrambleReveal(text, 0.3, 0.8, t => display.innerHTML = t);
}

function showResultTextIntro(text) {
    scrambleReveal(text, 0.3, 0.8, t => display.innerHTML = t);
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
    const isLong = (typeof currentTimerFormatLong !== 'undefined' && currentTimerFormatLong);
    
    if (totalSeconds < 60) return isLong ? `${totalSeconds} second${totalSeconds !== 1 ? 's' : ''}` : `${totalSeconds}s`;
    
    const h = Math.floor(totalSeconds / 3600);
    const m = Math.floor((totalSeconds % 3600) / 60);
    const s = totalSeconds % 60;
    let parts = [];
    
    if (h > 0) parts.push(isLong ? `${h} hour${h !== 1 ? 's' : ''}` : `${h}h`);
    if (m > 0 || (h > 0 && s > 0)) parts.push(isLong ? `${m} minute${m !== 1 ? 's' : ''}` : `${m}m`);
    if (s > 0) parts.push(isLong ? `${s} second${s !== 1 ? 's' : ''}` : `${s}s`);
    
    return parts.join(" ");
}

function showResultButtons(duration, respond = true) {
    canResolve = false;

    if (duration === "-") duration = 0;
    let timeLeft = duration || 10;

    if (respond) {
        buttonContainer.innerHTML = `
            <button id="achievedBtn" disabled>Pass</button>
            <div id="timerDisplay">${duration === 0 ? "INF" : formatTime(timeLeft)}</div>
            <button id="failedBtn" disabled>Failed</button>
        `;
        document.getElementById("achievedBtn").onclick = triggerPass;
        document.getElementById("failedBtn").onclick = triggerFail;
    } else {
        buttonContainer.innerHTML = `
            <div id="timerDisplay" style="font-size: 1.5em; padding: 10px;">${duration === 0 ? "INF" : formatTime(timeLeft)}</div>
        `;
    }

    if (currentTimer) clearInterval(currentTimer);
    window.inlineTimerValue = (duration === 0) ? "INF" : timeLeft;

    currentTimer = setInterval(() => {
        if (!canResolve) return;

        if (timeLeft > 0 || duration === 0) {
            if (duration !== 0) timeLeft--;
            const timerEl = document.getElementById("timerDisplay");
            let dispVal = duration === 0 ? "INF" : formatTime(timeLeft);
            if (timerEl) timerEl.textContent = dispVal;
            window.inlineTimerValue = (duration === 0) ? "INF" : timeLeft;
            const inlineTimers = document.querySelectorAll('.inline-timer');
            inlineTimers.forEach(el => el.textContent = window.inlineTimerValue);
        } else {
            clearInterval(currentTimer);
            let connected = (typeof txCharacteristic !== 'undefined' && txCharacteristic);
            if (!connected) {
                if (respond) {
                    if (typeof triggerFail === 'function') {
                        triggerFail();
                    }
                } else {
                    showResultTextIntro("CLEAR.");
                    showPlayButton();
                }
            } else {
                const timerEl = document.getElementById("timerDisplay");
                if (timerEl) timerEl.textContent = formatTime(0);
                window.inlineTimerValue = 0;
                const inlineTimers = document.querySelectorAll('.inline-timer');
                inlineTimers.forEach(el => el.textContent = window.inlineTimerValue);
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

    let hasMacro = false;
    let originalText = text;
    text = text.replace(/\{RAND:(\d+)-(\d+)\}/g, (match, min, max) => {
        hasMacro = true;
        min = parseInt(min);
        max = parseInt(max);
        return Math.floor(Math.random() * (max - min + 1)) + min;
    });

    if (hasMacro) {
        if (typeof sendBleMessage === 'function') {
            sendBleMessage(text, dur, respond);
        }
    } else {
        if (typeof txCharacteristic !== 'undefined' && txCharacteristic && idx !== -1) {
            sendBleCommand("SHOW_IDX:" + idx);
        }
    }

    if (typeof scrambleReveal === 'function') {
        scrambleReveal(text, scrambleDuration, revealDuration, t => display.innerHTML = t);
    } else {
        display.innerHTML = text;
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
    text = text.replace(/\{RAND:(\d+)-(\d+)\}/g, (match, min, max) => {
        min = parseInt(min);
        max = parseInt(max);
        return Math.floor(Math.random() * (max - min + 1)) + min;
    });

    if (typeof sendBleMessage === 'function') {
        sendBleMessage(text, duration, respond);
    }

    if (duration === "-") duration = 0;

    if (typeof scrambleReveal === 'function') {
        scrambleReveal(text, scrambleDuration, revealDuration, t => display.innerHTML = t);
    } else {
        display.innerHTML = text;
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
