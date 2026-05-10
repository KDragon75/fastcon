/*
  HeadlessButtonPairing.ino

  Button-based known-device FastCon pairing.

  Hardware:
    Button on GPIO 9, active low, INPUT_PULLUP
    Status LED on GPIO 15
    Optional ARGB on GPIO 8 using rgbLedWrite()

  Behavior:
    Hold button 2 seconds, release:
      start 15-second pairing loop

    Hold button 10 seconds, release:
      clear saved config and reboot

  Pairing workflow:
    1. Hold button for 2 seconds and release.
    2. Immediately power on/reset the bulb.
    3. LED blinks during provision loop.
    4. If the bulb blinks, provisioning was accepted.
    5. When LED stops, test control.
*/

#include <Arduino.h>
#include <FastConSimple.h>

#define USE_ARGB_STATUS 1

static const uint8_t BUTTON_PIN = 9;
static const uint8_t STATUS_LED_PIN = 15;
static const uint8_t ARGB_STATUS_PIN = 8;

static const uint32_t PAIR_BUTTON_MS = 2000;
static const uint32_t RESET_BUTTON_MS = 10000;
static const uint32_t PAIR_TIMEOUT_MS = 15000;
static const uint32_t PAIR_INTERVAL_MS = 250;
static const uint32_t BLINK_MS = 75;

FastConSimple light;

static bool buttonWasDown = false;
static uint32_t buttonDownAt = 0;

static bool pairingActive = false;
static uint32_t pairingEndAt = 0;
static uint32_t lastPairAt = 0;
static uint32_t lastBlinkAt = 0;
static bool blinkState = false;

static const uint8_t DEVICE_ID[6] = {
  0xE8, 0x70, 0x72, 0x88, 0x9D, 0xD4
};

static void setArgb(uint8_t r, uint8_t g, uint8_t b) {
#if USE_ARGB_STATUS
  rgbLedWrite(ARGB_STATUS_PIN, r, g, b);
#else
  (void)r;
  (void)g;
  (void)b;
#endif
}

static void statusIdle() {
  digitalWrite(STATUS_LED_PIN, LOW);
  setArgb(0, 0, 0);
}

static void statusBlue(bool on) {
  digitalWrite(STATUS_LED_PIN, on ? HIGH : LOW);
  if (on) setArgb(0, 0, 80);
  else setArgb(0, 0, 0);
}

static void statusRedSolid() {
  digitalWrite(STATUS_LED_PIN, HIGH);
  setArgb(80, 0, 0);
}

static bool buttonDown() {
  return digitalRead(BUTTON_PIN) == LOW;
}

static void startPairing() {
  pairingActive = true;
  pairingEndAt = millis() + PAIR_TIMEOUT_MS;
  lastPairAt = 0;
  lastBlinkAt = 0;
  blinkState = false;

  light.resetSequence(1);

  Serial.println("Headless pairing started. Power on/reset bulb now.");
}

static void stopPairing() {
  pairingActive = false;
  light.resetSequence(1);
  statusIdle();

  Serial.println("Headless pairing timed out. If the bulb blinked, test control.");
}

static void updatePairing() {
  if (!pairingActive) return;

  uint32_t now = millis();

  if (now >= pairingEndAt) {
    stopPairing();
    return;
  }

  if (now - lastBlinkAt >= BLINK_MS) {
    lastBlinkAt = now;
    blinkState = !blinkState;
    statusBlue(blinkState);
  }

  if (now - lastPairAt >= PAIR_INTERVAL_MS) {
    lastPairAt = now;
    light.sendProvisionOnce();
  }
}

static void clearAndReboot() {
  Serial.println("Factory reset config and reboot.");
  light.clearConfig();

  for (int i = 0; i < 6; i++) {
    statusRedSolid();
    delay(100);
    statusIdle();
    delay(100);
  }

  ESP.restart();
}

static void updateButton() {
  bool down = buttonDown();
  uint32_t now = millis();

  if (down && !buttonWasDown) {
    buttonWasDown = true;
    buttonDownAt = now;
    return;
  }

  if (down && buttonWasDown) {
    uint32_t held = now - buttonDownAt;

    if (!pairingActive) {
      if (held >= RESET_BUTTON_MS) {
        statusRedSolid();
      } else if (held >= PAIR_BUTTON_MS) {
        statusBlue(true);
      }
    }

    return;
  }

  if (!down && buttonWasDown) {
    uint32_t held = now - buttonDownAt;
    buttonWasDown = false;

    if (held >= RESET_BUTTON_MS) {
      clearAndReboot();
      return;
    }

    if (held >= PAIR_BUTTON_MS) {
      startPairing();
      return;
    }

    if (!pairingActive) {
      statusIdle();
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED_PIN, OUTPUT);
  statusIdle();

  Serial.println();
  Serial.println("FastConSimple HeadlessButtonPairing example");

  light.setDebug(true);
  light.begin("FastConButton");
  light.loadConfig();

  light.setDeviceId(DEVICE_ID);
  light.setLightId(1);
  light.setGroupId(1);
  light.setMeshKeyAscii("2907");
  light.saveConfig();

  Serial.println("Hold button 2s to pair. Hold 10s to clear settings.");
}

void loop() {
  updateButton();
  updatePairing();
}
