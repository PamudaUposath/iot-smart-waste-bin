// Smart Bin – ESP32 + 2x HC-SR04 + Blue/Red LEDs + Servo + MQTT + Node-RED permission lock
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <time.h>

// ---------- WiFi + MQTT (Set your own credentials here) ----------
const char* ssid        = "YOUR_WIFI_SSID";          // <-- Enter your WiFi name
const char* password    = "YOUR_WIFI_PASSWORD";      // <-- Enter your WiFi password

// Public MQTT broker OR your own private broker
const char* mqtt_server = "YOUR_MQTT_BROKER_URL";     // Example: "broker.hivemq.com"
const uint16_t mqtt_port = 1883;                      // Usually 1883 for MQTT

WiFiClient espClient;
PubSubClient client(espClient);

// ---------- MQTT Topics ----------
const char* BIN_ID             = "Bin1";
const char* T_TELE             = "smartbin/Bin1/tele";
const char* T_STATE            = "smartbin/Bin1/state";
const char* T_LOG              = "smartbin/Bin1/log";
const char* T_CMD_LOCK         = "smartbin/Bin1/cmd/lock";
const char* T_CMD_OPEN         = "smartbin/Bin1/cmd/open";
const char* T_CMD_CLOSE        = "smartbin/Bin1/cmd/close";

// ---------- Pin Definitions ----------
#define TRIG_PERSON 5
#define ECHO_PERSON 18
#define TRIG_BIN    23
#define ECHO_BIN    22
#define RED_LED     25
#define BLUE_LED    26
#define SERVO_PIN   19

// ---------- Bin Configuration ----------
const int   BIN_HEIGHT_CM        = 33;
const int   FULL_THRESH_PCT      = 90;
const int   PERSON_NEAR_CM       = 10;
const unsigned long LID_CLOSE_DELAY_MS = 5000;

const char* LOCATION_NAME   = "Jaffna Town";

// ---------- Servo Angles ----------
const int   LID_CLOSED_ANG  = 180;
const int   LID_OPEN_ANG    = 0;

// ---------- Internals ----------
Servo lidServo;
bool  lidOpen = false;
bool  userLock = false;
bool  fullLock = false;
bool  lidLocked = false;

unsigned long lastPersonSeenAt = 0;
String lastTriggerAt = "";

// ----------------------------------------------------
// ----------------- SENSOR FUNCTIONS -----------------
// ----------------------------------------------------
long readPulseCM(int trig, int echo) {
  digitalWrite(trig, LOW);  delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  unsigned long dur = pulseIn(echo, HIGH, 30000UL);
  if (dur == 0) return -1;
  return dur / 58;
}

long smoothCM(int trig, int echo, int samples = 3) {
  long sum = 0; int good = 0;
  for (int i = 0; i < samples; i++) {
    long d = readPulseCM(trig, echo);
    if (d > 0 && d < 400) { sum += d; good++; }
    delay(10);
  }
  return good ? (sum / good) : -1;
}

int computeFillPercent(long binDistCm) {
  if (binDistCm < 0) return 0;
  if (binDistCm > BIN_HEIGHT_CM) binDistCm = BIN_HEIGHT_CM;
  int filled = BIN_HEIGHT_CM - (int)binDistCm;
  int pct    = (filled * 100) / BIN_HEIGHT_CM;
  return constrain(pct, 0, 100);
}

// ----------------------------------------------------
// ---------------- LED / TIME FUNCTIONS --------------
// ----------------------------------------------------
void setLEDs(bool blueOn, bool redOn) {
  digitalWrite(BLUE_LED, blueOn ? HIGH : LOW);
  digitalWrite(RED_LED,  redOn  ? HIGH : LOW);
}

String niceLocalTime() {
  time_t now = time(nullptr);
  if (now < 1700000000) return "pending";
  struct tm tmLocal;
  localtime_r(&now, &tmLocal);
  char buf[30];
  strftime(buf, sizeof(buf), "%Y/%m/%d %I:%M %p", &tmLocal);
  return String(buf);
}

// ----------------------------------------------------
// ---------------- MQTT PAYLOAD BUILDER --------------
// ----------------------------------------------------
String buildPayload(int fillPct) {
  lidLocked = (userLock || fullLock);
  String ledColor = lidLocked ? "red" : "blue";

  String payload = "{";
  payload += "\"code\":\"" + String(BIN_ID) + "\"";
  payload += ",\"location\":\"" + String(LOCATION_NAME) + "\"";
  payload += ",\"fill\":" + String(fillPct);
  payload += ",\"locked\":" + (lidLocked ? "true" : "false");
  payload += ",\"led\":\"" + ledColor + "\"";
  payload += ",\"lastTriggerAt\":\"" + lastTriggerAt + "\"";
  payload += ",\"binHeight\":" + String(BIN_HEIGHT_CM);
  payload += "}";

  return payload;
}

// ----------------------------------------------------
// ---------------- WIFI / MQTT HANDLING --------------
// ----------------------------------------------------
void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) { delay(400); Serial.print("."); }
  Serial.println("\nWiFi connected.");
}

void ensureMQTT() {
  if (client.connected()) return;
  Serial.print("Connecting to MQTT...");
  String cid = "smartbin-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  while (!client.connect(cid.c_str())) { Serial.print("."); delay(500); }
  Serial.println("\nMQTT connected.");
  client.subscribe(T_CMD_LOCK);
  client.subscribe(T_CMD_OPEN);
  client.subscribe(T_CMD_CLOSE);
}

void publishLog(const String& s) {
  Serial.println(s);
  client.publish(T_LOG, s.c_str(), false);
}

void publishTele(int fillPct) {
  String p = buildPayload(fillPct);
  client.publish(T_TELE, p.c_str(), false);
  Serial.println("-> MQTT TELE: " + p);
}

// ----------------------------------------------------
// ----------------- SERVO CONTROL --------------------
// ----------------------------------------------------
void _servoClose() { lidServo.write(LID_CLOSED_ANG); lidOpen = false; }
void _servoOpen()  { lidServo.write(LID_OPEN_ANG);    lidOpen = true;  }

void tryOpenLid(int fillPct, const char* reason) {
  lidLocked = (userLock || fullLock);
  if (lidLocked) {
    publishLog(String("OPEN BLOCKED: ") + reason);
    lastTriggerAt = niceLocalTime();
    return;
  }
  _servoOpen();
  lastTriggerAt = niceLocalTime();
  publishLog("LID OPENED");
}

void closeLidAndReport(const char* reason, int fillPct) {
  _servoClose();
  lastTriggerAt = niceLocalTime();
  publishLog(String("LID CLOSED (") + reason + ")");
  publishTele(fillPct);
}

// ----------------------------------------------------
// ---------------- MQTT CALLBACK ---------------------
// ----------------------------------------------------
void onMqttMsg(char* topic, byte* payload, unsigned int length) {
  String t = String(topic);
  String pl;
  for (unsigned int i = 0; i < length; i++) pl += (char)payload[i];

  long binCm    = smoothCM(TRIG_BIN, ECHO_BIN);
  int  fillPct  = computeFillPercent(binCm);
  fullLock = (fillPct >= FULL_THRESH_PCT);

  if (t == T_CMD_LOCK) {
    String u = pl; u.toLowerCase();
    userLock = (u == "on" || u == "1" || u == "true");
    if (userLock && lidOpen) closeLidAndReport("userLock", fillPct);
    lastTriggerAt = niceLocalTime();
  }
  else if (t == T_CMD_OPEN)  tryOpenLid(fillPct,  "remoteOpen");
  else if (t == T_CMD_CLOSE) closeLidAndReport("remoteClose", fillPct);
}

// ----------------------------------------------------
// ---------------------- SETUP -----------------------
// ----------------------------------------------------
void setup() {
  pinMode(TRIG_PERSON, OUTPUT);
  pinMode(ECHO_PERSON, INPUT);
  pinMode(TRIG_BIN, OUTPUT);
  pinMode(ECHO_BIN, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  setLEDs(false, false);

  lidServo.attach(SERVO_PIN);
  _servoClose();

  Serial.begin(115200);
  delay(200);

  ensureWiFi();

  setenv("TZ", "LKT-5:30", 1);
  tzset();
  configTime(0, 0, "pool.ntp.org", "time.google.com");

  WiFi.mode(WIFI_STA);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(onMqttMsg);

  Serial.println("Smart Bin started.");
}

// ----------------------------------------------------
// ----------------------- LOOP -----------------------
// ----------------------------------------------------
void loop() {
  if (WiFi.status() != WL_CONNECTED) ensureWiFi();
  if (!client.connected()) ensureMQTT();
  client.loop();

  long personCm = smoothCM(TRIG_PERSON, ECHO_PERSON);
  long binCm    = smoothCM(TRIG_BIN, ECHO_BIN);
  int  fillPct  = computeFillPercent(binCm);

  fullLock = (fillPct >= FULL_THRESH_PCT);
  lidLocked = (userLock || fullLock);

  // Send full warning only once
  static bool wasFull = false;
  if (fullLock && !wasFull) {
    delay(5000);
    publishTele(fillPct);
    publishLog("Bin is 90% full!");
  }
  wasFull = fullLock;

  // LED Logic
  if (lidLocked)       setLEDs(false, true);
  else if (lidOpen)    setLEDs(true,  false);
  else                 setLEDs(false, false);

  bool personNear = (personCm > 0 && personCm <= PERSON_NEAR_CM);

  if (!lidLocked) {
    if (personNear) {
      lastPersonSeenAt = millis();
      if (!lidOpen) tryOpenLid(fillPct, "presence");
    } else if (lidOpen && millis() - lastPersonSeenAt >= LID_CLOSE_DELAY_MS) {
      closeLidAndReport("timer", fillPct);
    }
  } else if (lidOpen) {
    closeLidAndReport(fullLock ? "full" : "locked", fillPct);
  }

  Serial.printf("Person:%ldcm Bin:%ldcm Fill:%d%% lid:%s userLock:%s fullLock:%s\n",
    personCm, binCm, fillPct,
    lidOpen ? "OPEN" : "CLOSED",
    userLock ? "ON" : "OFF",
    fullLock ? "ON" : "OFF");

  delay(80);
}
