let customPrescripts = [];
let pendingDeviceCustoms = [];

function prescriptToString(p) {
    return `${p.duration}|${p.text}`;
}

function stringToPrescript(s) {
    const sep = s.indexOf('|');
    if (sep !== -1 && sep < 5) {
        return { duration: parseInt(s.substring(0, sep), 10) || 10, text: s.substring(sep + 1) };
    }
    return { duration: 10, text: s };
}

function savePrescripts() {
    try { localStorage.setItem("customPrescripts", JSON.stringify(customPrescripts)); } catch (err) {
        console.warn("localStorage write failed:", err);
    }
}

function loadPrescripts() {
    try {
        const saved = localStorage.getItem("customPrescripts");
        if (saved) return JSON.parse(saved);
    } catch (err) {
        console.warn("localStorage read failed:", err);
    }
    return null;
}

document.addEventListener("DOMContentLoaded", () => {
    const saved = loadPrescripts();

    if (saved && saved.length > 0) {
        if (typeof saved[0] === 'string') {
            customPrescripts = saved.map(stringToPrescript);
            savePrescripts();
        } else {
            customPrescripts = saved;
        }
    } else if (!saved) {
        if (typeof defaultPrescriptsList !== 'undefined') {
            customPrescripts = [...defaultPrescriptsList];
            savePrescripts();
        }
    }

    const modal = document.getElementById("customsModal");
    const btn = document.getElementById("customsBtn");
    const span = document.getElementById("closeCustoms");
    const addBtn = document.getElementById("addCustomBtn");
    const input = document.getElementById("newCustomInput");
    const durInput = document.getElementById("newCustomDuration");

    const syncModal = document.getElementById("syncModal");
    const closeSync = document.getElementById("closeSync");
    const btnOverride = document.getElementById("btnDeviceOverride");
    const btnMerge = document.getElementById("btnSmartMerge");

    if (btn) {
        btn.addEventListener("click", () => {
            modal.style.display = "block";
            renderCustomsList();
            if (typeof sendBleCommand === 'function') {
                sendBleCommand("GET_CUSTOMS");
            }
        });
    }

    if (span) {
        span.addEventListener("click", () => {
            modal.style.display = "none";
        });
    }

    if (closeSync) {
        closeSync.addEventListener("click", () => {
            syncModal.style.display = "none";
        });
    }

    window.addEventListener("click", (event) => {
        if (event.target === modal) {
            modal.style.display = "none";
        }
        if (event.target === syncModal) {
            syncModal.style.display = "none";
        }
    });

    if (btnOverride) {
        btnOverride.addEventListener("click", () => {
            syncModal.style.display = "none";
            updateCustomPrescriptsList(pendingDeviceCustoms);
        });
    }

    if (btnMerge) {
        btnMerge.addEventListener("click", () => {
            syncModal.style.display = "none";
            const merged = [...pendingDeviceCustoms];
            customPrescripts.forEach(localItem => {
                const isDuplicate = merged.some(m => m.text === localItem.text && m.duration === localItem.duration);
                if (!isDuplicate) {
                    merged.push(localItem);
                }
            });
            updateCustomPrescriptsList(merged);
            syncCustomsToDevice();
        });
    }

    if (addBtn) {
        addBtn.addEventListener("click", () => {
            const text = input.value.trim();
            const duration = parseInt(durInput.value.trim(), 10) || 10;

            if (text) {
                input.value = "";
                customPrescripts.push({ duration, text });
                savePrescripts();
                renderCustomsList();

                if (typeof sendBleCommand === 'function') {
                    sendBleCommand("ADD_CUSTOM", prescriptToString({ duration, text }));
                }
            }
        });
    }
});

function handleDeviceCustomsSync(deviceCustomStrings) {
    const deviceCustoms = deviceCustomStrings.map(stringToPrescript);
    const saved = loadPrescripts();
    let localCustoms = [];

    if (saved && saved.length > 0) {
        localCustoms = (typeof saved[0] === 'string') ? saved.map(stringToPrescript) : saved;
    }

    if (localCustoms.length === 0 || JSON.stringify(localCustoms) === JSON.stringify(deviceCustoms)) {
        updateCustomPrescriptsList(deviceCustoms);
    } else {
        pendingDeviceCustoms = deviceCustoms;
        document.getElementById("syncModal").style.display = "block";
    }
}

function updateCustomPrescriptsList(customs) {
    customPrescripts = customs;
    savePrescripts();
    renderCustomsList();
}

async function syncCustomsToDevice() {
    savePrescripts();
    if (typeof sendBleCommand === 'function') {
        await sendBleCommand("CLEAR_CUSTOMS");
        for (let i = 0; i < customPrescripts.length; i++) {
            await new Promise(r => setTimeout(r, 100));
            await sendBleCommand("ADD_CUSTOM", prescriptToString(customPrescripts[i]));
        }
    }
}

function renderCustomsList() {
    const listDiv = document.getElementById("customsList");
    if (!listDiv) return;

    if (!customPrescripts || customPrescripts.length === 0) {
        listDiv.textContent = "No custom prescripts found.";
        return;
    }

    listDiv.innerHTML = "";
    customPrescripts.forEach((prescript, index) => {
        const item = document.createElement("div");
        item.className = "custom-item";

        const textInput = document.createElement("input");
        textInput.type = "text";
        textInput.value = prescript.text;
        textInput.className = "custom-item-text";

        textInput.addEventListener("blur", () => {
            const newVal = textInput.value.trim();
            if (newVal !== prescript.text && newVal !== "") {
                customPrescripts[index] = { duration: prescript.duration, text: newVal };
                syncCustomsToDevice();
            }
        });

        const durInputEl = document.createElement("input");
        durInputEl.type = "number";
        durInputEl.value = prescript.duration;
        durInputEl.className = "custom-item-duration";

        durInputEl.addEventListener("blur", () => {
            const newDur = parseInt(durInputEl.value.trim(), 10) || 10;
            if (newDur !== prescript.duration) {
                customPrescripts[index] = { duration: newDur, text: textInput.value.trim() };
                syncCustomsToDevice();
            }
        });

        const playBtn = document.createElement("button");
        playBtn.textContent = "Send";
        playBtn.addEventListener("click", () => {
            if (typeof triggerManualPrescript === 'function') {
                triggerManualPrescript(textInput.value, parseInt(durInputEl.value, 10) || 10);
                document.getElementById("customsModal").style.display = "none";
            }
        });

        const delBtn = document.createElement("button");
        delBtn.textContent = "X";
        delBtn.style.color = "#ff6b6b";
        delBtn.style.borderColor = "#ff6b6b";
        delBtn.addEventListener("click", () => {
            customPrescripts.splice(index, 1);
            renderCustomsList();
            syncCustomsToDevice();
        });

        item.appendChild(textInput);
        item.appendChild(durInputEl);
        item.appendChild(playBtn);
        item.appendChild(delBtn);
        listDiv.appendChild(item);
    });
}
