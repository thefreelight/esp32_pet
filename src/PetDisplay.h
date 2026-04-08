#pragma once

#include <Arduino.h>

#include "PetGame.h"

class PetDisplay {
 public:
 void begin();
  void update(const PetGame& pet, bool menuVisible);
  void pushMessage(const String& message);

 private:
  class Arduino_G* panel_ = nullptr;
  class Arduino_GFX* gfx_ = nullptr;
  String transientMessage_;
  uint32_t transientUntilMs_ = 0;
  uint32_t lastDrawMs_ = 0;
  bool ready_ = false;
  bool menuVisible_ = false;

  void drawFrame(const PetGame::Snapshot& snapshot, uint32_t now);
  void drawMiniGame(const PetGame::Snapshot& snapshot, uint16_t accent);
  void drawSpeciesMenu(const PetGame::Snapshot& snapshot, uint16_t accent, uint32_t now);
  void drawBackground(const PetGame::Snapshot& snapshot, uint32_t now);
  void drawPet(const PetGame::Snapshot& snapshot, uint32_t now);
  void drawHeader(const PetGame::Snapshot& snapshot, uint16_t accent);
  void drawStats(const PetGame::Snapshot& snapshot, uint16_t accent);
  void drawActionDock(const PetGame::Snapshot& snapshot, uint16_t accent);
  void drawSpeechBubble(const String& text, uint16_t accent);
  void drawBar(int16_t x, int16_t y, int16_t w, int16_t h, const char* label, uint8_t value, uint16_t color,
               bool compact);
  void drawWrappedText(int16_t x, int16_t y, int16_t maxWidth, const String& text, uint16_t color);
  void drawPixel(int16_t x, int16_t y, int16_t scale, uint16_t color);
  void drawCurrentSpeciesSprite(PetGame::Species species, PetGame::Mood mood, bool asleep, bool animated,
                                uint32_t now, int16_t spriteX, int16_t spriteY, int16_t scale, uint16_t outline,
                                uint16_t accent);
  void drawIdleOverlay(const PetGame::Snapshot& snapshot, uint32_t now, int16_t spriteX, int16_t spriteY,
                       int16_t scale, uint16_t accent);
  void drawEffectOverlay(const PetGame::Snapshot& snapshot, int16_t spriteX, int16_t spriteY, int16_t scale,
                         uint16_t accent);
  void drawPetFace(PetGame::Mood mood, bool blink, int16_t x, int16_t y, int16_t scale, uint16_t color,
                   uint16_t accent);
  void setMoodLed(PetGame::Mood mood);
  String ageLabel(uint32_t ageMinutes) const;
  uint16_t colorForMood(PetGame::Mood mood) const;
  String firstLine(const String& text) const;
  String moodText(PetGame::Mood mood, bool asleep) const;
  String sceneMessage(const PetGame::Snapshot& snapshot) const;
  String clipText(const String& text, size_t maxLen) const;
};
