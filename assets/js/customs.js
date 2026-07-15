let customPrescripts = [];
let pendingDeviceCustoms = [];

function prescriptToString(p) {
    let durStr = (p.duration === 0 || p.duration === "-") ? "-" : (p.duration === undefined ? 10 : p.duration);
    return `${durStr}|${p.respond === false ? 0 : 1}|${p.text}`;
}

function stringToPrescript(s) {
    const sep1 = s.indexOf('|');
    if (sep1 !== -1 && sep1 < 5) {
        const sep2 = s.indexOf('|', sep1 + 1);
        let durStr = s.substring(0, sep1);
        let durVal = parseInt(durStr, 10);
        let dur = durStr === "-" ? "-" : (isNaN(durVal) ? 10 : durVal);

        if (sep2 !== -1 && sep2 - sep1 <= 2) {
            let res = s.substring(sep1 + 1, sep2) !== '0';
            return { duration: dur, respond: res, text: s.substring(sep2 + 1) };
        } else {
            return { duration: dur, respond: true, text: s.substring(sep1 + 1) };
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
    const unlimInput = document.getElementById("newCustomUnlimited");
    if (unlimInput && durInput) {
        unlimInput.addEventListener("change", () => {
            durInput.disabled = unlimInput.checked;
        });
    }

    const respondInput = document.getElementById("newCustomRespond");
    const colorPicker = document.getElementById("inlineColorPicker");
    const colorizeBtn = document.getElementById("colorizeBtn");

    const tabVisualEditorBtn = document.getElementById("tabVisualEditorBtn");
    const tabRawEditorBtn = document.getElementById("tabRawEditorBtn");
    const visualEditorContainer = document.getElementById("visualEditorContainer");
    const rawEditorContainer = document.getElementById("rawEditorContainer");
    const rawEditorTextarea = document.getElementById("rawEditorTextarea");
    const saveRawEditorBtn = document.getElementById("saveRawEditorBtn");

    if (tabVisualEditorBtn && tabRawEditorBtn) {
        tabVisualEditorBtn.addEventListener("click", () => {
            tabVisualEditorBtn.classList.add("active");
            tabRawEditorBtn.classList.remove("active");
            visualEditorContainer.style.display = "flex";
            rawEditorContainer.style.display = "none";
            renderCustomsList();
        });

        tabRawEditorBtn.addEventListener("click", () => {
            tabRawEditorBtn.classList.add("active");
            tabVisualEditorBtn.classList.remove("active");
            visualEditorContainer.style.display = "none";
            rawEditorContainer.style.display = "block";

            let rawText = "";
            customPrescripts.forEach(p => {
                rawText += prescriptToString(p) + "\n";
            });
            rawEditorTextarea.value = rawText.trim();
        });
    }

    let rawEditorTimeout = null;
    if (rawEditorTextarea) {
        rawEditorTextarea.addEventListener("input", () => {
            if (rawEditorTimeout) clearTimeout(rawEditorTimeout);
            rawEditorTimeout = setTimeout(() => {
                const lines = rawEditorTextarea.value.split('\n');
                const newPrescripts = [];
                for (let line of lines) {
                    line = line.trim();
                    if (line && !line.startsWith('#')) {
                        newPrescripts.push(stringToPrescript(line));
                    }
                }
                customPrescripts = newPrescripts;
                savePrescripts();
                syncCustomsToDevice();
            }, 800); // Auto-save after 800ms of no typing
        });
    }

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

    function getSurroundingTag(text, cursorStart, cursorEnd) {
        let tagStart = text.lastIndexOf('{#', Math.max(0, cursorStart));
        if (tagStart !== -1) {
            let hexEnd = text.indexOf('}', tagStart + 2);
            if (hexEnd !== -1) {
                let closeTag = text.indexOf('{}', hexEnd + 1);
                if (closeTag !== -1) {
                    if (cursorEnd <= closeTag + 2) {
                        const hex = text.substring(tagStart + 2, hexEnd);
                        if (hex.match(/^[0-9A-Fa-f]{6}$/)) {
                            return { start: tagStart, end: closeTag + 2, textInside: text.substring(hexEnd + 1, closeTag) };
                        }
                    }
                }
            }
        }
        return null;
    }

    function updateColorizeBtn() {
        if (!input || !colorizeBtn) return;
        const start = input.selectionStart;
        const end = input.selectionEnd;
        const tagInfo = getSurroundingTag(input.value, start, end);
        if (tagInfo) {
            colorizeBtn.textContent = "Remove Color";
            colorizeBtn.style.color = "#ff9999";
            colorizeBtn.style.borderColor = "#ff9999";
        } else {
            colorizeBtn.textContent = "Colorize Highlighted";
            colorizeBtn.style.color = "#99edff";
            colorizeBtn.style.borderColor = "#99edff";
        }
    }

    if (input) {
        input.addEventListener("mouseup", updateColorizeBtn);
        input.addEventListener("keyup", updateColorizeBtn);
        input.addEventListener("focus", updateColorizeBtn);
    }

    if (colorizeBtn && input && colorPicker) {
        colorizeBtn.addEventListener("click", () => {
            const start = input.selectionStart;
            const end = input.selectionEnd;
            const text = input.value;
            const tagInfo = getSurroundingTag(text, start, end);

            if (tagInfo) {
                const before = text.substring(0, tagInfo.start);
                const after = text.substring(tagInfo.end);
                input.value = before + tagInfo.textInside + after;
                input.focus();
                input.setSelectionRange(before.length, before.length + tagInfo.textInside.length);
            } else {
                if (start !== end) {
                    const hex = colorPicker.value.toUpperCase();
                    const before = text.substring(0, start);
                    const selected = text.substring(start, end);
                    const after = text.substring(end);
                    input.value = `${before}{${hex}}${selected}{}${after}`;
                    input.focus();
                    const newEnd = start + hex.length + 4 + selected.length + 2;
                    input.setSelectionRange(newEnd, newEnd);
                }
            }
            updateColorizeBtn();
        });
    }

    const previewNewBtn = document.getElementById("previewNewBtn");
    const previewDiv = document.getElementById("newCustomPreview");

    if (previewNewBtn && previewDiv && input) {
        previewNewBtn.addEventListener("click", () => {
            if (previewDiv.style.display === "none") {
                let html = input.value.replace(/</g, "&lt;").replace(/>/g, "&gt;");
                html = html.replace(/\{#([0-9A-Fa-f]{6})\}(.*?)\{\}/gi, '<span style="color: #$1; text-shadow: 0 0 5px #$1cc, 0 0 10px #$180;">$2</span>');
                previewDiv.innerHTML = html;
                previewDiv.style.display = "block";
                input.style.display = "none";
                previewNewBtn.textContent = "Edit RAW";
                previewNewBtn.style.color = "#000";
                previewNewBtn.style.background = "#99edff";
                previewDiv.focus();
            } else {
                previewDiv.style.display = "none";
                input.style.display = "block";
                previewNewBtn.textContent = "Preview";
                previewNewBtn.style.color = "#99edff";
                previewNewBtn.style.background = "none";
            }
        });

        previewDiv.addEventListener("input", () => {
            let raw = "";
            for (let node of previewDiv.childNodes) {
                if (node.nodeType === Node.TEXT_NODE) {
                    raw += node.textContent;
                } else if (node.nodeType === Node.ELEMENT_NODE) {
                    if (node.tagName === 'SPAN' && node.style.color) {
                        let c = node.style.color;
                        let hex = "FFFFFF";
                        if (c.startsWith('#')) {
                            hex = c.substring(1).toUpperCase();
                        } else {
                            let m = c.match(/^rgb\((\d+),\s*(\d+),\s*(\d+)\)$/);
                            if (m) {
                                let r = parseInt(m[1]).toString(16).padStart(2, '0');
                                let g = parseInt(m[2]).toString(16).padStart(2, '0');
                                let b = parseInt(m[3]).toString(16).padStart(2, '0');
                                hex = (r + g + b).toUpperCase();
                            }
                        }
                        raw += `{#${hex}}${node.textContent}{}`;
                    } else if (node.tagName === 'BR' || node.tagName === 'DIV') {
                        raw += "\n" + node.textContent;
                    } else {
                        raw += node.textContent;
                    }
                }
            }
            input.value = raw.trim();
        });
    }

    if (addBtn) {
        addBtn.addEventListener("click", () => {
            const text = input.value.trim();
            const unlimEl = document.getElementById("newCustomUnlimited");
            const duration = (unlimEl && unlimEl.checked) ? "-" : (parseInt(durInput.value.trim(), 10) || 10);
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
            const unlimEl = document.getElementById("newCustomUnlimited");
            const duration = (unlimEl && unlimEl.checked) ? "-" : (parseInt(durInput.value.trim(), 10) || 10);
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

    const normalizeList = (list) => list.map(p => prescriptToString({
        duration: (p.duration === "-" || p.duration === 0) ? "-" : (p.duration || 10),
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

let isSyncingCustoms = false;
let syncCustomsRequested = false;

async function syncCustomsToDevice() {
    savePrescripts();
    if (typeof sendBleCommand !== 'function') return;

    if (isSyncingCustoms) {
        syncCustomsRequested = true;
        return;
    }

    isSyncingCustoms = true;
    do {
        syncCustomsRequested = false;
        await sendBleCommand("CLEAR_CUSTOMS");
        for (let i = 0; i < customPrescripts.length; i++) {
            await new Promise(r => setTimeout(r, 250));
            await sendBleCommand("ADD_CUSTOM", prescriptToString(customPrescripts[i]));
        }
    } while (syncCustomsRequested);
    isSyncingCustoms = false;
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

        const textContainer = document.createElement("div");
        textContainer.style.flex = "1";
        textContainer.style.display = "flex";

        const textInput = document.createElement("input");
        textInput.type = "text";
        textInput.value = prescript.text;
        textInput.className = "custom-item-text";
        textInput.style.width = "100%";

        textInput.addEventListener("blur", () => {
            const newVal = textInput.value.trim();
            if (newVal !== prescript.text && newVal !== "") {
                customPrescripts[index] = { duration: prescript.duration, respond: prescript.respond, text: newVal };
                syncCustomsToDevice();
            }
        });

        textContainer.appendChild(textInput);

        const durInputEl = document.createElement("input");
        durInputEl.type = "number";
        durInputEl.value = prescript.duration === "-" ? "" : prescript.duration;
        durInputEl.className = "custom-item-duration";
        durInputEl.style.width = "40px";
        durInputEl.min = "1";
        if (prescript.duration === "-") {
            durInputEl.disabled = true;
        }

        const unlimLabel = document.createElement("label");
        unlimLabel.style.fontSize = "0.85em";
        unlimLabel.style.color = "#aaa";
        unlimLabel.style.cursor = "pointer";
        unlimLabel.style.whiteSpace = "nowrap";
        
        const unlimCheckbox = document.createElement("input");
        unlimCheckbox.type = "checkbox";
        unlimCheckbox.checked = prescript.duration === "-";
        unlimCheckbox.style.marginRight = "2px";
        
        unlimLabel.appendChild(unlimCheckbox);
        unlimLabel.appendChild(document.createTextNode(" \u221E"));

        unlimCheckbox.addEventListener("change", () => {
            durInputEl.disabled = unlimCheckbox.checked;
            const newDur = unlimCheckbox.checked ? "-" : (parseInt(durInputEl.value.trim(), 10) || 10);
            if (newDur !== prescript.duration) {
                customPrescripts[index] = { duration: newDur, respond: prescript.respond, text: textInput.value.trim() };
                syncCustomsToDevice();
                renderCustomsList();
            }
        });

        durInputEl.addEventListener("blur", () => {
            const newDur = unlimCheckbox.checked ? "-" : (parseInt(durInputEl.value.trim(), 10) || 10);
            if (newDur !== prescript.duration) {
                customPrescripts[index] = { duration: newDur, respond: prescript.respond, text: textInput.value.trim() };
                syncCustomsToDevice();
                renderCustomsList();
            }
        });

        textContainer.appendChild(durInputEl);
        textContainer.appendChild(unlimLabel);


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
                const triggerDur = unlimCheckbox.checked ? "-" : (parseInt(durInputEl.value.trim(), 10) || 10);
                triggerManualPrescript(textInput.value, triggerDur, resInputEl.checked);
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

        item.appendChild(textContainer);
        item.appendChild(durInputEl);
        item.appendChild(resLabel);
        item.appendChild(playBtn);
        item.appendChild(delBtn);
        listDiv.appendChild(item);
    });
}
