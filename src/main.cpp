#include <Arduino.h>

#include "PetDisplay.h"
#include "PetGame.h"

namespace {
PetGame gPet;
PetDisplay gDisplay;
String gInput;
bool gMenuVisible = true;
uint32_t gMenuDismissAtMs = 0;

constexpr uint8_t kBootButtonPin = 9;
constexpr uint32_t kDebounceMs = 25;
constexpr uint32_t kLongPressMs = 600;
constexpr uint32_t kMenuTimeoutMs = 4500;

enum class ButtonEvent : uint8_t {
  kNone,
  kShortPress,
  kLongPress,
};

struct ButtonTracker {
  bool lastRawPressed = false;
  bool stablePressed = false;
  bool longFired = false;
  uint32_t lastChangeMs = 0;
  uint32_t pressedAtMs = 0;
};

ButtonTracker gBootButton;

void printPrompt() {
  Serial.print("> ");
}

void printBlock(const String& text) {
  Serial.println();
  Serial.println(text);
}

void initControls() {
  pinMode(kBootButtonPin, INPUT_PULLUP);
}

void showMenu() {
  gMenuVisible = true;
  gMenuDismissAtMs = millis() + kMenuTimeoutMs;
}

void hideMenu() {
  gMenuVisible = false;
}

ButtonEvent pollBootButton() {
  const uint32_t now = millis();
  const bool rawPressed = digitalRead(kBootButtonPin) == LOW;

  if (rawPressed != gBootButton.lastRawPressed) {
    gBootButton.lastRawPressed = rawPressed;
    gBootButton.lastChangeMs = now;
  }

  if (now - gBootButton.lastChangeMs < kDebounceMs) {
    return ButtonEvent::kNone;
  }

  if (rawPressed != gBootButton.stablePressed) {
    gBootButton.stablePressed = rawPressed;
    if (rawPressed) {
      gBootButton.pressedAtMs = now;
      gBootButton.longFired = false;
    } else if (!gBootButton.longFired) {
      return ButtonEvent::kShortPress;
    }
  }

  if (gBootButton.stablePressed && !gBootButton.longFired && now - gBootButton.pressedAtMs >= kLongPressMs) {
    gBootButton.longFired = true;
    return ButtonEvent::kLongPress;
  }

  return ButtonEvent::kNone;
}

void handleButtonEvent(ButtonEvent event) {
  const auto snap = gPet.snapshot();
  const bool specialMode = snap.miniGameActive || snap.speciesMenuActive;

  if (event == ButtonEvent::kShortPress) {
    if (specialMode) {
      const String note = gPet.handleButtonTap();
      if (!note.isEmpty()) {
        gDisplay.pushMessage(note);
        Serial.println();
        Serial.print("[boot] ");
        Serial.println(note);
        printPrompt();
      }
      return;
    }

    if (!gMenuVisible) {
      showMenu();
      return;
    }

    gPet.nextAction();
    showMenu();
    const String note = "Selected " + gPet.selectedActionLabel();
    if (!note.isEmpty()) {
      gDisplay.pushMessage(note);
      Serial.println();
      Serial.print("[boot] ");
      Serial.println(note);
      printPrompt();
    }
    return;
  }

  if (event == ButtonEvent::kLongPress) {
    if (!gMenuVisible && !specialMode) {
      showMenu();
      return;
    }

    const String response = gPet.handleButtonHold();
    if (!response.isEmpty()) {
      printBlock(response);
      gDisplay.pushMessage(response);
      const auto after = gPet.snapshot();
      if (!after.miniGameActive && !after.speciesMenuActive) {
        hideMenu();
      }
      gDisplay.update(gPet, gMenuVisible);
      printPrompt();
    }
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);

  const uint32_t start = millis();
  while (!Serial && millis() - start < 4000) {
    delay(10);
  }
  delay(200);

  initControls();
  gPet.begin();
  gDisplay.begin();

  Serial.println();
  Serial.println("Desk Pet for ESP32-C6");
  Serial.println("---------------------");
  Serial.println("Type help to see the commands.");
  Serial.println("BOOT tap: next action");
  Serial.println("BOOT hold: use the action");
  Serial.println("Serial monitor stays available for advanced commands.");
  printBlock(gPet.renderStatus());

  const String note = gPet.consumeNotification();
  if (!note.isEmpty()) {
    Serial.print("[pet] ");
    Serial.println(note);
    gDisplay.pushMessage(note);
  }

  showMenu();
  gDisplay.update(gPet, gMenuVisible);
  printPrompt();
}

void loop() {
  gPet.update();
  const auto snap = gPet.snapshot();
  if (gMenuVisible && !snap.miniGameActive && !snap.speciesMenuActive && millis() > gMenuDismissAtMs) {
    hideMenu();
  }
  handleButtonEvent(pollBootButton());
  gDisplay.update(gPet, gMenuVisible);

  const String note = gPet.consumeNotification();
  if (!note.isEmpty()) {
    Serial.println();
    Serial.print("[pet] ");
    Serial.println(note);
    gDisplay.pushMessage(note);
    if (!gMenuVisible && !gPet.snapshot().miniGameActive && !gPet.snapshot().speciesMenuActive) {
      showMenu();
    }
    printPrompt();
  }

  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      const String response = gPet.handleCommand(gInput);
      gInput = "";
      printBlock(response);
      gDisplay.pushMessage(response);
      showMenu();
      gDisplay.update(gPet, gMenuVisible);
      printPrompt();
      continue;
    }
    gInput += c;
  }
}
