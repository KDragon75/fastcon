/*
  PairKnownDevice.ino

  Known working FastCon/brMesh pairing workflow.

  IMPORTANT:
    Pairing only works during the bulb boot/pairing window.

  Physical steps:
    1. Flash this sketch.
    2. Open Serial Monitor at 115200.
    3. Type: pair
    4. Immediately power on/reset the bulb.
    5. If the bulb blinks, provisioning was accepted.
    6. Wait for pairing loop to finish.
    7. Type: on
*/

#include <Arduino.h>
#include <FastConSimple.h>

FastConSimple light;

// From confirmed phone pairing capture:
// provision payload:
//   E8 70 72 88 9D D4 01 01 32 39 30 37
static const uint8_t DEVICE_ID[6] = {
  0xE8, 0x70, 0x72, 0x88, 0x9D, 0xD4
};

static String readLine() {
  static String buffer;

  while (Serial.available()) {
    char c = static_cast<char>(Serial.read());

    if (c == '\r') continue;

    if (c == '\n') {
      String line = buffer;
      buffer = "";
      line.trim();
      return line;
    }

    buffer += c;
  }

  return "";
}

static void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  pair        - send provision packets for 15 seconds");
  Serial.println("  paironce    - send one provision packet");
  Serial.println("  on");
  Serial.println("  off");
  Serial.println("  red");
  Serial.println("  green");
  Serial.println("  blue");
  Serial.println("  white");
  Serial.println();
  Serial.println("Pairing workflow:");
  Serial.println("  1. Type: pair");
  Serial.println("  2. Immediately power on/reset the bulb");
  Serial.println("  3. If the bulb blinks, provisioning was accepted");
  Serial.println("  4. Wait until pair finishes");
  Serial.println("  5. Type: on");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("FastConSimple PairKnownDevice example");

  light.setDebug(true);
  light.begin("FastConPair");

  light.loadConfig();

  // Configure the known-working provision values.
  light.setDeviceId(DEVICE_ID);
  light.setLightId(1);
  light.setGroupId(1);
  light.setMeshKeyAscii("2907");
  light.saveConfig();

  printHelp();
}

void loop() {
  String line = readLine();

  if (line.length() == 0) return;

  if (line == "help" || line == "?") {
    printHelp();
  } else if (line == "pair") {
    Serial.println("Starting 15 second provision loop. Power on/reset the bulb now.");
    light.pairKnownDevice(15000, 250);
    Serial.println("Pair loop finished. If the bulb blinked, try: on");
  } else if (line == "paironce") {
    light.sendProvisionOnce();
  } else if (line == "on") {
    light.on();
  } else if (line == "off") {
    light.off();
  } else if (line == "red") {
    light.setRGB(255, 0, 0);
  } else if (line == "green") {
    light.setRGB(0, 255, 0);
  } else if (line == "blue") {
    light.setRGB(0, 0, 255);
  } else if (line == "white") {
    light.setWhite();
  } else {
    Serial.println("Unknown command.");
    printHelp();
  }
}
