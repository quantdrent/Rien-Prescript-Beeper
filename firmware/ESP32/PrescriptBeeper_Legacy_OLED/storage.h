#ifndef STORAGE_H
#define STORAGE_H

#include <LittleFS.h>
#include "config.h"

struct Stats {
  int achieved;
  int failed;
  int total;
};

void sendChunked(const char* data, int len);
bool bleNotifyReady();

int textScale = DEFAULT_TEXT_SCALE;
int scrambleDurationFrames = DEFAULT_SCRAMBLE_FRAMES;
int scrambleDelayMs = DEFAULT_SCRAMBLE_DELAY;
int revealDelayMs = DEFAULT_REVEAL_DELAY;
int timerPosition = 0;
float timerScale = 1.0;
bool bleRequirePin = false;
bool useProportionalFont = false;

int cachedTotalPrescripts = -1;

void invalidatePrescriptCache() {
  cachedTotalPrescripts = -1;
}

void readSettings() {
  File f = LittleFS.open(SETTINGS_FILE, "r");
  if (f) {
    String content = f.readString();
    f.close();
    content.trim();
    if (content.length() > 0) {
      int c1 = content.indexOf(',');
      int c2 = content.indexOf(',', c1 + 1);
      int c3 = content.indexOf(',', c2 + 1);

      int c4 = content.indexOf(',', c3 + 1);
      int c5 = content.indexOf(',', c4 + 1);
      int c6 = content.indexOf(',', c5 + 1);
      int c7 = content.indexOf(',', c6 + 1);

      if (c1 == -1) {
        textScale = content.toInt();
      } else {
        textScale = content.substring(0, c1).toInt();
        scrambleDurationFrames = content.substring(c1 + 1, c2 == -1 ? content.length() : c2).toInt();
        if (c2 != -1) scrambleDelayMs = content.substring(c2 + 1, c3 == -1 ? content.length() : c3).toInt();

        if (c4 != -1 && c5 != -1) {
          revealDelayMs = content.substring(c3 + 1, c4).toInt();
          timerPosition = content.substring(c4 + 1, c5).toInt();

          if (c6 != -1) {
            timerScale = content.substring(c5 + 1, c6).toFloat();
            if (c7 != -1) {
              bleRequirePin = content.substring(c6 + 1, c7).toInt() != 0;
              useProportionalFont = content.substring(c7 + 1).toInt() != 0;
            } else {
              bleRequirePin = content.substring(c6 + 1).toInt() != 0;
            }
          } else {
            timerScale = content.substring(c5 + 1).toFloat();
          }
        } else {
          if (c3 != -1) revealDelayMs = content.substring(c3 + 1).toInt();
        }
      }
    }
  }
}

void writeSettings() {
  LittleFS.remove(SETTINGS_FILE);
  File f = LittleFS.open(SETTINGS_FILE, "w");
  if (f) {
    f.println(String(textScale) + "," + String(scrambleDurationFrames) + "," + String(scrambleDelayMs) + "," + String(revealDelayMs) + "," + String(timerPosition) + "," + String(timerScale, 2) + "," + String(bleRequirePin ? 1 : 0) + "," + String(useProportionalFont ? 1 : 0));
    f.close();
  }
}

void clearCustoms() {
  LittleFS.remove(CUSTOMS_FILE);
  invalidatePrescriptCache();
}

void sendSettings() {
  if (!bleNotifyReady()) return;
  String msg = "RES:SETTINGS|MSG:" + String(textScale) + "," + String(scrambleDurationFrames) + "," + String(scrambleDelayMs) + "," + String(revealDelayMs) + "," + String(timerPosition) + "," + String(timerScale, 2) + "," + String(bleRequirePin ? 1 : 0) + "," + String(useProportionalFont ? 1 : 0) + "\n";
  sendChunked(msg.c_str(), msg.length());
}

Stats parseCSV(const String& csv) {
  Stats s = {0, 0, 0};
  int c1 = csv.indexOf(',');
  int c2 = csv.lastIndexOf(',');
  if (c1 != -1 && c2 != -1 && c1 != c2) {
    s.achieved = csv.substring(0, c1).toInt();
    s.failed = csv.substring(c1 + 1, c2).toInt();
    s.total = csv.substring(c2 + 1).toInt();
  }
  return s;
}

Stats readStatsFromFile() {
  Stats s = {0, 0, 0};
  File f = LittleFS.open(STATS_FILE, "r");
  if (f) {
    String content = f.readString();
    f.close();
    content.trim();
    if (content.length() > 0) {
      s = parseCSV(content);
    }
  }
  return s;
}

void writeStatsToFile(Stats s) {
  LittleFS.remove(STATS_FILE);
  File f = LittleFS.open(STATS_FILE, "w");
  if (f) {
    f.println(String(s.achieved) + "," + String(s.failed) + "," + String(s.total));
    f.close();
  }
}

Stats currentStats = {0, 0, 0};
bool statsLoaded = false;
bool statsDirty = false;

void initStats() {
  if (!statsLoaded) {
    currentStats = readStatsFromFile();
    statsLoaded = true;
  }
}

void sendStats() {
  if (!bleNotifyReady()) return;
  initStats();
  String msg = "RES:STATS|MSG:" + String(currentStats.achieved) + "," + String(currentStats.failed) + "," + String(currentStats.total) + "\n";
  sendChunked(msg.c_str(), msg.length());
}

void addStats(String diffData) {
  initStats();
  Stats diff = parseCSV(diffData);
  if (diff.achieved > 0) Serial.println("PASSED");
  if (diff.failed > 0) Serial.println("FAILED");

  currentStats.achieved += diff.achieved;
  currentStats.failed += diff.failed;
  currentStats.total += diff.total;

  statsDirty = true;
  sendStats();
}

void flushStatsIfNeeded() {
  if (statsDirty) {
    writeStatsToFile(currentStats);
    statsDirty = false;
  }
}

void saveCustomRule(String rule) {
  File customsFile = LittleFS.open(CUSTOMS_FILE, "a");
  if (customsFile) {
    customsFile.println(rule);
    customsFile.close();
    invalidatePrescriptCache();
  }
}

void sendCustomRules() {
  if (!bleNotifyReady()) return;

  File customsFile = LittleFS.open(CUSTOMS_FILE, "r");
  if (!customsFile) {
    const char* empty = "RES:CUSTOMS|MSG:[]\n";
    sendChunked(empty, strlen(empty));
    return;
  }

  char buf[JSON_BUF_SIZE];
  int pos = 0;
  const char* prefix = "RES:CUSTOMS|MSG:[";
  int prefixLen = strlen(prefix);
  memcpy(buf, prefix, prefixLen);
  pos = prefixLen;

  bool first = true;

  while (customsFile.available()) {
    String line = customsFile.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    line.replace("\"", "\\\"");

    int needed = (first ? 0 : 1) + 1 + line.length() + 1;
    if (pos + needed + 3 >= JSON_BUF_SIZE) break;

    if (!first) buf[pos++] = ',';
    buf[pos++] = '"';
    memcpy(buf + pos, line.c_str(), line.length());
    pos += line.length();
    buf[pos++] = '"';
    first = false;
  }

  buf[pos++] = ']';
  buf[pos++] = '\n';
  buf[pos] = '\0';

  customsFile.close();
  sendChunked(buf, pos);
}

int countStoredPrescripts() {
  if (cachedTotalPrescripts != -1) return cachedTotalPrescripts;
  File f = LittleFS.open(CUSTOMS_FILE, "r");
  if (!f) return 0;

  int count = 0;
  bool hasChars = false;
  char readBuf[128];

  while (f.available()) {
    int bytesRead = f.read((uint8_t*)readBuf, sizeof(readBuf));
    for (int i = 0; i < bytesRead; i++) {
      char c = readBuf[i];
      if (c == '\n') {
        if (hasChars) count++;
        hasChars = false;
      } else if (c != ' ' && c != '\r') {
        hasChars = true;
      }
    }
  }
  if (hasChars) count++;

  f.close();
  cachedTotalPrescripts = count;
  return count;
}

String getPrescriptByIndex(int target) {
  File f = LittleFS.open(CUSTOMS_FILE, "r");
  if (!f) return "";

  int current = 0;
  String result = "";
  char buf[128];
  int pos = 0;
  bool hasChars = false;
  char readBuf[128];

  while (f.available()) {
    int bytesRead = f.read((uint8_t*)readBuf, sizeof(readBuf));
    for (int i = 0; i < bytesRead; i++) {
      char c = readBuf[i];
      if (c == '\n') {
        if (hasChars) {
          if (current == target) {
            buf[pos] = '\0';
            result = String(buf);
            result.trim();
            f.close();
            return result;
          }
          current++;
        }
        pos = 0;
        hasChars = false;
      } else {
        if (c != ' ' && c != '\r') hasChars = true;
        if (pos < 127) buf[pos++] = c;
      }
    }
  }

  if (hasChars && result.length() == 0 && current == target) {
    buf[pos] = '\0';
    result = String(buf);
    result.trim();
  }

  f.close();
  return result;
}

String getRandomPrescript(int* outIndex = nullptr) {
  int total = countStoredPrescripts();
  if (total == 0) {
    if (outIndex) *outIndex = -1;
    return "";
  }

  int target = random(0, total);
  if (outIndex) *outIndex = target;

  return getPrescriptByIndex(target);
}

bool parsePrescriptLine(const String& line, int& outDuration, bool& outRespond, String& outText) {
  int sep1 = line.indexOf('|');
  if (sep1 == -1 || sep1 > 4) {
    outDuration = 10;
    outRespond = true;
    outText = line;
    return true;
  }

  int sep2 = line.indexOf('|', sep1 + 1);
  if (sep2 != -1 && sep2 - sep1 <= 2) {
    outDuration = line.substring(0, sep1).toInt();
    outRespond = (line.substring(sep1 + 1, sep2) != "0");
    outText = line.substring(sep2 + 1);
  } else {
    outDuration = line.substring(0, sep1).toInt();
    outRespond = true;
    outText = line.substring(sep1 + 1);
  }

  if (outDuration <= 0) outDuration = 10;
  return outText.length() > 0;
}

#endif
