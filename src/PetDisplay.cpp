#include "PetDisplay.h"

#include <Arduino_GFX_Library.h>
#include <esp32-hal-rgb-led.h>

#include "PetSpriteAssets.h"

namespace {
constexpr int8_t kLcdMosi = 6;
constexpr int8_t kLcdSclk = 7;
constexpr int8_t kLcdCs = 14;
constexpr int8_t kLcdDc = 15;
constexpr int8_t kLcdRst = 21;
constexpr int8_t kLcdBl = 22;
constexpr int8_t kRgbPin = 8;
constexpr int16_t kPanelPhysicalWidth = 172;
constexpr int16_t kPanelPhysicalHeight = 320;
constexpr int16_t kDisplayWidth = 320;
constexpr int16_t kDisplayHeight = 172;
constexpr uint32_t kRedrawIntervalMs = 140;
constexpr uint32_t kMessageDurationMs = 8000;

constexpr int16_t kHeaderX = 8;
constexpr int16_t kHeaderY = 8;
constexpr int16_t kHeaderW = 304;
constexpr int16_t kHeaderH = 24;
constexpr int16_t kSceneX = 8;
constexpr int16_t kSceneY = 38;
constexpr int16_t kSceneW = 304;
constexpr int16_t kSceneH = 126;
constexpr int16_t kStatsX = 192;
constexpr int16_t kStatsY = 38;
constexpr int16_t kStatsW = 120;
constexpr int16_t kStatsH = 72;
constexpr int16_t kDockX = 192;
constexpr int16_t kDockY = 114;
constexpr int16_t kDockW = 120;
constexpr int16_t kDockH = 50;

constexpr const char kCubFrameA[16][17] = {
    "................", "....11....11....", "...1221111221...", "..122222222221..",
    ".12223322332221.", ".12222222222221.", ".12224444442221.", ".12244444444421.",
    ".12244444444421.", ".12224444442221.", ".12222222222221.", "..122222222221..",
    "...1222222221...", "....11111111....", ".....11..11.....", "....11....11....",
};

constexpr const char kCubFrameB[16][17] = {
    "................", "...11......11...", "..122111111221..", ".12222222222221.",
    ".12223322332221.", ".12222222222221.", ".12224444442221.", ".12244444444421.",
    ".12244444444421.", ".12224444442221.", ".12222222222221.", "..122222222221..",
    "...1222112221...", "....11111111....", "....11....11....", "...11......11...",
};

constexpr const char kLionFrameA[16][17] = {
    "....555555......", "..5551111555....", ".551222222155...", ".512222222215...",
    "55122333222155..", "51222222222215..", "51224444442215..", "51244444444415..",
    "51244444444415..", "51224444442215..", "51222222222215..", "55122222222155..",
    ".551222222155...", "..5551111555....", "....55..55......", ".....5..5.......",
};

constexpr const char kLionFrameB[16][17] = {
    "...55555555.....", "..5511111155....", ".551222222215...", "55122222222155..",
    "51222333222215..", "51222222222215..", "51224444442215..", "51244444444415..",
    "51244444444415..", "51224444442215..", "51222222222215..", ".551222222155...",
    "..55122222215...", "...551111155....", "....55....55....", ".....5....5.....",
};

constexpr const char kTigerFrameA[16][17] = {
    "....11....11....", "...1221111221...", "..122222222221..", ".12522555225221.",
    ".12223322332221.", ".12522222225221.", ".12224444442221.", ".12524444442521.",
    ".12244444444421.", ".12524444442521.", ".12222222222221.", "..1222222222215.",
    "...12222222155..", "....11111115....", "......11..55....", ".......1..5.....",
};

constexpr const char kTigerFrameB[16][17] = {
    "....11....11....", "...1221111221...", "..122222222221..", ".12522555225221.",
    ".12223322332221.", ".12522222225221.", ".12224444442221.", ".12524444442521.",
    ".12244444444421.", ".12524444442521.", ".12222222222221.", ".5222222222221..",
    "..55222222221...", "...511111111....", "...55..11.......", "...5...1........",
};

constexpr const char kRoboFrameA[16][17] = {
    "....11111111....", "...1166666611...", "..162222222261..", "..162233332261..",
    "..162344443261..", "..162344443261..", "..162233332261..", "..162222222261..",
    "...1662222661...", "....11266211....", ".....1.66.1.....", "....11.66.11....",
    "....1..66..1....", "......1111......", ".....6....6.....", "................",
};

constexpr const char kRoboFrameB[16][17] = {
    ".....111111.....", "...1166666611...", "..162222222261..", "..162233332261..",
    "..162344443261..", "..162344443261..", "..162233332261..", "..162222222261..",
    "...1662222661...", "....11266211....", ".....1.66.1.....", "....11.66.11....",
    "....1..66..1....", ".....611116.....", "......6..6......", ".......11.......",
};
}  // namespace

void PetDisplay::begin() {
  pinMode(kLcdBl, OUTPUT);
  digitalWrite(kLcdBl, HIGH);

  Arduino_DataBus* bus = new Arduino_ESP32SPI(kLcdDc, kLcdCs, kLcdSclk, kLcdMosi, GFX_NOT_DEFINED);
  panel_ = new Arduino_ST7789(bus, kLcdRst, 1 /* rotation */, true /* IPS */, kPanelPhysicalWidth,
                              kPanelPhysicalHeight,
                              34 /* col offset 1 */, 0 /* row offset 1 */, 34 /* col offset 2 */,
                              0 /* row offset 2 */);
  gfx_ = new Arduino_Canvas(kDisplayWidth, kDisplayHeight, panel_);

  ready_ = gfx_ && gfx_->begin(40000000);
  if (!ready_) {
    return;
  }

  gfx_->fillScreen(RGB565_BLACK);
  gfx_->flush();
  gfx_->setTextWrap(false);
  pushMessage("Tap BOOT to switch. Hold to use.");
}

void PetDisplay::update(const PetGame& pet, bool menuVisible) {
  if (!ready_) {
    return;
  }
  menuVisible_ = menuVisible;

  const uint32_t now = millis();
  if (now - lastDrawMs_ < kRedrawIntervalMs) {
    return;
  }

  transientUntilMs_ = (transientUntilMs_ > now) ? transientUntilMs_ : 0;
  if (transientUntilMs_ == 0) {
    transientMessage_ = "";
  }

  const PetGame::Snapshot snapshot = pet.snapshot();
  setMoodLed(snapshot.mood);
  drawFrame(snapshot, now);
  lastDrawMs_ = now;
}

void PetDisplay::pushMessage(const String& message) {
  transientMessage_ = clipText(firstLine(message), 42);
  transientUntilMs_ = millis() + kMessageDurationMs;
}

void PetDisplay::drawFrame(const PetGame::Snapshot& snapshot, uint32_t now) {
  const uint16_t accent = colorForMood(snapshot.mood);
  gfx_->fillScreen(RGB565(10, 14, 28));
  if (snapshot.miniGameActive) {
    drawMiniGame(snapshot, accent);
    gfx_->flush();
    return;
  }
  if (snapshot.speciesMenuActive) {
    drawSpeciesMenu(snapshot, accent, now);
    gfx_->flush();
    return;
  }
  drawBackground(snapshot, now);
  drawHeader(snapshot, accent);
  drawSpeechBubble(sceneMessage(snapshot), accent);
  drawPet(snapshot, now);
  if (menuVisible_) {
    drawStats(snapshot, accent);
    drawActionDock(snapshot, accent);
  }
  gfx_->flush();
}

void PetDisplay::drawSpeciesMenu(const PetGame::Snapshot& snapshot, uint16_t accent, uint32_t now) {
  gfx_->fillRoundRect(8, 8, 304, 24, 10, RGB565(28, 22, 52));
  gfx_->drawRoundRect(8, 8, 304, 24, 10, accent);
  gfx_->setTextColor(RGB565_WHITE);
  gfx_->setTextSize(1);
  gfx_->setCursor(18, 17);
  gfx_->print("PETS");
  gfx_->setCursor(78, 17);
  gfx_->print("Tap browse, hold adopt");

  const PetGame::Species roster[4] = {
      PetGame::Species::kCubCat,
      PetGame::Species::kLion,
      PetGame::Species::kTiger,
      PetGame::Species::kRoboCat,
  };
  const char* labels[4] = {"CUB", "LION", "TIGER", "ROBO"};

  for (uint8_t i = 0; i < 4; ++i) {
    const int16_t cardX = 8 + i * 78;
    const int16_t cardY = 42;
    const bool chosen = roster[i] == snapshot.speciesChoice;
    const bool current = roster[i] == snapshot.species;
    const uint16_t frame = chosen ? accent : RGB565(78, 98, 138);
    gfx_->fillRoundRect(cardX, cardY, 70, 84, 10, RGB565(18, 24, 44));
    gfx_->drawRoundRect(cardX, cardY, 70, 84, 10, frame);
    if (current) {
      gfx_->fillRoundRect(cardX + 40, cardY + 6, 22, 10, 5, RGB565_HOTPINK);
      gfx_->setTextColor(RGB565_BLACK);
      gfx_->setCursor(cardX + 44, cardY + 13);
      gfx_->print("NOW");
    }

    gfx_->fillRoundRect(cardX + 9, cardY + 18, 52, 52, 10, RGB565(195, 248, 237));
    gfx_->drawRoundRect(cardX + 9, cardY + 18, 52, 52, 10, frame);
    drawCurrentSpeciesSprite(roster[i], snapshot.mood, false, false, now, cardX + 11, cardY + 20, 2, RGB565_WHITE,
                             frame);
    gfx_->setTextColor(chosen ? frame : RGB565_WHITE);
    gfx_->setCursor(cardX + 20, cardY + 78);
    gfx_->print(labels[i]);
  }

  gfx_->fillRoundRect(8, 132, 304, 32, 10, RGB565(18, 24, 44));
  gfx_->drawRoundRect(8, 132, 304, 32, 10, accent);
  gfx_->setTextColor(RGB565_WHITE);
  gfx_->setCursor(18, 146);
  gfx_->print("Choose: ");
  gfx_->print(snapshot.speciesChoiceLabel);
  gfx_->setCursor(146, 146);
  gfx_->print("Hold BOOT to adopt");
}

void PetDisplay::drawMiniGame(const PetGame::Snapshot& snapshot, uint16_t accent) {
  gfx_->fillRoundRect(8, 8, 304, 24, 10, RGB565(28, 20, 50));
  gfx_->drawRoundRect(8, 8, 304, 24, 10, accent);

  gfx_->setTextColor(RGB565_WHITE);
  gfx_->setTextSize(1);
  gfx_->setCursor(18, 17);
  gfx_->print("STAR");
  gfx_->setCursor(52, 17);
  gfx_->print("CHASE");

  gfx_->setCursor(120, 17);
  gfx_->print("Round ");
  gfx_->print(snapshot.miniRound);
  gfx_->print("/");
  gfx_->print(snapshot.miniTotalRounds);
  gfx_->setCursor(204, 17);
  gfx_->print("Score ");
  gfx_->print(snapshot.miniScore);

  gfx_->fillRoundRect(8, 40, 178, 124, 12, RGB565(18, 28, 56));
  gfx_->drawRoundRect(8, 40, 178, 124, 12, RGB565(92, 132, 198));
  gfx_->fillCircle(34, 66, 10, RGB565(255, 208, 120));
  gfx_->fillCircle(34, 66, 4, RGB565(255, 232, 180));
  gfx_->fillCircle(160, 62, 16, RGB565(40, 56, 92));
  gfx_->fillCircle(150, 70, 10, RGB565(54, 72, 110));

  gfx_->setTextColor(RGB565_WHITE);
  gfx_->setCursor(18, 54);
  gfx_->print("Hit when the spark");
  gfx_->setCursor(18, 66);
  gfx_->print("lands inside");
  gfx_->fillRoundRect(26, 78, 44, 16, 8, RGB565_HOTPINK);
  gfx_->setTextColor(RGB565_BLACK);
  gfx_->setCursor(34, 89);
  gfx_->print("HEART");
  gfx_->setTextColor(RGB565_WHITE);
  gfx_->setCursor(76, 89);
  gfx_->print("zone.");

  const int16_t laneX = 18;
  const int16_t laneY = 116;
  const int16_t laneW = 156;
  gfx_->drawRoundRect(laneX, laneY, laneW, 14, 7, RGB565_WHITE);
  const int16_t zoneX = laneX + 4 + (snapshot.miniTargetStart * (laneW - 12)) / 100;
  const int16_t zoneW = max<int16_t>(12, (snapshot.miniTargetWidth * (laneW - 12)) / 100);
  gfx_->fillRoundRect(zoneX, laneY + 2, zoneW, 10, 5, RGB565_HOTPINK);
  const int16_t cursorX = laneX + 6 + (snapshot.miniCursor * (laneW - 16)) / 100;
  gfx_->fillCircle(cursorX, laneY + 7, 5, RGB565_GOLD);
  gfx_->drawCircle(cursorX, laneY + 7, 5, RGB565_BLACK);

  gfx_->fillRoundRect(194, 40, 118, 68, 10, RGB565(20, 18, 40));
  gfx_->drawRoundRect(194, 40, 118, 68, 10, accent);
  gfx_->setTextColor(RGB565_WHITE);
  gfx_->setCursor(202, 58);
  gfx_->print(clipText(snapshot.miniPrompt, 16));
  gfx_->setCursor(202, 76);
  if (snapshot.miniResult.isEmpty()) {
    gfx_->print("Tap/hold BOOT");
  } else {
    gfx_->print("Last hit: ");
    gfx_->print(snapshot.miniResult);
  }

  gfx_->fillRoundRect(194, 114, 118, 50, 10, RGB565(24, 18, 38));
  gfx_->drawRoundRect(194, 114, 118, 50, 10, accent);
  gfx_->fillRoundRect(202, 121, 42, 20, 10, accent);
  gfx_->setTextColor(RGB565_BLACK);
  gfx_->setCursor(214, 128);
  gfx_->print("HIT");
  gfx_->setTextColor(RGB565_WHITE);
  gfx_->setCursor(252, 126);
  gfx_->print("Tap/hold");
  gfx_->setCursor(252, 136);
  gfx_->print("BOOT");
  gfx_->setCursor(202, 149);
  gfx_->print("Perfect = 2 pts");
}

void PetDisplay::drawBackground(const PetGame::Snapshot& snapshot, uint32_t now) {
  const uint16_t top = snapshot.asleep ? RGB565(14, 18, 44) : RGB565(18, 40, 96);
  const uint16_t mid = snapshot.asleep ? RGB565(24, 26, 62) : RGB565(42, 86, 156);
  const uint16_t low = snapshot.asleep ? RGB565(58, 44, 78) : RGB565(56, 130, 172);

  gfx_->fillRect(kSceneX, kSceneY, kSceneW, 36, top);
  gfx_->fillRect(kSceneX, kSceneY + 36, kSceneW, 36, mid);
  gfx_->fillRect(kSceneX, kSceneY + 72, kSceneW, kSceneH - 72, low);
  gfx_->drawRoundRect(kSceneX, kSceneY, kSceneW, kSceneH, 12, RGB565(104, 146, 188));

  const int16_t orbX = kSceneX + kSceneW - 26;
  const int16_t orbY = kSceneY + 14;
  gfx_->fillCircle(orbX, orbY, 11, snapshot.asleep ? RGB565(240, 240, 210) : RGB565(255, 208, 120));
  if (snapshot.asleep) {
    gfx_->fillCircle(orbX + 4, orbY - 3, 10, top);
    for (int i = 0; i < 4; ++i) {
      const int16_t sx = kSceneX + 18 + ((now / 17) + i * 23) % 80;
      const int16_t sy = kSceneY + 18 + ((i % 2) * 12);
      gfx_->fillRect(sx, sy, 2, 2, RGB565_WHITE);
    }
  } else {
    const int16_t cloudDrift = static_cast<int16_t>((now / 80) % 26);
    gfx_->fillRoundRect(kSceneX + 12 + cloudDrift / 3, kSceneY + 16, 24, 10, 5, RGB565(238, 244, 248));
    gfx_->fillRoundRect(kSceneX + 16 + cloudDrift / 3, kSceneY + 12, 14, 12, 6, RGB565(238, 244, 248));
  }

  gfx_->fillRoundRect(kSceneX + 8, kSceneY + kSceneH - 22, kSceneW - 16, 14, 8, RGB565(42, 116, 74));
  for (int i = 0; i < 10; ++i) {
    const int16_t gx = kSceneX + 14 + i * 13;
    gfx_->drawFastVLine(gx, kSceneY + kSceneH - 22, 5 + (i % 3), RGB565(86, 176, 104));
  }
}

void PetDisplay::drawHeader(const PetGame::Snapshot& snapshot, uint16_t accent) {
  gfx_->fillRoundRect(kHeaderX, kHeaderY, kHeaderW, kHeaderH, 10, RGB565(18, 22, 40));
  gfx_->drawRoundRect(kHeaderX, kHeaderY, kHeaderW, kHeaderH, 10, RGB565(86, 132, 186));

  gfx_->setTextColor(RGB565_WHITE);
  gfx_->setTextSize(1);
  gfx_->setCursor(kHeaderX + 10, kHeaderY + 16);
  gfx_->print(clipText(snapshot.name, 9));

  gfx_->setCursor(kHeaderX + 102, kHeaderY + 16);
  gfx_->print(snapshot.title);
  gfx_->setCursor(kHeaderX + 210, kHeaderY + 16);
  gfx_->print(clipText(snapshot.speciesLabel, 8));
  gfx_->fillCircle(kHeaderX + kHeaderW - 16, kHeaderY + 12, 4, accent);
}

void PetDisplay::drawStats(const PetGame::Snapshot& snapshot, uint16_t accent) {
  if (!menuVisible_) {
    return;
  }
  gfx_->fillRoundRect(kStatsX, kStatsY, kStatsW, kStatsH, 10, RGB565(18, 22, 40));
  gfx_->drawRoundRect(kStatsX, kStatsY, kStatsW, kStatsH, 10, RGB565(86, 132, 186));

  gfx_->setTextColor(RGB565_WHITE);
  gfx_->setTextSize(1);
  gfx_->setCursor(kStatsX + 8, kStatsY + 10);
  gfx_->print("Age ");
  gfx_->print(ageLabel(snapshot.ageMinutes));
  gfx_->setCursor(kStatsX + 68, kStatsY + 10);
  gfx_->print("Find ");
  gfx_->print(snapshot.discoveries);

  drawBar(kStatsX + 8, kStatsY + 18, kStatsW - 16, 7, "FULL", snapshot.fullness, RGB565_GOLD, true);
  drawBar(kStatsX + 8, kStatsY + 29, kStatsW - 16, 7, "ENER", snapshot.energy, RGB565_TURQUOISE, true);
  drawBar(kStatsX + 8, kStatsY + 40, kStatsW - 16, 7, "JOY", snapshot.joy, RGB565_LIMEGREEN, true);
  drawBar(kStatsX + 8, kStatsY + 51, kStatsW - 16, 7, "CLEAN", snapshot.hygiene, RGB565_SKYBLUE, true);
  drawBar(kStatsX + 8, kStatsY + 62, kStatsW - 16, 7, "BOND", snapshot.bond, accent, true);
}

void PetDisplay::drawActionDock(const PetGame::Snapshot& snapshot, uint16_t accent) {
  if (!menuVisible_) {
    return;
  }
  gfx_->fillRoundRect(kDockX, kDockY, kDockW, kDockH, 10, RGB565(24, 18, 38));
  gfx_->drawRoundRect(kDockX, kDockY, kDockW, kDockH, 10, RGB565(86, 132, 186));

  gfx_->fillRoundRect(kDockX + 8, kDockY + 7, 48, 18, 9, accent);
  gfx_->setTextColor(RGB565_BLACK);
  gfx_->setTextSize(1);
  gfx_->setCursor(kDockX + 14, kDockY + 13);
  gfx_->print(snapshot.selectedActionLabel);

  gfx_->setTextColor(RGB565_WHITE);
  gfx_->setCursor(kDockX + 64, kDockY + 11);
  gfx_->print("Tap next");
  gfx_->setCursor(kDockX + 64, kDockY + 21);
  gfx_->print("Hold use");
  gfx_->setCursor(kDockX + 8, kDockY + 38);
  gfx_->print(clipText(snapshot.selectedActionHint, 18));
}

void PetDisplay::drawSpeechBubble(const String& text, uint16_t accent) {
  gfx_->fillRoundRect(kSceneX + 10, kSceneY + 8, 124, 24, 10, RGB565(244, 248, 250));
  gfx_->fillTriangle(kSceneX + 28, kSceneY + 32, kSceneX + 38, kSceneY + 32, kSceneX + 33, kSceneY + 40,
                     RGB565(244, 248, 250));
  gfx_->drawRoundRect(kSceneX + 10, kSceneY + 8, 124, 24, 10, accent);
  gfx_->setTextColor(RGB565(34, 28, 48));
  gfx_->setTextSize(1);
  drawWrappedText(kSceneX + 14, kSceneY + 16, 110, clipText(text, 18), RGB565(34, 28, 48));
}

void PetDisplay::drawPet(const PetGame::Snapshot& snapshot, uint32_t now) {
  const bool asleep = snapshot.asleep;
  const int16_t bob = asleep ? static_cast<int16_t>((now / 420) % 2) : static_cast<int16_t>((now / 180) % 3);
  const int16_t scale = menuVisible_ ? 4 : 5;
  const int16_t spriteX = menuVisible_ ? (kSceneX + 34) : (kSceneX + 84);
  const int16_t spriteY = menuVisible_ ? (kSceneY + 42 + bob) : (kSceneY + 22 + bob);
  const uint16_t outline = RGB565(26, 22, 42);
  const uint16_t accent = colorForMood(snapshot.mood);
  gfx_->fillEllipse(spriteX + (menuVisible_ ? 44 : 56), spriteY + (menuVisible_ ? 95 : 118),
                    menuVisible_ ? 34 : 42, menuVisible_ ? 8 : 10, RGB565(66, 120, 92));
  drawIdleOverlay(snapshot, now, spriteX, spriteY, scale, accent);
  drawCurrentSpeciesSprite(snapshot.species, snapshot.mood, snapshot.asleep, true, now, spriteX, spriteY, scale,
                           outline, accent);
  drawEffectOverlay(snapshot, spriteX, spriteY, scale, accent);
}

void PetDisplay::drawBar(int16_t x, int16_t y, int16_t w, int16_t h, const char* label, uint8_t value,
                         uint16_t color, bool compact) {
  gfx_->setTextColor(RGB565_WHITE);
  gfx_->setTextSize(1);
  gfx_->setCursor(x, y - (compact ? 0 : 2));
  gfx_->print(label);

  const int16_t labelWidth = compact ? 30 : 42;
  gfx_->drawRoundRect(x + labelWidth, y, w - labelWidth, h, 4, RGB565_DARKSLATEGRAY);
  const int16_t fillWidth = ((w - labelWidth - 4) * value) / 100;
  gfx_->fillRoundRect(x + labelWidth + 2, y + 2, max<int16_t>(0, fillWidth), h - 4, 3, color);

  gfx_->setCursor(x + w - 18, y - (compact ? 0 : 2));
  if (value < 100) {
    gfx_->print(' ');
  }
  if (value < 10) {
    gfx_->print(' ');
  }
  gfx_->print(value);
}

void PetDisplay::drawWrappedText(int16_t x, int16_t y, int16_t maxWidth, const String& text, uint16_t color) {
  gfx_->setTextColor(color);
  gfx_->setTextSize(1);

  const int charsPerLine = maxWidth / 6;
  String line;
  int16_t cursorY = y;

  for (uint16_t i = 0; i < text.length();) {
    while (i < text.length() && text[i] == ' ') {
      ++i;
    }

    String word;
    while (i < text.length() && text[i] != ' ') {
      word += text[i++];
    }

    if (word.isEmpty()) {
      break;
    }

    const String candidate = line.isEmpty() ? word : line + " " + word;
    if (candidate.length() > static_cast<size_t>(charsPerLine) && !line.isEmpty()) {
      gfx_->setCursor(x, cursorY);
      gfx_->print(line);
      cursorY += 10;
      line = word;
      if (cursorY > y + 18) {
        break;
      }
    } else {
      line = candidate;
    }
  }

  if (!line.isEmpty() && cursorY <= y + 18) {
    gfx_->setCursor(x, cursorY);
    gfx_->print(line);
  }
}

void PetDisplay::drawPixel(int16_t x, int16_t y, int16_t scale, uint16_t color) {
  gfx_->fillRect(x, y, scale, scale, color);
}

void PetDisplay::drawCurrentSpeciesSprite(PetGame::Species species, PetGame::Mood mood, bool asleep, bool animated,
                                          uint32_t now, int16_t spriteX, int16_t spriteY, int16_t scale,
                                          uint16_t outline, uint16_t accent) {
  const uint16_t (*colors)[PetSpriteAssets::kSpriteWidth * PetSpriteAssets::kSpriteHeight] = nullptr;
  const uint8_t (*mask)[PetSpriteAssets::kSpriteWidth * PetSpriteAssets::kSpriteHeight] = nullptr;
  uint8_t frameIndex = 0;
  if (animated) {
    uint32_t period = 220;
    if (species == PetGame::Species::kTiger) {
      period = 160;
    } else if (species == PetGame::Species::kRoboCat) {
      period = 260;
    } else if (asleep) {
      period = 420;
    }
    frameIndex = static_cast<uint8_t>((now / period) % PetSpriteAssets::kFrameCount);
  }

  switch (species) {
    case PetGame::Species::kCubCat:
      colors = PetSpriteAssets::CubCatColors;
      mask = PetSpriteAssets::CubCatMask;
      break;
    case PetGame::Species::kLion:
      colors = PetSpriteAssets::LionColors;
      mask = PetSpriteAssets::LionMask;
      break;
    case PetGame::Species::kTiger:
      colors = PetSpriteAssets::TigerColors;
      mask = PetSpriteAssets::TigerMask;
      break;
    case PetGame::Species::kRoboCat:
      colors = PetSpriteAssets::RoboCatColors;
      mask = PetSpriteAssets::RoboCatMask;
      break;
  }

  if (colors && mask) {
    for (int16_t py = 0; py < PetSpriteAssets::kSpriteHeight; ++py) {
      for (int16_t px = 0; px < PetSpriteAssets::kSpriteWidth; ++px) {
        const int idx = py * PetSpriteAssets::kSpriteWidth + px;
        if (!mask[frameIndex][idx]) {
          continue;
        }
        drawPixel(spriteX + px * scale, spriteY + py * scale, scale, colors[frameIndex][idx]);
      }
    }
  }

  if (mood == PetGame::Mood::kHappy) {
    gfx_->fillCircle(spriteX + 94, spriteY + 8, 4, RGB565_HOTPINK);
  } else if (mood == PetGame::Mood::kHungry) {
    gfx_->fillRect(spriteX + 92, spriteY + 12, 10, 10, RGB565_GOLD);
    gfx_->drawRect(spriteX + 92, spriteY + 12, 10, 10, outline);
  } else if (mood == PetGame::Mood::kSleepy) {
    gfx_->setTextColor(RGB565_WHITE);
    gfx_->setTextSize(1);
    gfx_->setCursor(spriteX + 90, spriteY + 6);
    gfx_->print("Z");
    gfx_->setCursor(spriteX + 98, spriteY + 0);
    gfx_->print("z");
  } else if (mood == PetGame::Mood::kDirty) {
    gfx_->fillCircle(spriteX + 94, spriteY + 10, 3, RGB565_BROWN);
  } else if (mood == PetGame::Mood::kLonely) {
    gfx_->fillCircle(spriteX + 94, spriteY + 12, 3, RGB565_SKYBLUE);
  } else if (mood == PetGame::Mood::kUpset) {
    gfx_->fillTriangle(spriteX + 92, spriteY + 12, spriteX + 102, spriteY + 12, spriteX + 97, spriteY + 4,
                       RGB565_RED);
  }
}

void PetDisplay::drawIdleOverlay(const PetGame::Snapshot& snapshot, uint32_t now, int16_t spriteX, int16_t spriteY,
                                 int16_t scale, uint16_t accent) {
  if (snapshot.effectActive || snapshot.miniGameActive || snapshot.speciesMenuActive) {
    return;
  }

  if (snapshot.asleep) {
    gfx_->setTextColor(RGB565_WHITE);
    gfx_->setTextSize(1);
    gfx_->setCursor(spriteX + 78, spriteY - 10 - ((now / 400) % 3));
    gfx_->print("z");
    return;
  }

  switch (snapshot.species) {
    case PetGame::Species::kCubCat: {
      const int16_t heartY = spriteY + 6 + static_cast<int16_t>((now / 220) % 4);
      gfx_->fillCircle(spriteX + 92, heartY, 2, RGB565_HOTPINK);
      gfx_->fillCircle(spriteX + 98, heartY + 3, 2, RGB565_HOTPINK);
      break;
    }
    case PetGame::Species::kLion: {
      const int16_t tuft = static_cast<int16_t>((now / 160) % 8);
      gfx_->drawLine(spriteX + 88, spriteY + 46, spriteX + 96, spriteY + 38 + (tuft > 4 ? 8 - tuft : tuft), accent);
      gfx_->fillCircle(spriteX + 98, spriteY + 40 + (tuft > 4 ? 8 - tuft : tuft), 2, accent);
      break;
    }
    case PetGame::Species::kTiger: {
      const int16_t paw = static_cast<int16_t>((now / 160) % 6);
      gfx_->fillRect(spriteX + 4, spriteY + 82 - paw, 6, 3, accent);
      break;
    }
    case PetGame::Species::kRoboCat: {
      const uint16_t pulse = ((now / 220) % 2) == 0 ? RGB565_CYAN : RGB565_WHITE;
      gfx_->fillCircle(spriteX + 46, spriteY - 2, 3, pulse);
      gfx_->drawCircle(spriteX + 46, spriteY - 2, 5, accent);
      break;
    }
  }
}

void PetDisplay::drawEffectOverlay(const PetGame::Snapshot& snapshot, int16_t spriteX, int16_t spriteY, int16_t scale,
                                   uint16_t accent) {
  if (!snapshot.effectActive) {
    return;
  }

  switch (snapshot.effect) {
    case PetGame::Effect::kFeed: {
      const int16_t foodX = spriteX + 68 - snapshot.effectProgress / 4;
      const int16_t foodY = spriteY + 28 + (snapshot.effectProgress > 50 ? 1 : 0);
      gfx_->fillRoundRect(foodX, foodY, 12, 8, 3, RGB565_GOLD);
      gfx_->drawRoundRect(foodX, foodY, 12, 8, 3, RGB565_BLACK);
      for (int i = 0; i < 3; ++i) {
        gfx_->fillCircle(spriteX + 40 + i * 6, spriteY + 44 + i, 2, RGB565(255, 222, 150));
      }
      break;
    }
    case PetGame::Effect::kPet: {
      gfx_->fillCircle(spriteX + 62, spriteY + 10, 4, RGB565_HOTPINK);
      gfx_->fillCircle(spriteX + 74, spriteY + 20, 3, RGB565_HOTPINK);
      gfx_->fillCircle(spriteX + 24, spriteY + 16, 3, RGB565_HOTPINK);
      break;
    }
    case PetGame::Effect::kClean: {
      for (int i = 0; i < 4; ++i) {
        gfx_->drawCircle(spriteX + 12 + i * 14, spriteY + 12 + (i % 2) * 10, 5, RGB565_WHITE);
      }
      break;
    }
    case PetGame::Effect::kExplore: {
      for (int i = 0; i < 4; ++i) {
        gfx_->fillCircle(spriteX + 10 + i * 14, spriteY + 50 - (i % 2) * 8, 2, accent);
        gfx_->drawLine(spriteX + 10 + i * 14, spriteY + 50 - (i % 2) * 8, spriteX + 14 + i * 14,
                       spriteY + 44 - (i % 2) * 8, accent);
      }
      break;
    }
    case PetGame::Effect::kSleep: {
      gfx_->setTextColor(RGB565_WHITE);
      gfx_->setCursor(spriteX + 64, spriteY - 4);
      gfx_->print("z");
      gfx_->setCursor(spriteX + 72, spriteY - 12);
      gfx_->print("Z");
      break;
    }
    case PetGame::Effect::kAdopt: {
      gfx_->fillRoundRect(spriteX - 8, spriteY + 62, 80, 12, 6, accent);
      gfx_->setTextColor(RGB565_BLACK);
      gfx_->setCursor(spriteX + 2, spriteY + 71);
      gfx_->print("NEW PAL");
      break;
    }
    case PetGame::Effect::kNone:
    default:
      break;
  }
}

void PetDisplay::drawPetFace(PetGame::Mood mood, bool blink, int16_t x, int16_t y, int16_t scale, uint16_t color,
                             uint16_t accent) {
  const auto dot = [&](int16_t gx, int16_t gy, int16_t w = 1, int16_t h = 1, uint16_t c = 0) {
    drawPixel(x + gx * scale, y + gy * scale, scale * w, c == 0 ? color : c);
    if (w > 1 || h > 1) {
      gfx_->fillRect(x + gx * scale, y + gy * scale, scale * w, scale * h, c == 0 ? color : c);
    }
  };

  if (blink && mood != PetGame::Mood::kSleepy) {
    gfx_->fillRect(x + 4 * scale, y + 6 * scale, 2 * scale, scale, color);
    gfx_->fillRect(x + 10 * scale, y + 6 * scale, 2 * scale, scale, color);
    return;
  }

  switch (mood) {
    case PetGame::Mood::kHappy:
      gfx_->fillRect(x + 4 * scale, y + 6 * scale, scale, 2 * scale, color);
      gfx_->fillRect(x + 11 * scale, y + 6 * scale, scale, 2 * scale, color);
      gfx_->fillRect(x + 6 * scale, y + 9 * scale, 4 * scale, scale, color);
      gfx_->fillRect(x + 7 * scale, y + 10 * scale, 2 * scale, scale, color);
      gfx_->fillRect(x + 3 * scale, y + 9 * scale, scale, scale, accent);
      gfx_->fillRect(x + 12 * scale, y + 9 * scale, scale, scale, accent);
      break;
    case PetGame::Mood::kHungry:
      gfx_->fillRect(x + 4 * scale, y + 6 * scale, scale, 2 * scale, color);
      gfx_->fillRect(x + 11 * scale, y + 6 * scale, scale, 2 * scale, color);
      gfx_->drawRect(x + 7 * scale, y + 9 * scale, 2 * scale, 2 * scale, color);
      break;
    case PetGame::Mood::kSleepy:
      gfx_->fillRect(x + 4 * scale, y + 7 * scale, 2 * scale, scale, color);
      gfx_->fillRect(x + 10 * scale, y + 7 * scale, 2 * scale, scale, color);
      gfx_->fillRect(x + 7 * scale, y + 10 * scale, 2 * scale, scale, color);
      break;
    case PetGame::Mood::kDirty:
      gfx_->drawLine(x + 4 * scale, y + 6 * scale, x + 6 * scale, y + 8 * scale, color);
      gfx_->drawLine(x + 6 * scale, y + 6 * scale, x + 4 * scale, y + 8 * scale, color);
      gfx_->drawLine(x + 10 * scale, y + 6 * scale, x + 12 * scale, y + 8 * scale, color);
      gfx_->drawLine(x + 12 * scale, y + 6 * scale, x + 10 * scale, y + 8 * scale, color);
      gfx_->fillRect(x + 7 * scale, y + 10 * scale, 2 * scale, scale, color);
      break;
    case PetGame::Mood::kLonely:
      gfx_->fillRect(x + 4 * scale, y + 6 * scale, scale, 2 * scale, color);
      gfx_->fillRect(x + 11 * scale, y + 6 * scale, scale, 2 * scale, color);
      gfx_->fillRect(x + 4 * scale, y + 8 * scale, scale, scale, RGB565_SKYBLUE);
      gfx_->fillRect(x + 11 * scale, y + 8 * scale, scale, scale, RGB565_SKYBLUE);
      gfx_->fillRect(x + 6 * scale, y + 10 * scale, 4 * scale, scale, color);
      break;
    case PetGame::Mood::kUpset:
      gfx_->drawLine(x + 3 * scale, y + 6 * scale, x + 5 * scale, y + 5 * scale, color);
      gfx_->drawLine(x + 10 * scale, y + 5 * scale, x + 12 * scale, y + 6 * scale, color);
      gfx_->fillRect(x + 4 * scale, y + 7 * scale, scale, scale, color);
      gfx_->fillRect(x + 11 * scale, y + 7 * scale, scale, scale, color);
      gfx_->fillRect(x + 6 * scale, y + 10 * scale, 4 * scale, scale, color);
      break;
    case PetGame::Mood::kContent:
    default:
      gfx_->fillRect(x + 4 * scale, y + 6 * scale, scale, 2 * scale, color);
      gfx_->fillRect(x + 11 * scale, y + 6 * scale, scale, 2 * scale, color);
      gfx_->fillRect(x + 7 * scale, y + 9 * scale, 2 * scale, scale, color);
      break;
  }
}

void PetDisplay::setMoodLed(PetGame::Mood mood) {
  switch (mood) {
    case PetGame::Mood::kHappy:
      rgbLedWrite(kRgbPin, 0, 48, 8);
      break;
    case PetGame::Mood::kContent:
      rgbLedWrite(kRgbPin, 0, 24, 28);
      break;
    case PetGame::Mood::kHungry:
      rgbLedWrite(kRgbPin, 48, 18, 0);
      break;
    case PetGame::Mood::kSleepy:
      rgbLedWrite(kRgbPin, 0, 0, 40);
      break;
    case PetGame::Mood::kDirty:
      rgbLedWrite(kRgbPin, 40, 12, 8);
      break;
    case PetGame::Mood::kLonely:
      rgbLedWrite(kRgbPin, 18, 0, 30);
      break;
    case PetGame::Mood::kUpset:
      rgbLedWrite(kRgbPin, 48, 0, 0);
      break;
  }
}

String PetDisplay::ageLabel(uint32_t ageMinutes) const {
  return String(ageMinutes / 60) + "h " + String(ageMinutes % 60) + "m";
}

uint16_t PetDisplay::colorForMood(PetGame::Mood mood) const {
  switch (mood) {
    case PetGame::Mood::kHappy:
      return RGB565_GREEN;
    case PetGame::Mood::kContent:
      return RGB565_CYAN;
    case PetGame::Mood::kHungry:
      return RGB565_ORANGE;
    case PetGame::Mood::kSleepy:
      return RGB565_BLUE;
    case PetGame::Mood::kDirty:
      return RGB565_BROWN;
    case PetGame::Mood::kLonely:
      return RGB565_MAGENTA;
    case PetGame::Mood::kUpset:
      return RGB565_RED;
  }

  return RGB565_WHITE;
}

String PetDisplay::firstLine(const String& text) const {
  const int newline = text.indexOf('\n');
  if (newline < 0) {
    return text;
  }
  return text.substring(0, newline);
}

String PetDisplay::sceneMessage(const PetGame::Snapshot& snapshot) const {
  if (!transientMessage_.isEmpty()) {
    return transientMessage_;
  }
  if (snapshot.selectedAction == PetGame::Action::kPlay) {
    return "Select GAME, then hold BOOT to start.";
  }
  if (snapshot.selectedAction == PetGame::Action::kRoster) {
    return "Open PETS to hatch lion, tiger, cub, or robo cat.";
  }
  if (snapshot.ageMinutes < 8 && snapshot.discoveries == 0) {
    return "Tap to pick an action";
  }
  return "Now: " + snapshot.selectedActionLabel + " | Hold to use";
}

String PetDisplay::moodText(PetGame::Mood mood, bool asleep) const {
  switch (mood) {
    case PetGame::Mood::kHappy:
      return "HAPPY";
    case PetGame::Mood::kContent:
      return "CONTENT";
    case PetGame::Mood::kHungry:
      return "HUNGRY";
    case PetGame::Mood::kSleepy:
      return asleep ? "SLEEP" : "SLEEPY";
    case PetGame::Mood::kDirty:
      return "DIRTY";
    case PetGame::Mood::kLonely:
      return "LONELY";
    case PetGame::Mood::kUpset:
      return "UPSET";
  }

  return "";
}

String PetDisplay::clipText(const String& text, size_t maxLen) const {
  if (text.length() <= maxLen) {
    return text;
  }
  return text.substring(0, maxLen - 1) + ".";
}
