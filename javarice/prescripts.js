const defaultPrescriptsList = [
    { duration: 10, text: "Go corrode NOW!" },
    { duration: 15, text: "Spend 1300 lunacy on a standard fare" },
    { duration: 10, text: "Use the spork" },
    { duration: 20, text: "Solo rien sang md" },
    { duration: 10, text: "Use a overclock ego" }
];

function pickMessage() {
    let picked = null;
    let attempts = 0;

    let sourceList = defaultPrescriptsList;
    if (typeof customPrescripts !== 'undefined') {
        sourceList = customPrescripts;
    }

    if (!sourceList || sourceList.length === 0) {
        return { text: "NO PRESCRIPTS FOUND", duration: 10 };
    }

    while (attempts < 10) {
        picked = sourceList[Math.floor(Math.random() * sourceList.length)];
        if (!lastMessage || picked.text !== lastMessage.text || sourceList.length === 1) break;
        attempts++;
    }
    lastMessage = picked;
    return picked;
}
