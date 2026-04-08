#pragma once

#include <Arduino.h>

class PetGame {
 public:
  enum class Species : uint8_t {
    kCubCat,
    kLion,
    kTiger,
    kRoboCat,
  };

  enum class Mood : uint8_t {
    kHappy,
    kContent,
    kHungry,
    kSleepy,
    kDirty,
    kLonely,
    kUpset,
  };

  enum class Action : uint8_t {
    kFeed,
    kPlay,
    kPet,
    kClean,
    kExplore,
    kSleep,
    kRoster,
  };

  enum class Effect : uint8_t {
    kNone,
    kFeed,
    kPet,
    kClean,
    kExplore,
    kSleep,
    kAdopt,
  };

  struct Snapshot {
    String name;
    Species species;
    String speciesLabel;
    Mood mood;
    bool asleep;
    uint8_t fullness;
    uint8_t energy;
    uint8_t joy;
    uint8_t hygiene;
    uint8_t bond;
    uint8_t messes;
    uint32_t ageMinutes;
    String face;
    String hint;
    String title;
    Action selectedAction;
    String selectedActionLabel;
    String selectedActionHint;
    uint16_t discoveries;
    bool miniGameActive;
    uint8_t miniRound;
    uint8_t miniTotalRounds;
    uint8_t miniScore;
    uint8_t miniCursor;
    uint8_t miniTargetStart;
    uint8_t miniTargetWidth;
    String miniPrompt;
    String miniResult;
    bool speciesMenuActive;
    Species speciesChoice;
    String speciesChoiceLabel;
    bool effectActive;
    Effect effect;
    uint8_t effectProgress;
    String effectLabel;
  };

  void begin();
  void update();

  String handleCommand(const String& rawInput);
  String commandHelp() const;
  String renderStatus() const;
  String consumeNotification();
  Snapshot snapshot() const;
  Mood mood() const;
  void nextAction();
  String activateSelectedAction();
  String selectedActionLabel() const;
  String handleButtonTap();
  String handleButtonHold();

 private:
  struct EffectState {
    bool active = false;
    Effect type = Effect::kNone;
    uint32_t startedAtMs = 0;
    uint32_t endsAtMs = 0;
    String label;
  };

  struct SpeciesMenuState {
    bool active = false;
    uint8_t choiceIndex = 0;
  };

  struct MiniGameState {
    bool active = false;
    bool queuedNextRound = false;
    uint8_t round = 0;
    uint8_t totalRounds = 3;
    uint8_t score = 0;
    uint8_t cursor = 0;
    int8_t direction = 1;
    uint8_t targetStart = 0;
    uint8_t targetWidth = 0;
    uint32_t lastMoveMs = 0;
    uint32_t waitUntilMs = 0;
    String prompt;
    String result;
  };

  struct PersistedState {
    uint32_t magic;
    char name[16];
    uint8_t species;
    uint8_t hunger;
    uint8_t energy;
    uint8_t joy;
    uint8_t hygiene;
    uint8_t bond;
    uint8_t messes;
    uint8_t asleep;
    uint16_t neglectMinutes;
    uint32_t ageMinutes;
    uint16_t discoveries;
  };

  static constexpr uint32_t kStateMagic = 0x50455431UL;
  static constexpr uint32_t kGameMinuteMs = 4000UL;
  static constexpr uint32_t kAutosaveMs = 45000UL;
  static constexpr uint32_t kEffectDurationMs = 1800UL;
  static constexpr uint8_t kActionCount = 7;
  static constexpr uint8_t kSpeciesCount = 4;

  PersistedState state_{};
  uint32_t lastMinuteTickMs_ = 0;
  uint32_t lastAutosaveMs_ = 0;
  bool dirty_ = false;
  String notification_;
  uint8_t selectedActionIndex_ = 0;
  EffectState effect_{};
  SpeciesMenuState speciesMenu_{};
  MiniGameState miniGame_{};

  void loadOrCreateState();
  void resetState();
  void saveState();
  void advanceMinutes(uint16_t minutes, bool allowNotification);
  void applyMinute();
  void clampState();
  void markInteraction();
  void queueNotification(const String& message);
  void startEffect(Effect type, const String& label);
  Action selectedAction() const;
  Species currentSpecies() const;
  String performAction(Action action);
  String actionLabel(Action action) const;
  String actionHint(Action action) const;
  String titleLabel() const;
  String speciesLabel(Species species) const;
  String defaultNameForSpecies(Species species) const;
  void adoptSpecies(Species species, bool resetProgress);
  void startSpeciesMenu();
  void nextSpeciesChoice();
  String confirmSpeciesChoice();
  void startMiniGame();
  void configureMiniGameRound();
  void updateMiniGame(uint32_t now);
  String handleMiniGamePress();
  String finishMiniGame(const String& lastJudgement);

  Mood currentMood() const;
  String faceForMood(Mood mood) const;
  String moodLabel(Mood mood) const;
  String hintForMood(Mood mood) const;
  String bar(const String& label, uint8_t value) const;
  String formatAge() const;
  String normalizedName(const String& raw) const;
  String summaryAfterAction(const String& headline) const;
};
