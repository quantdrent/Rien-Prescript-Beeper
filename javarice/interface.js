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

function triggerPass() {
    if (!canResolve) return;
    if (currentTimer) clearInterval(currentTimer);
    
    if (typeof txCharacteristic !== 'undefined' && txCharacteristic) {
        sendBleCommand("ADD_STATS", "1,0,1");
    } else {
        achieved++; total++;
        updateCounters();
    }
    
    showResultText("PASSED");
    if (typeof sendBleMessage === 'function') {
        sendBleMessage("PASSED", 5);
    }
}

function triggerFail() {
    if (!canResolve) return;
    if (currentTimer) clearInterval(currentTimer);
    
    if (typeof txCharacteristic !== 'undefined' && txCharacteristic) {
        sendBleCommand("ADD_STATS", "0,1,1");
    } else {
        failed++; total++;
        updateCounters();
    }
    
    showResultText("FAILED");
    if (typeof sendBleMessage === 'function') {
        sendBleMessage("FAILED", 5);
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
