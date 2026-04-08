#include "PetGame.h"

#include <Preferences.h>

namespace {
Preferences gStorage;
}

void PetGame::begin() {
  randomSeed(esp_random());
  loadOrCreateState();
  lastMinuteTickMs_ = millis();
  lastAutosaveMs_ = millis();
}

void PetGame::update() {
  const uint32_t now = millis();
  updateMiniGame(now);
  if (effect_.active && now >= effect_.endsAtMs) {
    effect_ = EffectState{};
  }

  if (now - lastMinuteTickMs_ >= kGameMinuteMs) {
    uint32_t elapsed = now - lastMinuteTickMs_;
    uint16_t minutes = elapsed / kGameMinuteMs;
    if (minutes > 90) {
      minutes = 90;
    }
    lastMinuteTickMs_ += static_cast<uint32_t>(minutes) * kGameMinuteMs;
    advanceMinutes(minutes, true);
  }

  if (dirty_ && now - lastAutosaveMs_ >= kAutosaveMs) {
    saveState();
  }
}

String PetGame::handleCommand(const String& rawInput) {
  String raw = rawInput;
  raw.trim();
  String input = raw;
  input.toLowerCase();

  if (raw.isEmpty()) {
    return "Type help to see the commands.";
  }

  if (input == "help" || input == "?") {
    return commandHelp();
  }

  if (input == "status" || input == "stats" || input == "s") {
    return renderStatus();
  }

  if (input == "species" || input == "pets" || input == "roster") {
    String output = "Pets:\n";
    for (uint8_t i = 0; i < kSpeciesCount; ++i) {
      const Species species = static_cast<Species>(i);
      output += "  ";
      output += speciesLabel(species);
      if (species == currentSpecies()) {
        output += "  (current)";
      }
      output += "\n";
    }
    output += "\nUse: adopt lion | adopt tiger | adopt cub | adopt robocat";
    return output;
  }

  if (input.startsWith("adopt ")) {
    const String which = input.substring(6);
    if (which == "lion") {
      adoptSpecies(Species::kLion, true);
      saveState();
      return summaryAfterAction("Leo the lion bounds onto the desk.");
    }
    if (which == "tiger") {
      adoptSpecies(Species::kTiger, true);
      saveState();
      return summaryAfterAction("Tora the tiger stalks into view.");
    }
    if (which == "cub" || which == "cat" || which == "kitten") {
      adoptSpecies(Species::kCubCat, true);
      saveState();
      return summaryAfterAction("Mochi the cub cat curls up nearby.");
    }
    if (which == "robocat" || which == "robotcat" || which == "robot") {
      adoptSpecies(Species::kRoboCat, true);
      saveState();
      return summaryAfterAction("Nova the robo cat boots up with a tiny chime.");
    }
    return "Unknown pet. Try adopt lion, adopt tiger, adopt cub, or adopt robocat.";
  }

  if (input == "feed" || input == "f") {
    return performAction(Action::kFeed);
  }

  if (input == "play" || input == "p") {
    return performAction(Action::kPlay);
  }

  if (input == "pet" || input == "touch" || input == "pat") {
    return performAction(Action::kPet);
  }

  if (input == "clean" || input == "c") {
    return performAction(Action::kClean);
  }

  if (input == "explore" || input == "adventure" || input == "x") {
    return performAction(Action::kExplore);
  }

  if (input == "sleep" || input == "nap") {
    return performAction(Action::kSleep);
  }

  if (input == "wake") {
    if (!state_.asleep) {
      return String(state_.name) + " is already awake.\n\n" + renderStatus();
    }
    return performAction(Action::kSleep);
  }

  if (input == "save") {
    saveState();
    return "Game saved.\n\n" + renderStatus();
  }

  if (input == "new" || input == "reset") {
    resetState();
    saveState();
    return "A brand new desk pet hatches from the serial wire.\n\n" + renderStatus();
  }

  if (input.startsWith("name ")) {
    const String freshName = normalizedName(rawInput.substring(5));
    if (freshName.isEmpty()) {
      return "Pick a short ASCII name, like name mochi";
    }

    memset(state_.name, 0, sizeof(state_.name));
    freshName.toCharArray(state_.name, sizeof(state_.name));
    markInteraction();
    dirty_ = true;
    saveState();
    return summaryAfterAction("Your pet now answers to " + freshName + ".");
  }

  if (input.startsWith("skip ")) {
    long requested = rawInput.substring(5).toInt();
    if (requested <= 0) {
      return "Usage: skip 15";
    }
    if (requested > 720) {
      requested = 720;
    }
    advanceMinutes(static_cast<uint16_t>(requested), true);
    dirty_ = true;
    saveState();
    return "Time jumps forward by " + String(requested) + " in-game minutes.\n\n" + renderStatus();
  }

  return "Unknown command. Type help to see the commands.";
}

String PetGame::commandHelp() const {
  return String()
         + "Controls:\n"
         + "  BOOT tap       next action\n"
         + "  BOOT hold      use action\n"
         + "  RESET/EN       hard reset\n"
         + "\nActions:\n"
         + "  feed           give a snack\n"
         + "  play           open the game action\n"
         + "  pet            build trust\n"
         + "  clean          remove one mess\n"
         + "  explore        random adventure\n"
         + "  sleep          sleep or wake up\n"
         + "  pets           open the pet roster\n"
         + "\nSerial:\n"
          + "  status\n"
         + "  species\n"
         + "  adopt lion | adopt tiger | adopt cub | adopt robocat\n"
         + "  skip 15\n"
         + "  name mochi\n"
         + "  save\n"
         + "  new";
}

String PetGame::renderStatus() const {
  const Mood mood = currentMood();
  const uint8_t fullness = static_cast<uint8_t>(100 - state_.hunger);

  String output;
  output.reserve(512);
  output += "== ";
  output += state_.name;
  output += " ==\n";
  output += "Pet: ";
  output += speciesLabel(currentSpecies());
  output += "\n";
  output += "Title: ";
  output += titleLabel();
  output += " | Finds: ";
  output += String(state_.discoveries);
  output += "\n";
  output += "Mood: ";
  output += moodLabel(mood);
  output += " | ";
  output += state_.asleep ? "Asleep" : "Awake";
  output += " | Age: ";
  output += formatAge();
  output += "\n";
  output += "Messes: ";
  output += String(state_.messes);
  output += "\n";
  output += faceForMood(mood);
  output += "\n";
  output += bar("Full", fullness);
  output += bar("Energy", state_.energy);
  output += bar("Joy", state_.joy);
  output += bar("Clean", state_.hygiene);
  output += bar("Bond", state_.bond);
  output += "Action: ";
  output += actionLabel(selectedAction());
  output += "\n";
  output += "\nHint: ";
  output += hintForMood(mood);
  return output;
}

String PetGame::consumeNotification() {
  const String out = notification_;
  notification_ = "";
  return out;
}

PetGame::Snapshot PetGame::snapshot() const {
  const Mood activeMood = currentMood();

  Snapshot view;
  view.name = state_.name;
  view.species = currentSpecies();
  view.speciesLabel = speciesLabel(view.species);
  view.mood = activeMood;
  view.asleep = state_.asleep;
  view.fullness = static_cast<uint8_t>(100 - state_.hunger);
  view.energy = state_.energy;
  view.joy = state_.joy;
  view.hygiene = state_.hygiene;
  view.bond = state_.bond;
  view.messes = state_.messes;
  view.ageMinutes = state_.ageMinutes;
  view.face = faceForMood(activeMood);
  view.hint = hintForMood(activeMood);
  view.title = titleLabel();
  view.selectedAction = selectedAction();
  view.selectedActionLabel = actionLabel(view.selectedAction);
  view.selectedActionHint = actionHint(view.selectedAction);
  view.discoveries = state_.discoveries;
  view.miniGameActive = miniGame_.active;
  view.miniRound = miniGame_.round;
  view.miniTotalRounds = miniGame_.totalRounds;
  view.miniScore = miniGame_.score;
  view.miniCursor = miniGame_.cursor;
  view.miniTargetStart = miniGame_.targetStart;
  view.miniTargetWidth = miniGame_.targetWidth;
  view.miniPrompt = miniGame_.prompt;
  view.miniResult = miniGame_.result;
  view.speciesMenuActive = speciesMenu_.active;
  view.speciesChoice = static_cast<Species>(speciesMenu_.choiceIndex % kSpeciesCount);
  view.speciesChoiceLabel = speciesLabel(view.speciesChoice);
  view.effectActive = effect_.active;
  view.effect = effect_.type;
  view.effectProgress =
      effect_.active ? static_cast<uint8_t>(constrain(((millis() - effect_.startedAtMs) * 100) /
                                                          max<uint32_t>(1, effect_.endsAtMs - effect_.startedAtMs),
                                                      0UL, 100UL))
                     : 0;
  view.effectLabel = effect_.label;
  return view;
}

PetGame::Mood PetGame::mood() const {
  return currentMood();
}

void PetGame::nextAction() {
  if (miniGame_.active || speciesMenu_.active) {
    return;
  }
  selectedActionIndex_ = (selectedActionIndex_ + 1) % kActionCount;
}

String PetGame::activateSelectedAction() {
  if (miniGame_.active) {
    return handleMiniGamePress();
  }
  if (speciesMenu_.active) {
    return confirmSpeciesChoice();
  }
  return performAction(selectedAction());
}

String PetGame::selectedActionLabel() const {
  return actionLabel(selectedAction());
}

String PetGame::handleButtonTap() {
  if (miniGame_.active) {
    return handleMiniGamePress();
  }
  if (speciesMenu_.active) {
    nextSpeciesChoice();
    return "Pet " + speciesLabel(static_cast<Species>(speciesMenu_.choiceIndex));
  }

  nextAction();
  return "Selected " + selectedActionLabel();
}

String PetGame::handleButtonHold() {
  if (miniGame_.active) {
    return handleMiniGamePress();
  }
  if (speciesMenu_.active) {
    return confirmSpeciesChoice();
  }

  return activateSelectedAction();
}

void PetGame::loadOrCreateState() {
  gStorage.begin("desk-pet", false);

  PersistedState saved{};
  bool loaded = false;
  if (gStorage.getBytesLength("state") == sizeof(saved)) {
    const size_t read = gStorage.getBytes("state", &saved, sizeof(saved));
    loaded = read == sizeof(saved) && saved.magic == kStateMagic;
  }

  if (loaded) {
    state_ = saved;
    clampState();
    return;
  }

  resetState();
  saveState();
  queueNotification("New pet online. Tap BOOT to switch. Hold to use.");
}

void PetGame::resetState() {
  memset(&state_, 0, sizeof(state_));
  state_.magic = kStateMagic;
  state_.species = static_cast<uint8_t>(Species::kCubCat);
  strlcpy(state_.name, defaultNameForSpecies(Species::kCubCat).c_str(), sizeof(state_.name));
  state_.hunger = 28;
  state_.energy = 76;
  state_.joy = 72;
  state_.hygiene = 84;
  state_.bond = 50;
  state_.messes = 0;
  state_.asleep = 0;
  state_.neglectMinutes = 0;
  state_.ageMinutes = 0;
  state_.discoveries = 0;
  dirty_ = true;
  effect_ = EffectState{};
  speciesMenu_ = SpeciesMenuState{};
  miniGame_ = MiniGameState{};
}

void PetGame::saveState() {
  state_.magic = kStateMagic;
  gStorage.putBytes("state", &state_, sizeof(state_));
  lastAutosaveMs_ = millis();
  dirty_ = false;
}

void PetGame::startEffect(Effect type, const String& label) {
  effect_.active = true;
  effect_.type = type;
  effect_.startedAtMs = millis();
  effect_.endsAtMs = effect_.startedAtMs + kEffectDurationMs;
  effect_.label = label;
}

PetGame::Action PetGame::selectedAction() const {
  return static_cast<Action>(selectedActionIndex_ % kActionCount);
}

PetGame::Species PetGame::currentSpecies() const {
  return static_cast<Species>(state_.species % kSpeciesCount);
}

String PetGame::speciesLabel(Species species) const {
  switch (species) {
    case Species::kCubCat:
      return "CUB CAT";
    case Species::kLion:
      return "LION";
    case Species::kTiger:
      return "TIGER";
    case Species::kRoboCat:
      return "ROBO CAT";
  }

  return "PET";
}

String PetGame::defaultNameForSpecies(Species species) const {
  switch (species) {
    case Species::kCubCat:
      return "Mochi";
    case Species::kLion:
      return "Leo";
    case Species::kTiger:
      return "Tora";
    case Species::kRoboCat:
      return "Nova";
  }

  return "Mochi";
}

void PetGame::adoptSpecies(Species species, bool resetProgress) {
  state_.species = static_cast<uint8_t>(species);
  memset(state_.name, 0, sizeof(state_.name));
  defaultNameForSpecies(species).toCharArray(state_.name, sizeof(state_.name));

  if (resetProgress) {
    state_.hunger = 26;
    state_.energy = 82;
    state_.joy = 74;
    state_.hygiene = species == Species::kRoboCat ? 92 : 84;
    state_.bond = 46;
    state_.messes = 0;
    state_.asleep = 0;
    state_.neglectMinutes = 0;
    state_.ageMinutes = 0;
    state_.discoveries = 0;
  }

  effect_ = EffectState{};
  speciesMenu_ = SpeciesMenuState{};
  miniGame_ = MiniGameState{};
  startEffect(Effect::kAdopt, "New friend joined!");
  markInteraction();
  clampState();
  dirty_ = true;
}

void PetGame::startSpeciesMenu() {
  speciesMenu_.active = true;
  speciesMenu_.choiceIndex = static_cast<uint8_t>(currentSpecies());
  effect_ = EffectState{};
}

void PetGame::nextSpeciesChoice() {
  if (!speciesMenu_.active) {
    return;
  }
  speciesMenu_.choiceIndex = (speciesMenu_.choiceIndex + 1) % kSpeciesCount;
}

String PetGame::confirmSpeciesChoice() {
  if (!speciesMenu_.active) {
    return "";
  }

  const Species chosen = static_cast<Species>(speciesMenu_.choiceIndex);
  speciesMenu_.active = false;
  adoptSpecies(chosen, true);
  saveState();
  return summaryAfterAction(speciesLabel(chosen) + " joins your desk team.");
}

String PetGame::performAction(Action action) {
  const Species species = currentSpecies();
  switch (action) {
    case Action::kFeed: {
      if (state_.asleep) {
        state_.asleep = 0;
        queueNotification(String(state_.name) + " woke up for a snack.");
      }

      const bool alreadyFull = state_.hunger < 18;
      int hungerGain = alreadyFull ? 12 : 28;
      int joyGain = alreadyFull ? 2 : 6;
      String headline = String(state_.name) + " happily munches the snack.";
      String effectLabel = "Snack time!";

      if (species == Species::kLion) {
        hungerGain += 4;
        joyGain += 1;
        headline = String(state_.name) + " crunches a giant meatball treat.";
        effectLabel = "Lion feast";
      } else if (species == Species::kTiger) {
        joyGain += 2;
        headline = String(state_.name) + " pounces on a stripey fish snack.";
        effectLabel = "Tiger snack";
      } else if (species == Species::kRoboCat) {
        hungerGain -= 4;
        state_.energy = static_cast<uint8_t>(min(100, state_.energy + 10));
        state_.hygiene = static_cast<uint8_t>(min(100, state_.hygiene + 2));
        headline = String(state_.name) + " recharges on a glowing tuna battery.";
        effectLabel = "Power snack";
      }

      state_.hunger = static_cast<uint8_t>(max(0, state_.hunger - hungerGain));
      state_.joy = static_cast<uint8_t>(min(100, state_.joy + joyGain));
      state_.bond = static_cast<uint8_t>(min(100, state_.bond + 3));
      state_.hygiene = static_cast<uint8_t>(max(0, state_.hygiene - 1));
      markInteraction();
      clampState();
      dirty_ = true;
      saveState();
      startEffect(Effect::kFeed, effectLabel);
      return summaryAfterAction(alreadyFull ? String(state_.name) + " takes a polite nibble." : headline);
    }

    case Action::kPlay: {
      if (state_.asleep) {
        return "Your pet is sleeping. Try wake first.\n\n" + renderStatus();
      }
      if (state_.energy < 12) {
        return String(state_.name) + " is too sleepy to play right now.\n\n" + renderStatus();
      }
      if (miniGame_.active) {
        return handleMiniGamePress();
      }
      startMiniGame();
      return "STAR CHASE started.\n\nTap BOOT when the spark lands in the heart zone.";
    }

    case Action::kPet: {
      uint8_t joyGain = 8;
      uint8_t bondGain = 10;
      String headline = String(state_.name) + " leans into the attention.";
      String effectLabel = "Warm cuddle";
      if (species == Species::kLion) {
        joyGain = 6;
        bondGain = 12;
        headline = String(state_.name) + " rumbles a proud little purr.";
        effectLabel = "Lion nuzzle";
      } else if (species == Species::kTiger) {
        joyGain = 10;
        bondGain = 9;
        headline = String(state_.name) + " bumps your hand with a playful headbutt.";
        effectLabel = "Tiger cuddle";
      } else if (species == Species::kRoboCat) {
        joyGain = 7;
        bondGain = 11;
        headline = String(state_.name) + " lights up and chirps a happy beep.";
        effectLabel = "Robot purr";
      }
      state_.joy = static_cast<uint8_t>(min(100, state_.joy + joyGain));
      state_.bond = static_cast<uint8_t>(min(100, state_.bond + bondGain));
      if (state_.asleep) {
        state_.energy = static_cast<uint8_t>(min(100, state_.energy + 2));
      }
      markInteraction();
      clampState();
      dirty_ = true;
      saveState();
      startEffect(Effect::kPet, effectLabel);
      return summaryAfterAction(headline);
    }

    case Action::kClean: {
      state_.hygiene = static_cast<uint8_t>(min(100, state_.hygiene + 32));
      if (state_.messes > 0) {
        state_.messes--;
      }
      state_.joy = static_cast<uint8_t>(min(100, state_.joy + 4));
      if (species == Species::kRoboCat) {
        state_.energy = static_cast<uint8_t>(min(100, state_.energy + 4));
      }
      markInteraction();
      clampState();
      dirty_ = true;
      saveState();
      startEffect(Effect::kClean, species == Species::kRoboCat ? "Polish mode" : "Fresh and clean");
      return summaryAfterAction("Everything smells much better now.");
    }

    case Action::kExplore: {
      if (state_.asleep) {
        return String(state_.name) + " is asleep. Wake it before exploring.\n\n" + renderStatus();
      }
      if (state_.energy < 16) {
        return String(state_.name) + " is too tired for an adventure.\n\n" + renderStatus();
      }

      const long roll = random(6);
      String headline;
      switch (roll) {
        case 0:
          state_.hunger = static_cast<uint8_t>(max(0, state_.hunger - 16));
          state_.joy = static_cast<uint8_t>(min(100, state_.joy + 8));
          state_.bond = static_cast<uint8_t>(min(100, state_.bond + 4));
          state_.discoveries++;
          headline = String(state_.name) + " found a moonberry in a drawer corner.";
          break;
        case 1:
          state_.joy = static_cast<uint8_t>(min(100, state_.joy + 14));
          state_.energy = static_cast<uint8_t>(max(0, state_.energy - 10));
          state_.bond = static_cast<uint8_t>(min(100, state_.bond + 5));
          headline = String(state_.name) + " chased a prism moth across the desk.";
          break;
        case 2:
          state_.joy = static_cast<uint8_t>(min(100, state_.joy + 10));
          state_.bond = static_cast<uint8_t>(min(100, state_.bond + 9));
          state_.discoveries++;
          headline = String(state_.name) + " dug up a shiny sticker treasure.";
          break;
        case 3:
          state_.joy = static_cast<uint8_t>(min(100, state_.joy + 8));
          state_.energy = static_cast<uint8_t>(max(0, state_.energy - 6));
          state_.hygiene = static_cast<uint8_t>(max(0, state_.hygiene - 16));
          state_.messes = static_cast<uint8_t>(min(3, state_.messes + 1));
          headline = String(state_.name) + " rolled through dusty cable tunnels.";
          break;
        case 4:
          state_.energy = static_cast<uint8_t>(min(100, state_.energy + 10));
          state_.joy = static_cast<uint8_t>(min(100, state_.joy + 10));
          state_.bond = static_cast<uint8_t>(min(100, state_.bond + 6));
          state_.discoveries++;
          headline = String(state_.name) + " met a tiny cloud friend by the monitor.";
          break;
        default:
          state_.joy = static_cast<uint8_t>(max(0, state_.joy - 4));
          state_.energy = static_cast<uint8_t>(max(0, state_.energy - 4));
          state_.bond = static_cast<uint8_t>(min(100, state_.bond + 8));
          headline = String(state_.name) + " got spooked, then came back for a brave cuddle.";
          break;
      }

      state_.hunger = static_cast<uint8_t>(min(100, state_.hunger + 6));
      if (species == Species::kTiger) {
        state_.joy = static_cast<uint8_t>(min(100, state_.joy + 4));
      } else if (species == Species::kLion) {
        state_.bond = static_cast<uint8_t>(min(100, state_.bond + 2));
      } else if (species == Species::kRoboCat) {
        state_.discoveries += random(100) < 20 ? 1 : 0;
      }
      markInteraction();
      clampState();
      dirty_ = true;
      saveState();
      startEffect(Effect::kExplore, species == Species::kTiger ? "Stripe sprint" : "Adventure time");
      return summaryAfterAction(headline);
    }

    case Action::kSleep: {
      state_.asleep = !state_.asleep;
      markInteraction();
      dirty_ = true;
      saveState();
      startEffect(Effect::kSleep, state_.asleep ? "Dream bubble" : "Wake stretch");
      return summaryAfterAction(state_.asleep ? String(state_.name) + " curls up and dozes off."
                                              : String(state_.name) + " blinks awake.");
    }

    case Action::kRoster: {
      if (speciesMenu_.active) {
        return confirmSpeciesChoice();
      }
      startSpeciesMenu();
      return "ROSTER opened.\n\nTap BOOT to browse pets. Hold to adopt.";
    }
  }

  return renderStatus();
}

String PetGame::actionLabel(Action action) const {
  switch (action) {
    case Action::kFeed:
      return "SNACK";
    case Action::kPlay:
      return "GAME";
    case Action::kPet:
      return "PET";
    case Action::kClean:
      return "CLEAN";
    case Action::kExplore:
      return "EXPLORE";
    case Action::kSleep:
      return state_.asleep ? "WAKE" : "NAP";
    case Action::kRoster:
      return "PETS";
  }

  return "ACT";
}

String PetGame::actionHint(Action action) const {
  switch (action) {
    case Action::kFeed:
      return "Give a snack and lift the mood.";
    case Action::kPlay:
      return "Start Star Chase. Hit the heart zone.";
    case Action::kPet:
      return "A calm cuddle to build trust.";
    case Action::kClean:
      return "Brush away one mess and tidy up.";
    case Action::kExplore:
      return "Roll the dice for an adventure.";
    case Action::kSleep:
      return state_.asleep ? "Wake up and get moving again." : "Take a nap to restore energy.";
    case Action::kRoster:
      return "Browse lion, tiger, cub cat, and robo cat.";
  }

  return "";
}

String PetGame::titleLabel() const {
  if (state_.ageMinutes >= 720 && state_.discoveries >= 8 && state_.bond >= 85) {
    return "STAR PAL";
  }
  if (state_.discoveries >= 5 && state_.bond >= 65) {
    return "SCOUT";
  }
  if (state_.bond >= 55 || state_.ageMinutes >= 120) {
    return "SIDEKICK";
  }
  return "HATCHLING";
}

void PetGame::advanceMinutes(uint16_t minutes, bool allowNotification) {
  const Mood moodBefore = currentMood();
  String localEvent;

  for (uint16_t i = 0; i < minutes; ++i) {
    applyMinute();

    if (!state_.asleep && state_.hygiene <= 35 && state_.messes < 3 && random(100) < 5) {
      state_.messes++;
      state_.hygiene = static_cast<uint8_t>(max(0, state_.hygiene - 16));
      localEvent = String(state_.name) + " made a tiny desk mess.";
    } else if (!state_.asleep && state_.joy >= 75 && state_.bond >= 60 && random(100) < 4) {
      localEvent = String(state_.name) + " hums a happy little tune.";
    } else if (!state_.asleep && state_.neglectMinutes >= 20 && random(100) < 5) {
      localEvent = String(state_.name) + " wants a little attention.";
    }
  }

  clampState();
  dirty_ = true;

  const Mood moodAfter = currentMood();
  if (allowNotification) {
    if (!localEvent.isEmpty()) {
      queueNotification(localEvent);
    } else if (moodAfter != moodBefore) {
      queueNotification(String(state_.name) + " now feels " + moodLabel(moodAfter) + ".");
    }
  }
}

void PetGame::applyMinute() {
  const Species species = currentSpecies();
  state_.ageMinutes++;

  if (state_.asleep) {
    int energyRecover = 4;
    int hungerRise = 2;
    if (species == Species::kLion) {
      energyRecover = 5;
      hungerRise = 3;
    } else if (species == Species::kRoboCat) {
      energyRecover = 3;
      hungerRise = 1;
    }
    state_.energy = static_cast<uint8_t>(min(100, state_.energy + energyRecover));
    state_.hunger = static_cast<uint8_t>(min(100, state_.hunger + hungerRise));
    state_.joy = static_cast<uint8_t>(max(0, state_.joy - 1));
    state_.neglectMinutes = 0;
  } else {
    int hungerRise = 3;
    int energyDrop = 2;
    if (species == Species::kLion) {
      hungerRise = 4;
    } else if (species == Species::kTiger) {
      energyDrop = 3;
    } else if (species == Species::kRoboCat) {
      hungerRise = 2;
      energyDrop = 1;
    }
    state_.hunger = static_cast<uint8_t>(min(100, state_.hunger + hungerRise));
    state_.energy = static_cast<uint8_t>(max(0, state_.energy - energyDrop));
    state_.joy = static_cast<uint8_t>(max(0, state_.joy - 1));
    state_.neglectMinutes++;

    if (state_.ageMinutes % (species == Species::kRoboCat ? 5 : 3) == 0) {
      state_.hygiene = static_cast<uint8_t>(max(0, state_.hygiene - 1));
    }
  }

  if (state_.hunger >= 80) {
    state_.joy = static_cast<uint8_t>(max(0, state_.joy - 1));
  }
  if (state_.energy <= 20) {
    state_.joy = static_cast<uint8_t>(max(0, state_.joy - 1));
  }
  if (state_.hygiene <= 25 || state_.messes >= 2) {
    state_.joy = static_cast<uint8_t>(max(0, state_.joy - 1));
  }
  if (state_.neglectMinutes >= 30 && state_.ageMinutes % 5 == 0) {
    state_.bond = static_cast<uint8_t>(max(0, state_.bond - 1));
  }

  if (!state_.asleep && state_.ageMinutes % 12 == 0 && state_.discoveries < 999) {
    if (state_.joy >= 70 && state_.bond >= 65 && random(100) < 18) {
      state_.discoveries++;
      queueNotification(String(state_.name) + " discovered a new hiding spot.");
    }
  }
}

void PetGame::startMiniGame() {
  miniGame_ = MiniGameState{};
  miniGame_.active = true;
  miniGame_.round = 1;
  miniGame_.totalRounds = 3;
  miniGame_.score = 0;
  miniGame_.prompt = "Tap or hold BOOT when the spark reaches the heart.";
  startEffect(Effect::kExplore, "Star Chase");
  configureMiniGameRound();
}

void PetGame::configureMiniGameRound() {
  miniGame_.cursor = static_cast<uint8_t>(random(4, 18));
  miniGame_.direction = random(2) == 0 ? 1 : -1;
  miniGame_.targetWidth = static_cast<uint8_t>(random(currentSpecies() == Species::kLion ? 18 : 14,
                                                      currentSpecies() == Species::kTiger ? 22 : 24));
  miniGame_.targetStart = static_cast<uint8_t>(random(24, 76));
  miniGame_.lastMoveMs = millis();
  miniGame_.waitUntilMs = 0;
  miniGame_.queuedNextRound = false;
  miniGame_.result = "";
}

void PetGame::updateMiniGame(uint32_t now) {
  if (!miniGame_.active) {
    return;
  }

  if (miniGame_.queuedNextRound) {
    if (now >= miniGame_.waitUntilMs) {
      miniGame_.queuedNextRound = false;
      miniGame_.round++;
      miniGame_.prompt = "Round " + String(miniGame_.round) + ": hit the heart zone.";
      configureMiniGameRound();
    }
    return;
  }

  if (miniGame_.waitUntilMs != 0 && now < miniGame_.waitUntilMs) {
    return;
  }

  if (now - miniGame_.lastMoveMs < 36) {
    return;
  }

  miniGame_.lastMoveMs = now;
  const int16_t next = static_cast<int16_t>(miniGame_.cursor) + miniGame_.direction * 3;
  if (next <= 6) {
    miniGame_.cursor = 6;
    miniGame_.direction = 1;
  } else if (next >= 94) {
    miniGame_.cursor = 94;
    miniGame_.direction = -1;
  } else {
    miniGame_.cursor = static_cast<uint8_t>(next);
  }
}

String PetGame::handleMiniGamePress() {
  if (!miniGame_.active) {
    return "";
  }

  const uint32_t now = millis();
  if (miniGame_.queuedNextRound || (miniGame_.waitUntilMs != 0 && now < miniGame_.waitUntilMs)) {
    return "";
  }

  const int16_t zoneStart = miniGame_.targetStart;
  const int16_t zoneEnd = miniGame_.targetStart + miniGame_.targetWidth;
  const int16_t zoneCenter = zoneStart + miniGame_.targetWidth / 2;
  const int16_t distance = abs(static_cast<int16_t>(miniGame_.cursor) - zoneCenter);

  String judgement;
  uint8_t points = 0;
  if (miniGame_.cursor >= zoneStart && miniGame_.cursor <= zoneEnd) {
    if (distance <= miniGame_.targetWidth / 5) {
      judgement = "Perfect";
      points = 2;
    } else {
      judgement = "Good";
      points = 1;
    }
  } else {
    judgement = "Miss";
  }

  miniGame_.score = static_cast<uint8_t>(miniGame_.score + points);
  miniGame_.result = judgement;

  if (miniGame_.round >= miniGame_.totalRounds) {
    return finishMiniGame(judgement);
  }

  miniGame_.queuedNextRound = true;
  miniGame_.waitUntilMs = now + 650;
  miniGame_.prompt = "Round " + String(miniGame_.round) + ": " + judgement + ". Get ready...";
  return "Round " + String(miniGame_.round) + ": " + judgement + ".";
}

String PetGame::finishMiniGame(const String& lastJudgement) {
  String headline;
  const Species species = currentSpecies();

  if (miniGame_.score >= 5) {
    state_.joy = static_cast<uint8_t>(min(100, state_.joy + 20));
    state_.bond = static_cast<uint8_t>(min(100, state_.bond + 10));
    state_.discoveries++;
    headline = String(state_.name) + " wins STAR CHASE with a brilliant " + lastJudgement + "!";
  } else if (miniGame_.score >= 3) {
    state_.joy = static_cast<uint8_t>(min(100, state_.joy + 12));
    state_.bond = static_cast<uint8_t>(min(100, state_.bond + 6));
    headline = String(state_.name) + " clears STAR CHASE with " + String(miniGame_.score) + " points.";
  } else if (miniGame_.score >= 1) {
    state_.joy = static_cast<uint8_t>(min(100, state_.joy + 6));
    state_.bond = static_cast<uint8_t>(min(100, state_.bond + 3));
    headline = String(state_.name) + " learns the game with " + String(miniGame_.score) + " point.";
  } else {
    state_.energy = static_cast<uint8_t>(max(0, state_.energy - 4));
    headline = String(state_.name) + " misses the spark, but wants another try.";
  }

  if (species == Species::kLion && miniGame_.score >= 3) {
    state_.bond = static_cast<uint8_t>(min(100, state_.bond + 3));
  } else if (species == Species::kTiger && miniGame_.score >= 3) {
    state_.joy = static_cast<uint8_t>(min(100, state_.joy + 4));
  } else if (species == Species::kRoboCat && miniGame_.score >= 3) {
    state_.energy = static_cast<uint8_t>(min(100, state_.energy + 6));
  }

  state_.hunger = static_cast<uint8_t>(min(100, state_.hunger + 4));
  markInteraction();
  clampState();
  dirty_ = true;
  saveState();
  miniGame_ = MiniGameState{};
  return summaryAfterAction(headline);
}

void PetGame::clampState() {
  state_.hunger = constrain(state_.hunger, 0, 100);
  state_.energy = constrain(state_.energy, 0, 100);
  state_.joy = constrain(state_.joy, 0, 100);
  state_.hygiene = constrain(state_.hygiene, 0, 100);
  state_.bond = constrain(state_.bond, 0, 100);
  state_.messes = constrain(state_.messes, 0, 3);
}

void PetGame::markInteraction() {
  state_.neglectMinutes = 0;
}

void PetGame::queueNotification(const String& message) {
  if (message.isEmpty()) {
    return;
  }
  notification_ = message;
}

PetGame::Mood PetGame::currentMood() const {
  if (state_.asleep) {
    return Mood::kSleepy;
  }
  if (state_.hunger >= 82) {
    return Mood::kHungry;
  }
  if (state_.energy <= 18) {
    return Mood::kSleepy;
  }
  if (state_.hygiene <= 25 || state_.messes >= 2) {
    return Mood::kDirty;
  }
  if (state_.bond <= 22) {
    return Mood::kLonely;
  }
  if (state_.joy <= 24) {
    return Mood::kUpset;
  }
  if (state_.joy >= 76 && state_.bond >= 55) {
    return Mood::kHappy;
  }
  return Mood::kContent;
}

String PetGame::faceForMood(Mood mood) const {
  switch (mood) {
    case Mood::kHappy:
      return " /\\_/\\\\\n( ^.^ )\n / > <";
    case Mood::kHungry:
      return " /\\_/\\\\\n( o.o )\n / 0  \\\n";
    case Mood::kSleepy:
      return " /\\_/\\\\\n( -.- )\n z  z  z";
    case Mood::kDirty:
      return " /\\_/\\\\\n( x.x )\n / ~ ~\\";
    case Mood::kLonely:
      return " /\\_/\\\\\n( ;.; )\n /   < ";
    case Mood::kUpset:
      return " /\\_/\\\\\n( >.< )\n / !!!\\";
    case Mood::kContent:
    default:
      return " /\\_/\\\\\n( o.o )\n / > <";
  }
}

String PetGame::moodLabel(Mood mood) const {
  switch (mood) {
    case Mood::kHappy:
      return "HAPPY";
    case Mood::kHungry:
      return "HUNGRY";
    case Mood::kSleepy:
      return state_.asleep ? "SLEEPING" : "SLEEPY";
    case Mood::kDirty:
      return "DIRTY";
    case Mood::kLonely:
      return "LONELY";
    case Mood::kUpset:
      return "UPSET";
    case Mood::kContent:
    default:
      return "CONTENT";
  }
}

String PetGame::hintForMood(Mood mood) const {
  switch (mood) {
    case Mood::kHappy:
      return "Things are glowing. Maybe try explore.";
    case Mood::kHungry:
      return "Try feed before the mood drops further.";
    case Mood::kSleepy:
      return state_.asleep ? "A nap restores energy over time." : "Try nap or slow down the play.";
    case Mood::kDirty:
      return "Use clean, especially with more than one mess.";
    case Mood::kLonely:
      return "A little pet or play will rebuild trust.";
    case Mood::kUpset:
      return "Recover with pet, food, nap, or cleaning.";
    case Mood::kContent:
    default:
      return "Calm and ready. Try play or explore.";
  }
}

String PetGame::bar(const String& label, uint8_t value) const {
  const uint8_t filled = value / 10;

  String output = label;
  while (output.length() < 6) {
    output += ' ';
  }
  output += "[";
  for (uint8_t i = 0; i < 10; ++i) {
    output += (i < filled) ? '#' : '-';
  }
  output += "] ";
  if (value < 10) {
    output += " ";
  }
  if (value < 100) {
    output += " ";
  }
  output += String(value);
  output += "\n";
  return output;
}

String PetGame::formatAge() const {
  const uint32_t hours = state_.ageMinutes / 60;
  const uint32_t minutes = state_.ageMinutes % 60;
  return String(hours) + "h " + String(minutes) + "m";
}

String PetGame::normalizedName(const String& raw) const {
  String trimmed = raw;
  trimmed.trim();
  if (trimmed.isEmpty()) {
    return "";
  }

  String clean;
  clean.reserve(15);
  for (size_t i = 0; i < trimmed.length() && clean.length() < 15; ++i) {
    const char c = trimmed[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
        c == '-' || c == '_') {
      clean += c;
    }
  }

  return clean;
}

String PetGame::summaryAfterAction(const String& headline) const {
  return headline + "\n\n" + renderStatus();
}
