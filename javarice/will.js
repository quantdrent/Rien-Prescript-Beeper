function handlePlayClick() {
    clickCount++;

    const picked = pickMessage();
    if (!picked) return;

    showResultButtons(picked.duration);
    

    if (typeof sendBleMessage === 'function') {
        sendBleMessage(picked.text, picked.duration);
    }

    scrambleReveal(
        picked.text,
        scrambleDuration,
        revealDuration,
        t => display.textContent = t
    );
}
startBtn.addEventListener("click", handlePlayClick);

document.addEventListener("DOMContentLoaded", () => {
    showResultTextIntro("Welcome back ####.");
});

function triggerManualPrescript(text, duration = 10) {
    showResultButtons(duration);
    if (typeof sendBleMessage === 'function') {
        sendBleMessage(text, duration);
    }
    scrambleReveal(
        text,
        scrambleDuration,
        revealDuration,
        t => display.textContent = t
    );
}
