#ifndef STORAGE_H
#define STORAGE_H

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include "config.h"

using namespace Adafruit_LittleFS_Namespace;

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

void readSettings() {
  File f(SETTINGS_FILE, FILE_O_READ, InternalFS);
  if (f) {
    String content = f.readString();
    f.close();
    content.trim();
    if (content.length() > 0) {
      int c1 = content.indexOf(',');
      int c2 = content.indexOf(',', c1 + 1);
      int c3 = content.indexOf(',', c2 + 1);
      
      if (c1 == -1) {
        textScale = content.toInt();
      } else {
        textScale = content.substring(0, c1).toInt();
        scrambleDurationFrames = content.substring(c1 + 1, c2 == -1 ? content.length() : c2).toInt();
        if (c2 != -1) scrambleDelayMs = content.substring(c2 + 1, c3 == -1 ? content.length() : c3).toInt();
        if (c3 != -1) revealDelayMs = content.substring(c3 + 1).toInt();
      }
    }
  }
}

void writeSettings() {
  InternalFS.remove(SETTINGS_FILE);
  File f(SETTINGS_FILE, FILE_O_WRITE, InternalFS);
  if (f) {
    f.println(String(textScale) + "," + String(scrambleDurationFrames) + "," + String(scrambleDelayMs) + "," + String(revealDelayMs));
    f.close();
  }
}

void sendSettings() {
  if (!bleNotifyReady()) return;
  String msg = "RES:SETTINGS|MSG:" + String(textScale) + "," + String(scrambleDurationFrames) + "," + String(scrambleDelayMs) + "," + String(revealDelayMs) + "\n";
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
  File f(STATS_FILE, FILE_O_READ, InternalFS);
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
  InternalFS.remove(STATS_FILE);
  File f(STATS_FILE, FILE_O_WRITE, InternalFS);
  if (f) {
    f.println(String(s.achieved) + "," + String(s.failed) + "," + String(s.total));
    f.close();
  }
}

void sendStats() {
  if (!bleNotifyReady()) return;
  Stats s = readStatsFromFile();
  String msg = "RES:STATS|MSG:" + String(s.achieved) + "," + String(s.failed) + "," + String(s.total) + "\n";
  sendChunked(msg.c_str(), msg.length());
}

void addStats(String diffData) {
  Stats diff = parseCSV(diffData);
  if (diff.achieved > 0) Serial.println("PASSED");
  if (diff.failed > 0) Serial.println("FAILED");

  Stats curr = readStatsFromFile();
  curr.achieved += diff.achieved;
  curr.failed += diff.failed;
  curr.total += diff.total;

  writeStatsToFile(curr);
  sendStats();
}

void saveCustomRule(String rule) {
  File customsFile(CUSTOMS_FILE, FILE_O_WRITE, InternalFS);
  if (customsFile) {
    customsFile.seek(customsFile.size());
    customsFile.println(rule);
    customsFile.close();
  }
}

void sendCustomRules() {
  if (!bleNotifyReady()) return;

  File customsFile(CUSTOMS_FILE, FILE_O_READ, InternalFS);
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
  File f(CUSTOMS_FILE, FILE_O_READ, InternalFS);
  if (!f) return 0;

  int count = 0;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) count++;
  }
  f.close();
  return count;
}

String getRandomPrescript() {
  int total = countStoredPrescripts();
  if (total == 0) return "";

  int target = random(0, total);

  File f(CUSTOMS_FILE, FILE_O_READ, InternalFS);
  if (!f) return "";

  int current = 0;
  String result = "";
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    if (current == target) {
      result = line;
      break;
    }
    current++;
  }
  f.close();
  return result;
}

bool parsePrescriptLine(const String& line, int& outDuration, String& outText) {
  int sep = line.indexOf('|');
  if (sep == -1 || sep > 4) {
    outDuration = 10;
    outText = line;
    return true;
  }
  outDuration = line.substring(0, sep).toInt();
  if (outDuration <= 0) outDuration = 10;
  outText = line.substring(sep + 1);
  return outText.length() > 0;
}

#endif
