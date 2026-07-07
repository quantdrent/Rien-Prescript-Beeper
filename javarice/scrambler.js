function randomChar() {
    return chars[Math.floor(Math.random() * chars.length)];
}

function scrambleReveal(text, scrambleTime, revealTime, onUpdate, onComplete) {
    const runId = ++activeScrambleId;
    const start = performance.now();
    let lastTick = 0;

    scrambleAudio.pause();
    scrambleAudio.currentTime = 0;
    scrambleAudio.play().catch(() => {});

    function loop(now) {
        if (runId !== activeScrambleId) {
            scrambleAudio.pause();
            scrambleAudio.currentTime = 0;
            return;
        }

        if (now - lastTick < scrambleSpeed) {
            requestAnimationFrame(loop);
            return;
        }
        lastTick = now;

        const elapsed = (now - start) / 1000;

        if (elapsed < scrambleTime) {
            let out = "";
            for (let i = 0; i < text.length; i++) out += randomChar();
            onUpdate("_" + out + "_");
            return requestAnimationFrame(loop);
        }

        const progress = Math.min((elapsed - scrambleTime) / revealTime, 1);
        const revealCount = Math.floor(progress * text.length);

        let out = "";
        for (let i = 0; i < text.length; i++) {
            out += i < revealCount ? text[i] : randomChar();
        }

        onUpdate("_" + out + "_");

        if (progress < 1) {
            requestAnimationFrame(loop);
        } else {
            scrambleAudio.pause();
            scrambleAudio.currentTime = 0;

            onUpdate("_" + text + "_");

            canResolve = true;
            document.getElementById("achievedBtn")?.removeAttribute("disabled");
            document.getElementById("failedBtn")?.removeAttribute("disabled");

            onComplete?.();
        }
    }

    requestAnimationFrame(loop);
}
function scrambleRevealIntro(text, scrambleTime, revealTime, onUpdate) {
    const runId = ++activeScrambleId;
    const start = performance.now();
    let lastTick = 0;

    scrambleAudio.pause();
    scrambleAudio.currentTime = 0;
    scrambleAudio.play().catch(() => {});

    function loop(now) {
        if (runId !== activeScrambleId) {
            scrambleAudio.pause();
            scrambleAudio.currentTime = 0;
            return;
        }

        if (now - lastTick < scrambleSpeed) {
            requestAnimationFrame(loop);
            return;
        }
        lastTick = now;

        const elapsed = (now - start) / 1000;

        if (elapsed < scrambleTime) {
            let out = "";
            for (let i = 0; i < text.length; i++) out += randomChar();
            onUpdate("_" + out + "_");
            return requestAnimationFrame(loop);
        }

        const progress = Math.min((elapsed - scrambleTime) / revealTime, 1);
        const revealCount = Math.floor(progress * text.length);

        let out = "";
        for (let i = 0; i < text.length; i++) {
            out += i < revealCount ? text[i] : randomChar();
        }

        onUpdate("_" + out + "_");

        if (progress < 1) {
            requestAnimationFrame(loop);
        } else {
            scrambleAudio.pause();
            scrambleAudio.currentTime = 0;

            onUpdate("_" + text + "_");

            canResolve = true;
            document.getElementById("achievedBtn")?.removeAttribute("disabled");
            document.getElementById("failedBtn")?.removeAttribute("disabled");

            onComplete?.();
        }
    }

    requestAnimationFrame(loop);
}
