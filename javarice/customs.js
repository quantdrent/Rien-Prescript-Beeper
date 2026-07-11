let customPrescripts = [];
let pendingDeviceCustoms = [];

function prescriptToString(p) {
    return `${p.duration}|${p.respond === false ? 0 : 1}|${p.text}`;
}

function stringToPrescript(s) {
    const sep1 = s.indexOf('|');
    if (sep1 !== -1 && sep1 < 5) {
        const sep2 = s.indexOf('|', sep1 + 1);
        if (sep2 !== -1 && sep2 - sep1 <= 2) {
            let dur = parseInt(s.substring(0, sep1), 10) || 10;
            let res = s.substring(sep1 + 1, sep2) !== '0';
            return { duration: dur, respond: res, text: s.substring(sep2 + 1) };
        } else {
            return { duration: parseInt(s.substring(0, sep1), 10) || 10, respond: true, text: s.substring(sep1 + 1) };
        }
    }
    return { duration: 10, respond: true, text: s };
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
    const sendBtn = document.getElementById("sendCustomBtn");
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
            const respondEl = document.getElementById("newCustomRespond");
            const respond = respondEl ? respondEl.checked : true;

            if (text) {
                input.value = "";
                customPrescripts.push({ duration, respond, text });
                savePrescripts();
                renderCustomsList();

                if (typeof sendBleCommand === 'function') {
                    sendBleCommand("ADD_CUSTOM", prescriptToString({ duration, respond, text }));
                }
            }
        });
    }

    if (sendBtn) {
        sendBtn.addEventListener("click", () => {
            const text = input.value.trim();
            const duration = parseInt(durInput.value.trim(), 10) || 10;
            const respondEl = document.getElementById("newCustomRespond");
            const respond = respondEl ? respondEl.checked : true;

            if (text) {
                input.value = "";
                if (typeof triggerManualPrescript === 'function') {
                    triggerManualPrescript(text, duration, respond);
                    document.getElementById("customsModal").style.display = "none";
                }
            }
        });
    }

    // Export prescripts to .txt
    const exportBtn = document.getElementById("exportPrescriptsBtn");
    if (exportBtn) {
        exportBtn.addEventListener("click", () => {
            if (!customPrescripts || customPrescripts.length === 0) {
                alert("No prescripts to export.");
                return;
            }
            const header = "# dur|respond|prescript\n# respond: 1=requires button press, 0=info only\n";
            const lines = customPrescripts.map(p => prescriptToString(p));
            const blob = new Blob([header + lines.join("\n")], { type: "text/plain" });
            const a = document.createElement("a");
            a.href = URL.createObjectURL(blob);
            a.download = "prescripts.txt";
            a.click();
            URL.revokeObjectURL(a.href);
        });
    }

    // Import prescripts from .txt
    const importBtn = document.getElementById("importPrescriptsBtn");
    const importFile = document.getElementById("importFileInput");
    if (importBtn && importFile) {
        importBtn.addEventListener("click", () => importFile.click());
        importFile.addEventListener("change", (e) => {
            const file = e.target.files[0];
            if (!file) return;
            const reader = new FileReader();
            reader.onload = (ev) => {
                const text = ev.target.result;
                const lines = text.split("\n").map(l => l.trim()).filter(l => l.length > 0 && !l.startsWith("#"));
                const imported = lines.map(stringToPrescript);
                if (imported.length > 0) {
                    customPrescripts = imported;
                    savePrescripts();
                    renderCustomsList();
                    syncCustomsToDevice();
                    alert(`Imported ${imported.length} prescripts.`);
                }
            };
            reader.readAsText(file);
            importFile.value = "";
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

    // Normalize both sides to the same string format before comparing
    const normalizeList = (list) => list.map(p => prescriptToString({
        duration: p.duration || 10,
        respond: p.respond !== false,
        text: p.text || ""
    }));

    const deviceNorm = JSON.stringify(normalizeList(deviceCustoms));
    const localNorm = JSON.stringify(normalizeList(localCustoms));

    if (deviceCustoms.length === 0 && localCustoms.length > 0) {
        syncCustomsToDevice();
    } else if (localCustoms.length === 0 || deviceNorm === localNorm) {
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
                customPrescripts[index] = { duration: prescript.duration, respond: prescript.respond, text: newVal };
                syncCustomsToDevice();
            }
        });

        const durInputEl = document.createElement("input");
        durInputEl.type = "number";
        durInputEl.value = prescript.duration;
        durInputEl.className = "custom-item-duration";
        durInputEl.style.width = "40px";

        durInputEl.addEventListener("blur", () => {
            const newDur = parseInt(durInputEl.value.trim(), 10) || 10;
            if (newDur !== prescript.duration) {
                customPrescripts[index] = { duration: newDur, respond: prescript.respond, text: textInput.value.trim() };
                syncCustomsToDevice();
            }
        });

        const resLabel = document.createElement("label");
        resLabel.style.display = "flex";
        resLabel.style.alignItems = "center";
        resLabel.style.fontSize = "0.75em";
        resLabel.style.color = "#aaa";
        resLabel.style.cursor = "pointer";
        resLabel.textContent = " Res:";
        
        const resInputEl = document.createElement("input");
        resInputEl.type = "checkbox";
        resInputEl.checked = prescript.respond !== false;
        resInputEl.style.margin = "0 5px";
        resInputEl.addEventListener("change", () => {
            customPrescripts[index].respond = resInputEl.checked;
            syncCustomsToDevice();
        });
        resLabel.appendChild(resInputEl);

        const playBtn = document.createElement("button");
        playBtn.textContent = "Send";
        playBtn.addEventListener("click", () => {
            if (typeof triggerManualPrescript === 'function') {
                triggerManualPrescript(textInput.value, parseInt(durInputEl.value, 10) || 10, resInputEl.checked);
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
        item.appendChild(resLabel);
        item.appendChild(playBtn);
        item.appendChild(delBtn);
        listDiv.appendChild(item);
    });
}
