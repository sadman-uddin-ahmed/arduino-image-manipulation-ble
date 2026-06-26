#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>

// ================= OLED CONFIGURATION =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// ================= PIN DEFINITIONS =================
#define JOY_X A0
#define JOY_Y A1
#define BT_RX 2
#define BT_TX 3
#define LED_PIN 6

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
SoftwareSerial BT(BT_RX, BT_TX);

// ================= LETTER BITMAP DATA =================
const byte letters[8][5] = {
  {B0111111, B0001001, B0001001, B0001001, B0000001}, // F
  {B0011110, B0100001, B0101001, B0101001, B0111010}, // G
  {B0010000, B0100000, B0100001, B0011111, B0000001}, // J
  {B0111111, B0001000, B0010100, B0100010, B0000001}, // K
  {B0111111, B0100000, B0100000, B0100000, B0100000}, // L
  {B0111111, B0001001, B0001001, B0001001, B0000110}, // P
  {B0011110, B0100001, B0101001, B0010001, B0101110}, // Q
  {B0111111, B0001001, B0011001, B0010101, B0100010}  // R
};

const char letterNames[8] = {'F','G','J','K','L','P','Q','R'};

// ================= LOCAL IMAGE STATE =================
int txLetter = 0;        // Local/current selected letter
int txAngleIndex = 0;    // 0,1,2,3 -> 0,90,180,270
int txX = 15;            // Local image X position
int txY = 24;            // Local image Y position

// ================= RECEIVED IMAGE STATE =================
int rxLetter = 1;        // Received/stored letter
int rxAngle = 0;         // Received angle in degrees
int rxX = 78;            // Received image X position
int rxY = 24;            // Received image Y position

const int scale = 2;

// ================= JOYSTICK CHARACTERISTICS =================
// Based on the measured joystick values
const int centerX = 512;
const int centerY = 506;
const int MOVE_OFFSET = 70;    // Moderate tilt = movement
const int HOLD_OFFSET = 190;   // Strong tilt = special action

// ================= HOLD TIMERS =================
unsigned long rightHoldStart = 0;
unsigned long leftHoldStart  = 0;
unsigned long downHoldStart  = 0;

bool rightTriggered = false;
bool leftTriggered  = false;
bool downTriggered  = false;

// ================= LED TIMER =================
// LED stays ON until millis() reaches ledOffTime
unsigned long ledOffTime = 0;

// ================= COMMUNICATION STATE =================
String statusText = "Ready";
String inputBuffer = "";

// ================= HELPER: LETTER CHAR TO INDEX =================
int findLetterIndex(char c) {
  for (int i = 0; i < 8; i++) {
    if (letterNames[i] == c) return i;
  }
  return 0; // Default to F if unknown
}

// ================= FUNCTION: TURN LED ON FOR 2 SECONDS =================
void triggerLED() {
  digitalWrite(LED_PIN, HIGH);     // Turn LED ON
  ledOffTime = millis() + 2000;    // Keep it ON for 2000 ms
}

// ================= FUNCTION: TURN LED OFF WHEN TIME EXPIRES =================
void updateLED() {
  if (ledOffTime != 0 && millis() >= ledOffTime) {
    digitalWrite(LED_PIN, LOW);    // Turn LED OFF
    ledOffTime = 0;
  }
}

// ================= FUNCTION: DRAW ROTATED LETTER =================
void drawRotatedLetter5x7(const byte letter[5], int x, int y, int scale, int angle) {
  for (int col = 0; col < 5; col++) {
    for (int row = 0; row < 7; row++) {
      if (letter[col] & (1 << row)) {
        int px, py;

        if (angle == 0) {
          px = x + col * scale;
          py = y + row * scale;
        }
        else if (angle == 90) {
          px = x + (6 - row) * scale;
          py = y + col * scale;
        }
        else if (angle == 180) {
          px = x + (4 - col) * scale;
          py = y + (6 - row) * scale;
        }
        else { // 270 degrees
          px = x + row * scale;
          py = y + (4 - col) * scale;
        }

        display.fillRect(px, py, scale, scale, SSD1306_WHITE);
      }
    }
  }
}

// ================= FUNCTION: UPDATE OLED DISPLAY =================
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Local/transmit image info
  display.setCursor(0, 0);
  display.print("TX ");
  display.print(letterNames[txLetter]);
  display.print(" ");
  display.print(txAngleIndex * 90);

  // Received image info
  display.setCursor(68, 0);
  display.print("RX ");
  display.print(letterNames[rxLetter]);
  display.print(" ");
  display.print(rxAngle);

  // Draw local image on left half
  drawRotatedLetter5x7(letters[txLetter], txX, txY, scale, txAngleIndex * 90);

  // Draw received image on right half
  drawRotatedLetter5x7(letters[rxLetter], rxX, rxY, scale, rxAngle);

  // Status line
  display.setCursor(0, 56);
  display.print(statusText);

  display.display();
}

// ================= FUNCTION: SEND LOCAL DATA TO ANDROID =================
// Packet format: (F,30,20,180)
void sendCurrentData() {
  String msg = "(";
  msg += letterNames[txLetter];
  msg += ",";
  msg += String(txX);
  msg += ",";
  msg += String(txY);
  msg += ",";
  msg += String(txAngleIndex * 90);
  msg += ")";

  BT.println(msg);        // Send data to Android
  statusText = "Sent";
  triggerLED();           // LED ON for 2 seconds on send
  updateDisplay();
}

// ================= FUNCTION: PARSE INCOMING PACKET =================
// Expected format from Android: (F,30,20,180)
void parseIncoming(String line) {
  line.trim();

  // Packet must start with '(' and end with ')'
  if (!line.startsWith("(") || !line.endsWith(")")) {
    statusText = "Bad format";
    updateDisplay();
    return;
  }

  // Remove opening and closing brackets
  line.remove(0, 1);
  line.remove(line.length() - 1);

  // Find comma positions
  int p1 = line.indexOf(',');
  int p2 = line.indexOf(',', p1 + 1);
  int p3 = line.indexOf(',', p2 + 1);

  if (p1 < 0 || p2 < 0 || p3 < 0) {
    statusText = "Bad packet";
    updateDisplay();
    return;
  }

  // Split fields
  String letterStr = line.substring(0, p1);
  String xStr      = line.substring(p1 + 1, p2);
  String yStr      = line.substring(p2 + 1, p3);
  String angleStr  = line.substring(p3 + 1);

  if (letterStr.length() == 0) {
    statusText = "Bad letter";
    updateDisplay();
    return;
  }

  char receivedChar = letterStr.charAt(0);
  int receivedX = xStr.toInt();
  int receivedY = yStr.toInt();
  int receivedAngle = angleStr.toInt();

  // Store received image data
  rxLetter = findLetterIndex(receivedChar);
  rxAngle = receivedAngle % 360;

  // Keep received image on right half so both images remain visible
  rxX = constrain(receivedX, 64, 108);
  rxY = constrain(receivedY, 16, 50);

  statusText = "Recv:";
  statusText += receivedChar;

  triggerLED();           // LED ON for 2 seconds on receive
  updateDisplay();
}

void setup() {
  // Start Bluetooth serial
  BT.begin(9600);

  // Configure LED output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Start OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1);
  }

  statusText = "Move=normal";
  updateDisplay();
}

void loop() {
  // ================= JOYSTICK CONTROL =================
  int xVal = analogRead(JOY_X);
  int yVal = analogRead(JOY_Y);

  // Convert raw joystick values into offsets from the center
  int dx = xVal - centerX;
  int dy = yVal - centerY;

  unsigned long now = millis();
  bool moved = false;

  // ---------------- HOLD RIGHT = NEXT LETTER ----------------
  if (dx >= HOLD_OFFSET) {
    statusText = "Hold RIGHT...";

    if (rightHoldStart == 0) {
      rightHoldStart = now;
    } else if ((now - rightHoldStart >= 800) && !rightTriggered) {
      txLetter = (txLetter + 1) % 8;
      rightTriggered = true;
      statusText = "Letter changed";
      updateDisplay();
    }
  } else {
    rightHoldStart = 0;
    rightTriggered = false;
  }

  // ---------------- HOLD LEFT = ROTATE LETTER ----------------
  if (dx <= -HOLD_OFFSET) {
    statusText = "Hold LEFT...";

    if (leftHoldStart == 0) {
      leftHoldStart = now;
    } else if ((now - leftHoldStart >= 800) && !leftTriggered) {
      txAngleIndex = (txAngleIndex + 1) % 4;
      leftTriggered = true;
      statusText = "Angle changed";
      updateDisplay();
    }
  } else {
    leftHoldStart = 0;
    leftTriggered = false;
  }

  // ---------------- HOLD DOWN = SEND DATA ----------------
  if (dy <= -HOLD_OFFSET) {
    statusText = "Hold DOWN...";

    if (downHoldStart == 0) {
      downHoldStart = now;
    } else if ((now - downHoldStart >= 900) && !downTriggered) {
      sendCurrentData();
      downTriggered = true;
    }
  } else {
    downHoldStart = 0;
    downTriggered = false;
  }

  // If any hold gesture is active, block normal movement
  bool holdActive = (dx >= HOLD_OFFSET) || (dx <= -HOLD_OFFSET) || (dy <= -HOLD_OFFSET);

  // ---------------- NORMAL MOVEMENT ----------------
  if (!holdActive) {
    statusText = "Move=normal";

    // Horizontal movement
    if (dx <= -MOVE_OFFSET) {
      txX--;
      moved = true;
    } else if (dx >= MOVE_OFFSET) {
      txX++;
      moved = true;
    }

    // Vertical movement
    if (dy <= -MOVE_OFFSET) {
      txY++;
      moved = true;
    } else if (dy >= MOVE_OFFSET) {
      txY--;
      moved = true;
    }
  }

  // Keep local image on the left half of the OLED
  txX = constrain(txX, 0, 44);
  txY = constrain(txY, 16, 50);

  if (moved || holdActive) {
    updateDisplay();
    delay(45);
  }

  // ================= BLUETOOTH RECEIVE =================
  while (BT.available()) {
    char c = BT.read();

    if (c == '\n') {
      parseIncoming(inputBuffer);
      inputBuffer = "";
    } else if (c != '\r') {
      inputBuffer += c;
    }
  }

  // ================= LED TIMER UPDATE =================
  updateLED();
}