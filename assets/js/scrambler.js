function randomChar() {
    return chars[Math.floor(Math.random() * chars.length)];
}

function parseTags(str) {
    let parts = [];
    let regex = /(\{#([0-9A-Fa-f]{6})\})|(\{\})|(\{TIMER\})/gi;
    let lastIdx = 0;
    let match;
    while ((match = regex.exec(str)) !== null) {
        if (match.index > lastIdx) {
            parts.push({ type: 'text', val: str.substring(lastIdx, match.index) });
        }
        parts.push({ type: 'tag', val: match[0], color: match[2] || null, isTimer: !!match[4] });
        lastIdx = regex.lastIndex;
    }
    if (lastIdx < str.length) {
        parts.push({ type: 'text', val: str.substring(lastIdx) });
    }
    return parts;
}

function renderScrambled(parts, revealCount) {
    let out = "";
    let charIndex = 0;
    let inColor = false;

    for (let p of parts) {
        if (p.type === 'tag') {
            if (p.isTimer) {
                let val = (typeof window.inlineTimerValue !== 'undefined') ? window.inlineTimerValue : "";
                out += `<span class="inline-timer">${val}</span>`;
            } else if (p.color) {
                out += `<span style="color:#${p.color}; text-shadow: 0 0 10px #${p.color}cc, 0 0 20px #${p.color}80, 0 0 40px #${p.color}4d;">`;
                inColor = true;
            } else {
                if (inColor) out += `</span>`;
                inColor = false;
            }
        } else {
            for (let i = 0; i < p.val.length; i++) {
                if (p.val[i] === ' ' || p.val[i] === '\n') {
                    out += p.val[i];
                } else {
                    out += charIndex < revealCount ? p.val[i] : randomChar();
                }
                charIndex++;
            }
        }
    }
    if (inColor) out += `</span>`;
    return out;
}

function scrambleReveal(text, scrambleTime, revealTime, onUpdate, onComplete) {
    const runId = ++activeScrambleId;
    const start = performance.now();
    let lastTick = 0;

    let parts = parseTags(text);
    let totalLen = parts.reduce((acc, p) => acc + (p.type === 'text' ? p.val.length : 0), 0);

    if (typeof scrambleAudio !== 'undefined' && scrambleAudio) {
        scrambleAudio.currentTime = 0;
        scrambleAudio.play().catch(() => {});
    }

    function loop(now) {
        if (runId !== activeScrambleId) return;

        if (now - lastTick < scrambleSpeed) {
            requestAnimationFrame(loop);
            return;
        }
        lastTick = now;

        const elapsed = (now - start) / 1000;

        if (elapsed < scrambleTime) {
            onUpdate("_" + renderScrambled(parts, -1) + "_");
            return requestAnimationFrame(loop);
        }

        const progress = Math.min((elapsed - scrambleTime) / revealTime, 1);
        const revealCount = Math.floor(progress * totalLen);

        onUpdate("_" + renderScrambled(parts, revealCount) + "_");

        if (progress < 1) {
            requestAnimationFrame(loop);
        } else {
            onUpdate("_" + renderScrambled(parts, totalLen) + "_");

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

    let parts = parseTags(text);
    let totalLen = parts.reduce((acc, p) => acc + (p.type === 'text' ? p.val.length : 0), 0);

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
            onUpdate("_" + renderScrambled(parts, -1) + "_");
            return requestAnimationFrame(loop);
        }

        const progress = Math.min((elapsed - scrambleTime) / revealTime, 1);
        const revealCount = Math.floor(progress * totalLen);

        onUpdate("_" + renderScrambled(parts, revealCount) + "_");

        if (progress < 1) {
            requestAnimationFrame(loop);
        } else {
            scrambleAudio.pause();
            scrambleAudio.currentTime = 0;
            onUpdate("_" + renderScrambled(parts, totalLen) + "_");

            canResolve = true;
            document.getElementById("achievedBtn")?.removeAttribute("disabled");
            document.getElementById("failedBtn")?.removeAttribute("disabled");
        }
    }

    requestAnimationFrame(loop);
}
