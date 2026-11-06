#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

const char* WIFI_SSID = "Airtel_megh_4783";
const char* WIFI_PASSWORD = "air86942";
const char* FIREBASE_URL = "https://esp32connect-daa17-default-rtdb.firebaseio.com";

#define ZMPT_PIN 35
#define RELAY_PIN 26
#define SDA_PIN 21
#define SCL_PIN 22

LiquidCrystal_I2C lcd(0x27, 16, 2);

const float VOLTAGE_CALIBRATION = 660.0;
const float VOLTAGE_OFF_THRESHOLD = 20.0;
const float BULB_POWER_W = 100.0;

const unsigned long LIVE_INTERVAL_MS = 1000;
const unsigned long HISTORY_INTERVAL_MS = 30000;
const unsigned long RELAY_POLL_INTERVAL_MS = 500;
const unsigned long WIFI_RETRY_INTERVAL_MS = 5000;

// Dashboard overload protection
const unsigned long OVERLOAD_OFF_TIME_MS = 10000;

bool overloadProtection = false;
unsigned long overloadOffUntil = 0;

float energyKWh = 0.0;
unsigned long lastEnergyMillis = 0;

const int AVG_SAMPLES = 60;
float powerSamples[AVG_SAMPLES];
int powerSampleIndex = 0;
int powerSampleCount = 0;

float regressionSlope = 0.0;
float regressionIntercept = 0.0;
float regressionNextPowerW = 0.0;

bool relayState = false;
float voltageV = 0.0;
float currentA = 0.0;
float currentMA = 0.0;
float powerW = 0.0;

unsigned long lastLiveMillis = 0;
unsigned long lastHistoryMillis = 0;
unsigned long lastRelayPollMillis = 0;
unsigned long lastWiFiRetryMillis = 0;

void addPowerSample(float p) {
  powerSamples[powerSampleIndex] = p;
  powerSampleIndex = (powerSampleIndex + 1) % AVG_SAMPLES;
  if (powerSampleCount < AVG_SAMPLES) powerSampleCount++;
}

float getAveragePower() {
  if (powerSampleCount == 0) return 0.0;

  float total = 0.0;
  for (int i = 0; i < powerSampleCount; i++) {
    total += powerSamples[i];
  }
  return total / powerSampleCount;
}

// Least-squares linear regression:
// x = recent sample number, y = measured power in watts.
void calculateLinearRegression() {
  if (powerSampleCount < 2) {
    regressionSlope = 0.0;
    regressionIntercept = getAveragePower();
    regressionNextPowerW = regressionIntercept;
    return;
  }

  double sumX = 0.0;
  double sumY = 0.0;
  double sumXY = 0.0;
  double sumX2 = 0.0;

  for (int i = 0; i < powerSampleCount; i++) {
    double x = i;
    double y = powerSamples[i];

    sumX += x;
    sumY += y;
    sumXY += x * y;
    sumX2 += x * x;
  }

  double n = powerSampleCount;
  double denominator = n * sumX2 - sumX * sumX;

  if (fabs(denominator) < 0.000001) {
    regressionSlope = 0.0;
    regressionIntercept = getAveragePower();
  } else {
    regressionSlope = (float)((n * sumXY - sumX * sumY) / denominator);
    regressionIntercept =
        (float)((sumY - regressionSlope * sumX) / n);
  }

  regressionNextPowerW =
      regressionIntercept + regressionSlope * powerSampleCount;

  if (regressionNextPowerW < 0.0) regressionNextPowerW = 0.0;
  if (regressionNextPowerW > 1000.0) regressionNextPowerW = 1000.0;
}

float get30DayForecastKWh() {
  calculateLinearRegression();
  return (regressionNextPowerW * 24.0 * 30.0) / 1000.0;
}

bool connectWiFi(unsigned long timeoutMs) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting WiFi");

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < timeoutMs) {
    delay(250);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
    return true;
  }

  Serial.println("WiFi not connected.");
  return false;
}

bool firebasePUT(const String& path, const String& json) {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  String url = String(FIREBASE_URL) + path;

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int code = http.PUT(json);
  bool ok = (code >= 200 && code < 300);

  if (!ok) {
    Serial.print("Firebase PUT failed: ");
    Serial.println(code);
  }

  http.end();
  return ok;
}

String firebaseGET(const String& path) {
  if (WiFi.status() != WL_CONNECTED) return "";

  HTTPClient http;
  String url = String(FIREBASE_URL) + path;

  http.begin(url);
  int code = http.GET();

  String payload = "";

  if (code >= 200 && code < 300) {
    payload = http.getString();
  }

  http.end();
  return payload;
}

bool jsonBoolValue(const String& json, bool &value) {
  String s = json;
  s.trim();

  if (s == "true") {
    value = true;
    return true;
  }

  if (s == "false") {
    value = false;
    return true;
  }

  return false;
}

float readVoltageRMS() {
  const int samples = 1000;

  double sum = 0.0;
  double sumSq = 0.0;

  for (int i = 0; i < samples; i++) {
    int adc = analogRead(ZMPT_PIN);
    sum += adc;
    sumSq += (double)adc * (double)adc;
    delayMicroseconds(200);
  }

  double mean = sum / samples;
  double variance = (sumSq / samples) - (mean * mean);

  if (variance < 0) variance = 0;

  double rmsCounts = sqrt(variance);
  double rmsADC = rmsCounts * (3.3 / 4095.0);

  float measuredVoltage = rmsADC * VOLTAGE_CALIBRATION;

  if (measuredVoltage < VOLTAGE_OFF_THRESHOLD) {
    measuredVoltage = 0.0;
  }

  return measuredVoltage;
}

void setRelay(bool state) {
  relayState = state;
  digitalWrite(RELAY_PIN, state ? HIGH : LOW);
}

void forceDisconnectedState() {
  // WiFi loss means live data is considered invalid.
  voltageV = 0.0;
  currentA = 0.0;
  currentMA = 0.0;
  powerW = 0.0;

  // Prevent the bulb from remaining ON when communication is lost.
  setRelay(false);
}

void activateOverloadProtection() {
  overloadProtection = true;
  overloadOffUntil = millis() + OVERLOAD_OFF_TIME_MS;

  // Immediately switch bulb OFF.
  setRelay(false);

  Serial.println("OVERLOAD PROTECTION: BULB OFF FOR 10 SECONDS");
}

void updateOverloadProtection() {
  if (overloadProtection &&
      (long)(millis() - overloadOffUntil) >= 0) {

    overloadProtection = false;

    // Automatically turn bulb ON after 10 seconds.
    setRelay(true);

    // Keep the normal Relay widget synchronized with the physical relay.
    firebasePUT("/device/relay.json", "true");

    Serial.println("OVERLOAD PROTECTION: 10 SECONDS COMPLETE - BULB ON");
  }
}

void checkDashboardOverloadTrigger() {
  String payload = firebaseGET("/device/overload_trigger.json");

  bool trigger = false;

  if (jsonBoolValue(payload, trigger) && trigger) {
    // Consume the trigger so one button press causes one cycle.
    firebasePUT("/device/overload_trigger.json", "false");

    // Link the overload control to the normal relay control.
    firebasePUT("/device/relay.json", "false");

    activateOverloadProtection();
  }
}

void updateLCD() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("V:");
  lcd.print(voltageV, 1);
  lcd.print(" I:");
  lcd.print((int)round(currentMA));
  lcd.print("mA");

  lcd.setCursor(0, 1);
  lcd.print("P:");
  lcd.print(powerW, 1);
  lcd.print("W ");
  lcd.print(relayState ? "ON " : "OFF");
}

void uploadLive() {
  float avgPowerW = getAveragePower();
  float forecast30d = get30DayForecastKWh();

  String json = "{";
  json += "\"voltage\":" + String(voltageV, 2) + ",";
  json += "\"current\":" + String(currentA, 4) + ",";
  json += "\"current_ma\":" + String(currentMA, 1) + ",";
  json += "\"power_w\":" + String(powerW, 2) + ",";
  json += "\"energy_kwh\":" + String(energyKWh, 6) + ",";
  json += "\"relay\":" + String(relayState ? "true" : "false") + ",";
  json += "\"average_power_w\":" + String(avgPowerW, 2) + ",";
  json += "\"regression_slope\":" + String(regressionSlope, 5) + ",";
  json += "\"regression_next_power_w\":" + String(regressionNextPowerW, 2) + ",";
  json += "\"forecast_30d_kwh\":" + String(forecast30d, 3) + ",";
  json += "\"wifi_connected\":true,";
  json += "\"overload_protection\":" + String(overloadProtection ? "true" : "false") + ",";
  json += "\"timestamp_ms\":" + String(millis());
  json += "}";

  firebasePUT("/device/live.json", json);

  String forecastJson = "{";
  forecastJson += "\"forecast_30d_kwh\":" + String(forecast30d, 3) + ",";
  forecastJson += "\"average_power_w\":" + String(avgPowerW, 2) + ",";
  forecastJson += "\"regression_slope\":" + String(regressionSlope, 5) + ",";
  forecastJson += "\"regression_next_power_w\":" + String(regressionNextPowerW, 2) + ",";
  forecastJson += "\"method\":\"linear_regression\",";
  forecastJson += "\"assumption\":\"linear trend in recent power continues for 30 days\"";
  forecastJson += "}";

  firebasePUT("/device/forecast_kwh.json", forecastJson);
}

void uploadHistory() {
  calculateLinearRegression();

  String key = String(millis());

  String json = "{";
  json += "\"voltage\":" + String(voltageV, 2) + ",";
  json += "\"current\":" + String(currentA, 4) + ",";
  json += "\"current_ma\":" + String(currentMA, 1) + ",";
  json += "\"power_w\":" + String(powerW, 2) + ",";
  json += "\"energy_kwh\":" + String(energyKWh, 6) + ",";
  json += "\"relay\":" + String(relayState ? "true" : "false") + ",";
  json += "\"average_power_w\":" + String(getAveragePower(), 2) + ",";
  json += "\"regression_slope\":" + String(regressionSlope, 5) + ",";
  json += "\"regression_next_power_w\":" + String(regressionNextPowerW, 2) + ",";
  json += "\"forecast_30d_kwh\":" + String(get30DayForecastKWh(), 3) + ",";
  json += "\"wifi_connected\":true,";
  json += "\"overload_protection\":" + String(overloadProtection ? "true" : "false") + ",";
  json += "\"timestamp_ms\":" + String(millis());
  json += "}";

  firebasePUT("/device/history/" + key + ".json", json);
}

void pollRelayCommand() {
  // The overload cycle has exclusive control of the relay for 10 seconds.
  if (overloadProtection) return;

  String payload = firebaseGET("/device/relay.json");

  bool requestedState;

  if (jsonBoolValue(payload, requestedState)) {
    if (requestedState != relayState) {
      setRelay(requestedState);
      Serial.print("Relay changed from Firebase: ");
      Serial.println(relayState ? "ON" : "OFF");
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(ZMPT_PIN, ADC_11db);

  Wire.begin(SDA_PIN, SCL_PIN);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Energy");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(1500);

  for (int i = 0; i < AVG_SAMPLES; i++) {
    powerSamples[i] = 0.0;
  }

  bool wifiOK = connectWiFi(10000);

  if (!wifiOK) {
    forceDisconnectedState();
  }

  lastEnergyMillis = millis();

  updateLCD();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(wifiOK ? "WiFi Connected" : "WiFi Offline");
  lcd.setCursor(0, 1);
  lcd.print(wifiOK ? "Monitoring..." : "Voltage = 0V");
  delay(1000);
}

void loop() {
  unsigned long now = millis();

  // Keep the 10-second overload cycle running independently.
  updateOverloadProtection();

  // IMPORTANT:
  // If WiFi disconnects, immediately treat the live measurement as invalid.
  if (WiFi.status() != WL_CONNECTED) {
    forceDisconnectedState();

    if (now - lastWiFiRetryMillis >= WIFI_RETRY_INTERVAL_MS) {
      lastWiFiRetryMillis = now;
      connectWiFi(3000);
    }

    updateLCD();
  }

  if (WiFi.status() == WL_CONNECTED) {

    if (now - lastRelayPollMillis >= RELAY_POLL_INTERVAL_MS) {
      lastRelayPollMillis = now;

      // Normal relay command
      pollRelayCommand();

      // Dashboard overload button
      if (!overloadProtection) {
        checkDashboardOverloadTrigger();
      }
    }

    if (now - lastLiveMillis >= LIVE_INTERVAL_MS) {
      lastLiveMillis = now;

      voltageV = readVoltageRMS();

      if (!relayState || voltageV <= VOLTAGE_OFF_THRESHOLD) {
        currentA = 0.0;
        currentMA = 0.0;
        powerW = 0.0;
      } else {
        currentA = BULB_POWER_W / voltageV;
        currentMA = currentA * 1000.0;
        powerW = voltageV * currentA;
      }

      unsigned long elapsed = now - lastEnergyMillis;
      lastEnergyMillis = now;

      energyKWh +=
          (powerW * ((float)elapsed / 3600000.0)) / 1000.0;

      addPowerSample(powerW);

      updateLCD();
      uploadLive();

      Serial.print("V=");
      Serial.print(voltageV, 1);
      Serial.print(" V | I=");
      Serial.print(currentMA, 0);
      Serial.print(" mA | P=");
      Serial.print(powerW, 1);
      Serial.print(" W | E=");
      Serial.print(energyKWh, 6);
      Serial.print(" kWh | Avg=");
      Serial.print(getAveragePower(), 1);
      Serial.print(" W | Regression next=");
      Serial.print(regressionNextPowerW, 1);
      Serial.print(" W | 30D=");
      Serial.print(get30DayForecastKWh(), 3);
      Serial.println(" kWh");
    }

    if (now - lastHistoryMillis >= HISTORY_INTERVAL_MS) {
      lastHistoryMillis = now;
      uploadHistory();
    }
  }

  delay(5);
}
