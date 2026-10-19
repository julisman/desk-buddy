// claude_link.cpp — HTTP server that receives Claude Code status (see claude_link.h).
#include "claude_link.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_http_server.h>

#include <mutex>

#include "app_config.h"
#include "log.h"

struct Session {
  bool used = false;
  char id[48] = "";
  char project[24] = "";
  char model[24] = "";
  char mode[16] = "";
  char tool[24] = "";
  float ctxPct = -1;
  ClaudeActivity activity = ClaudeActivity::None;
  uint32_t turnStartMs = 0;  // millis() when the last prompt was sent
  uint32_t updatedMs = 0;
};

static std::mutex stateMutex;  // handlers run in the httpd task, read from loop()
static Session sessions[CLAUDE_MAX_SESSIONS];
static int selected = -1;      // session slot being shown
static bool valid = false;
static float sessionPct = -1, weekPct = -1;
static uint32_t sessionResets = 0, weekResets = 0;
static uint8_t pendingEvents = 0;
static httpd_handle_t server = nullptr;

static void copyStr(char *dst, size_t size, JsonVariantConst v) {
  if (v.is<const char *>()) strlcpy(dst, v.as<const char *>(), size);
}

static void readNum(float &dst, JsonVariantConst v) {
  if (!v.isNull()) dst = v.as<float>();
}

// ---- Session slots (all functions below are called with stateMutex held) ----

static int mostRecentSlot() {
  int best = -1;
  for (int i = 0; i < CLAUDE_MAX_SESSIONS; i++) {
    if (sessions[i].used && (best < 0 || sessions[i].updatedMs - sessions[best].updatedMs < 0x80000000u))
      best = i;
  }
  return best;
}

static void removeSlot(int slot) {
  sessions[slot] = Session();
  if (selected == slot) selected = mostRecentSlot();
}

// Sessions silent for a long time are treated as closed (SessionEnd can be missed if the laptop sleeps)
static void pruneStale(uint32_t now) {
  for (int i = 0; i < CLAUDE_MAX_SESSIONS; i++) {
    if (sessions[i].used && now - sessions[i].updatedMs > CLAUDE_SESSION_TIMEOUT_MS) removeSlot(i);
  }
}

static int findOrCreate(const char *id, uint32_t now) {
  for (int i = 0; i < CLAUDE_MAX_SESSIONS; i++) {
    if (sessions[i].used && strcmp(sessions[i].id, id) == 0) return i;
  }
  int slot = -1;
  for (int i = 0; i < CLAUDE_MAX_SESSIONS && slot < 0; i++) {
    if (!sessions[i].used) slot = i;
  }
  if (slot < 0) {  // full: replace the one idle the longest
    slot = 0;
    for (int i = 1; i < CLAUDE_MAX_SESSIONS; i++) {
      if (now - sessions[i].updatedMs > now - sessions[slot].updatedMs) slot = i;
    }
  }
  sessions[slot] = Session();
  sessions[slot].used = true;
  strlcpy(sessions[slot].id, id, sizeof(sessions[slot].id));
  if (selected < 0 || !sessions[selected].used) selected = slot;
  LOG("[claude] new session in slot %d\n", slot);
  return slot;
}

static void applyHook(int slot, JsonDocument &doc, uint32_t now) {
  Session &s = sessions[slot];
  const char *event = doc["event"] | "";
  const char *notification = doc["notification"] | "";
  copyStr(s.mode, sizeof(s.mode), doc["mode"]);
  pendingEvents |= CLAUDE_EV_ACTIVITY;

  bool needsYou = strcmp(event, "PermissionRequest") == 0 ||
                  (strcmp(event, "Notification") == 0 &&
                   (strcmp(notification, "permission_prompt") == 0 ||
                    strcmp(notification, "agent_needs_input") == 0 ||
                    strncmp(notification, "elicitation", 11) == 0));

  if (needsYou) {
    if (s.activity != ClaudeActivity::NeedsYou) pendingEvents |= CLAUDE_EV_NEEDS_YOU;
    copyStr(s.tool, sizeof(s.tool), doc["tool"]);
    s.activity = ClaudeActivity::NeedsYou;
    selected = slot;  // show the session that needs you
  } else if (strcmp(event, "UserPromptSubmit") == 0) {
    pendingEvents |= CLAUDE_EV_WORKING;
    s.turnStartMs = now;
    s.activity = ClaudeActivity::Working;
    selected = slot;  // the session you just typed in
  } else if (strcmp(event, "PostToolUse") == 0) {
    s.activity = ClaudeActivity::Working;  // tool ran = permission granted / still working
  } else if (strcmp(event, "Stop") == 0) {
    // Measured from the prompt (not the last permission), so long tasks still get the sound
    if (s.activity == ClaudeActivity::Working && s.turnStartMs && now - s.turnStartMs >= CLAUDE_CHIRP_MIN_MS) {
      pendingEvents |= CLAUDE_EV_DONE;
    }
    s.activity = ClaudeActivity::Done;
  } else if (strcmp(event, "Notification") == 0 && strcmp(notification, "idle_prompt") == 0) {
    s.activity = ClaudeActivity::Idle;
  } else if (strcmp(event, "SessionStart") == 0) {
    s.activity = ClaudeActivity::Idle;
  }
  LOG("[claude] slot %d hook %s%s%s -> %d\n", slot, event, *notification ? " " : "", notification,
      (int)s.activity);
}

static void applyStatus(int slot, JsonDocument &doc) {
  Session &s = sessions[slot];
  copyStr(s.model, sizeof(s.model), doc["model"]);
  readNum(s.ctxPct, doc["ctx"]);
  // Limits belong to the account: latest value from any session
  readNum(sessionPct, doc["session_pct"]);
  readNum(weekPct, doc["week_pct"]);
  if (!doc["session_resets"].isNull()) sessionResets = doc["session_resets"].as<uint32_t>();
  if (!doc["week_resets"].isNull()) weekResets = doc["week_resets"].as<uint32_t>();
}

// ---- HTTP ----

static esp_err_t postHandler(httpd_req_t *req) {
  char body[768];
  if (req->content_len >= sizeof(body)) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "body too large");
    return ESP_FAIL;
  }
  int got = 0;
  while (got < (int)req->content_len) {
    int r = httpd_req_recv(req, body + got, req->content_len - got);
    if (r <= 0) return ESP_FAIL;
    got += r;
  }
  body[got] = 0;

  JsonDocument doc;
  if (deserializeJson(doc, body)) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");
    return ESP_FAIL;
  }

  uint32_t now = millis();
  {
    std::lock_guard<std::mutex> lock(stateMutex);
    pruneStale(now);
    const char *id = doc["session"] | "default";
    const char *type = doc["type"] | "";
    const char *event = doc["event"] | "";
    if (strcmp(type, "hook") == 0 && strcmp(event, "SessionEnd") == 0) {
      for (int i = 0; i < CLAUDE_MAX_SESSIONS; i++) {
        if (sessions[i].used && strcmp(sessions[i].id, id) == 0) {
          removeSlot(i);
          LOG("[claude] slot %d ended (SessionEnd)\n", i);
        }
      }
    } else {
      int slot = findOrCreate(id, now);
      copyStr(sessions[slot].project, sizeof(sessions[slot].project), doc["project"]);
      if (strcmp(type, "hook") == 0) applyHook(slot, doc, now);
      else applyStatus(slot, doc);
      sessions[slot].updatedMs = now;
    }
    valid = true;
  }
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_sendstr(req, "{\"ok\":true}");
}

// GET /claude: view current state (debug from the laptop)
static esp_err_t getHandler(httpd_req_t *req) {
  char json[1024];
  int n;
  uint32_t now = millis();
  {
    std::lock_guard<std::mutex> lock(stateMutex);
    n = snprintf(json, sizeof(json),
                 "{\"session_pct\":%.1f,\"session_resets\":%lu,\"week_pct\":%.1f,\"week_resets\":%lu,"
                 "\"selected\":%d,\"sessions\":[",
                 sessionPct, (unsigned long)sessionResets, weekPct, (unsigned long)weekResets, selected);
    bool first = true;
    for (int i = 0; i < CLAUDE_MAX_SESSIONS && n < (int)sizeof(json) - 200; i++) {
      const Session &s = sessions[i];
      if (!s.used) continue;
      n += snprintf(json + n, sizeof(json) - n,
                    "%s{\"slot\":%d,\"project\":\"%s\",\"model\":\"%s\",\"mode\":\"%s\",\"ctx\":%.1f,"
                    "\"activity\":%d,\"age_s\":%lu}",
                    first ? "" : ",", i, s.project, s.model, s.mode, s.ctxPct, (int)s.activity,
                    (unsigned long)((now - s.updatedMs) / 1000));
      first = false;
    }
  }
  snprintf(json + n, sizeof(json) - n, "]}");
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_sendstr(req, json);
}

void claudeLinkStart() {
  if (server) return;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.stack_size = 6144;  // JsonDocument + body buffer on the handler stack
  if (httpd_start(&server, &config) != ESP_OK) {
    LOG("[claude] server failed to start\n");
    server = nullptr;
    return;
  }
  httpd_uri_t post = {};
  post.uri = "/claude";
  post.method = HTTP_POST;
  post.handler = postHandler;
  httpd_register_uri_handler(server, &post);

  httpd_uri_t get = {};
  get.uri = "/claude";
  get.method = HTTP_GET;
  get.handler = getHandler;
  httpd_register_uri_handler(server, &get);
  LOG("[claude] server ready: POST http://%s.local/claude\n", NET_HOSTNAME);
}

bool claudeLinkStarted() { return server != nullptr; }

ClaudeState claudeState() {
  std::lock_guard<std::mutex> lock(stateMutex);
  pruneStale(millis());
  ClaudeState st;
  st.valid = valid;
  st.sessionPct = sessionPct;
  st.weekPct = weekPct;
  st.sessionResets = sessionResets;
  st.weekResets = weekResets;
  for (int i = 0; i < CLAUDE_MAX_SESSIONS; i++) {
    const Session &s = sessions[i];
    if (!s.used) continue;
    if (i == selected) {
      st.index = st.count;
      strlcpy(st.project, s.project, sizeof(st.project));
      strlcpy(st.model, s.model, sizeof(st.model));
      strlcpy(st.mode, s.mode, sizeof(st.mode));
      strlcpy(st.tool, s.tool, sizeof(st.tool));
      st.ctxPct = s.ctxPct;
      st.activity = s.activity;
      st.updatedMs = s.updatedMs;
    }
    st.activities[st.count++] = s.activity;
  }
  return st;
}

uint8_t claudeTakeEvents() {
  std::lock_guard<std::mutex> lock(stateMutex);
  uint8_t e = pendingEvents;
  pendingEvents = 0;
  return e;
}

void claudeSelect(int delta) {
  std::lock_guard<std::mutex> lock(stateMutex);
  if (selected < 0) return;
  for (int step = 1; step <= CLAUDE_MAX_SESSIONS; step++) {
    int slot = ((selected + delta * step) % CLAUDE_MAX_SESSIONS + CLAUDE_MAX_SESSIONS) % CLAUDE_MAX_SESSIONS;
    if (sessions[slot].used) {
      selected = slot;
      return;
    }
  }
}
