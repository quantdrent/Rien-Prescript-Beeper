
const display = document.getElementById("scrambleText");
const buttonContainer = document.getElementById("buttonContainer");
const startBtn = document.getElementById("startBtn");
const scrambleAudio = document.getElementById("scrambleAudio");

const achievedSpan = document.getElementById("achievedCount");
const failedSpan = document.getElementById("failedCount");
const totalSpan = document.getElementById("totalCount");

const scrambleSpeed = 50;
const scrambleDuration = 0.5;
const revealDuration = 1.5;
const chars = "ABCDEF@HIJ_LM%OPQR^WX#YZa#b+cdefgh*iqrxyz0123456789";

let achieved = 0;
let failed = 0;
let total = 0;

let canResolve = false;
let clickCount = 0;
let activeScrambleId = 0;
let lastMessage = null;
