#include "audio/bank.hpp"
#include "audio/events.hpp"
#include "audio/limiter.hpp"
#include "audio/policy.hpp"
#include "game/session.hpp"
#include "support/check.hpp"
#include "support/hybrid_fixtures.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
using namespace blobber;
int main() {
    audio::PeakLimiter limiter;
    limiter.reset(48000);
    std::array<float, 4> quiet{{.2f, -.1f, -.5f, .4f}};
    const auto unchanged = quiet;
    limiter.process(quiet.data(), 2);
    check(quiet == unchanged, "Quiet stereo passes unchanged");
    std::array<float, 4> hot{{8.f, -4.f, -16.f, 8.f}};
    limiter.process(hot.data(), 2);
    for (float sample : hot)
        check(std::abs(sample) <= audio::PeakLimiter::ceiling + 1e-6f,
              "Summed positive/negative peaks stay below the output ceiling");
    check(std::abs(hot[0] + 2 * hot[1]) < 1e-6f && std::abs(hot[2] + 2 * hot[3]) < 1e-6f,
          "Linked gain preserves channel balance under limiting");
    float previousGain = 0;
    for (unsigned i = 0; i < 48000; ++i) {
        float frame[]{.1f, -.05f};
        limiter.process(frame, 1);
        check(frame[0] >= previousGain && frame[0] <= .1f, "Release recovers without overshoot");
        previousGain = frame[0];
    }
    check(previousGain > .0999f, "Gain recovers after the overload");
    float invalid[]{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()};
    limiter.process(invalid, 1);
    check(invalid[0] == 0 && invalid[1] == 0, "Invalid samples cannot poison the output");
    std::array<float, 2048> whole{}, chunked{};
    for (unsigned i = 0; i < whole.size(); ++i)
        whole[i] = std::sin(float(i) * .23f) * (i < 512 ? 3.f : .2f);
    chunked = whole;
    limiter.reset(44100);
    limiter.process(whole.data(), 1024);
    limiter.reset(44100);
    for (unsigned i = 0; i < 1024; i += 32)
        limiter.process(chunked.data() + 2 * i, 32);
    check(whole == chunked, "Limiter behavior is independent of callback partitioning");
    limiter.reset(48000);
    quiet = unchanged;
    limiter.process(quiet.data(), 2);
    check(quiet == unchanged, "New device starts with no residual attenuation");
    audio::Variation variation;
    unsigned previous = 16;
    for (unsigned cycle = 0; cycle < 10; ++cycle) {
        std::set<unsigned> seen;
        for (unsigned i = 0; i < 6; ++i) {
            const auto v = variation.select(6);
            check(v != previous, "No immediate repeats including wrap");
            previous = v;
            seen.insert(v);
        }
        check(seen.size() == 6, "Bag covers each sample");
    }
    check(variation.select(1) == 0 && variation.select(1) == 0 && variation.select(0) == 16,
          "Empty and singleton");
    auto left = audio::spatial(-2, 0, 1, 0, 6), right = audio::spatial(2, 0, 1, 0, 6);
    check(left.pan == -1 && right.pan == 1 && left.gain == right.gain, "Signed pan preserves left/right");
    check(audio::spatial(0, 0, 1, 0, 6).gain == 1 && audio::spatial(6, 0, 1, 0, 6).gain == 0,
          "Near/far boundaries");
    Map wall({"#######", "#..#..#", "#.....#", "#.....#", "#######"});
    check(!audio::unobstructed(wall, 1.5, 1.5, 5.5, 1.5), "Wall blocks");
    check(audio::unobstructed(wall, 1.5, 2.5, 5.5, 2.5), "Open path audible");
    FloorFixtures defs;
    defs.count = 1;
    defs.entries[0] = {{3, 2}, Heading::East, FixtureKind::Door};
    FloorGeometry floor(wall, defs);
    check(!audio::unobstructed(floor.view(), 1.5, 2.5, 5.5, 2.5), "Closed door blocks");

    std::array<audio::Voice, 64> voices{};
    for (auto& v : voices) {
        v.active = true;
        v.priority = 4;
        v.gain = 1;
    }
    check(audio::chooseVoice(voices, 4) == 64, "Terminal voices protected");
    voices[60].active = false;
    check(audio::chooseVoice(voices, 1) == 64 && audio::chooseVoice(voices, 3) == 60, "Critical reserve");
    voices[4].priority = 1;
    check(audio::chooseVoice(voices, 2) == 4, "Important voice replaces ordinary");
    voices[4].generation = UINT64_MAX;
    check(audio::chooseVoice(voices, 2) == 64, "Exhausted generation cannot be stolen and reused");
    audio::Journal journal;
    for (unsigned i = 0; i < 1024; ++i)
        journal.emit({"low"});
    journal.emit({"death", {}, 0, 0, false, false, false, 4});
    journal.emit({"low"});
    check(journal.count == 1024 && journal.dropped == 2, "Bounded overflow");
    bool found = false;
    for (unsigned i = 0; i < journal.count; ++i)
        found |= std::strcmp(journal.entries[i].cue, "death") == 0;
    check(found, "Critical event retained");
    journal.clear();
    CombatEvents events;
    events.audioJournal = &journal;
    CombatOutcome attack;
    attack.event = CombatEvent::Attack;
    attack.hit = true;
    attack.damage = 0;
    events.append(attack);
    check(journal.count == 1, "Zero damage no hurt");
    events.clear();
    check(journal.count == 1, "Batch reset preserves journal");
    attack.damage = 1;
    attack.died = true;
    events.append(attack);
    check(journal.count == 3 && journal.entries[2].priority == 3, "Death reaction");
    const auto map = hybridRoom();
    const auto catalog = hybridCatalog();
    NpcWorld empty(map, {5, 8}, {}, {});
    GameSession session(map, catalog, empty, 19, {{5, 8}, Heading::North}, false);
    session.audioEvents().enabled = true;
    session.audioEvents().clear();
    session.move(Action::Forward);
    check(session.audioEvents().count == 0, "Reservation silent");
    session.move(Action::Forward);
    session.update(.16, false);
    check(session.audioEvents().count == 1, "Arrival before queued replacement");
    session.update(10, true);
    check(session.audioEvents().count == 1, "Pause no repeated step");
    session.update(.16, false);
    check(session.audioEvents().count == 2, "Second completed step");
    session.audioEvents().clear();
    session.move(Action::TurnLeft);
    session.update(.16, false);
    check(session.audioEvents().count == 0, "Silent turn");
    session.setAnimated(false);
    session.move(Action::Back);
    check(session.audioEvents().count == 1, "Instant movement audible once");
    GameSession silent(map, catalog, empty, 27, {{5, 8}, Heading::North}, false);
    GameSession audible(map, catalog, empty, 27, {{5, 8}, Heading::North}, false);
    audible.audioEvents().enabled = true;
    for (auto* game : {&silent, &audible}) {
        game->move(Action::Forward);
        game->update(.2, false);
    }
    FloorState silentState, audibleState;
    silent.exportFloor(silentState);
    audible.exportFloor(audibleState);
    check(silentState.digest() == audibleState.digest(),
          "Audio observation preserves canonical state and RNG");
    NpcWorld walkers(map, {5, 8}, {"goblin"}, {{{0}, {5, 2}, {0, 0}, Heading::South}});
    GameSession walking(map, catalog, walkers, 2, {{5, 8}, Heading::North}, false);
    walking.audioEvents().enabled = true;
    auto& actors = const_cast<ActorWorld&>(walking.actors());
    check(bool(actors.walk(actors.id(0), {4, 2}, {0, 0}, {5, 8}, 0)), "Owned movement starts");
    walking.update(.59, false);
    check(walking.audioEvents().count == 0, "Enemy destination reservation silent");
    walking.update(.02, false);
    check(walking.audioEvents().count == 1 && walking.audioEvents().entries[0].creature,
          "Enemy arrival once");
    walking.audioEvents().clear();
    walking.update(.1, false);
    check(walking.audioEvents().count == 0, "Repeated settled snapshot silent");
    actors.walk(actors.id(0), {3, 2}, {0, 0}, {5, 8}, walking.timeMs());
    actors.damage(actors.id(0), 10000, walking.timeMs());
    walking.update(.7, false);
    check(walking.audioEvents().count == 0, "Dead walker has no arrival");
    for (unsigned i = 0; i < unsigned(rules::SpellKind::Count); ++i)
        check(audio::spellFamily(rules::SpellKind(i))[0], "Every spell maps");
    const auto tmp = std::filesystem::temp_directory_path() /
                     ("blobber-audio-policy-" + std::to_string(session.actors().snapshot().world));
    std::filesystem::create_directories(tmp);
    audio::Settings settings;
    settings.values[4] = 0;
    settings.borderlessFullscreen = true;
    std::string warning;
    check(audio::saveSettings(tmp / "settings.json", settings, warning), "Save settings");
    check(audio::loadSettings(tmp / "settings.json", warning).values[4] == 0, "Footstep off roundtrip");
    check(audio::loadSettings(tmp / "settings.json", warning).borderlessFullscreen,
          "Borderless preference survives a settings roundtrip");
    settings.borderlessFullscreen = false;
    check(audio::saveSettings(tmp / "settings.json", settings, warning) &&
              !audio::loadSettings(tmp / "settings.json", warning).borderlessFullscreen,
          "Returning to windowed mode persists");
    {
        std::ofstream f(tmp / "settings.json");
        f << R"({"version":2,"audio":[0.4,0.8,0.35,0.5,0.45],"music_enabled":false})";
    }
    const auto legacy = audio::loadSettings(tmp / "settings.json", warning);
    check(!legacy.borderlessFullscreen && legacy.values[0] == .4f && !legacy.musicEnabled,
          "Older settings keep audio preferences and default to windowed mode");
    {
        std::ofstream f(tmp / "settings.json");
        f << "invalid";
    }
    check(audio::loadSettings(tmp / "settings.json", warning).values[0] == .7f,
          "Malformed settings defaults");
    {
        std::ofstream f(tmp / "base.wav");
        f << "fixture";
    }
    {
        std::ofstream f(tmp / "bank.json");
        f << R"({"cues":{"test":{"variants":[{"id":"01","path":"base.wav"}]}}})";
    }
    {
        std::ofstream f(tmp / "overrides.json");
        f << R"({"cues":{"test":{"gain":0.2,"variant_overrides":{"01":{"path":"missing.wav"}}}}})";
    }
    const auto bank = audio::loadBank(tmp);
    check(bank.cues.size() == 1 && bank.cues[0].variants[0].path == "base.wav" && bank.cues[0].gain == .2f,
          "Per-variant override fallback");
    check(bank.categoryGain[1] == 1 && bank.categoryGain[4] == 1, "Old banks keep unity bus gain");
    {
        std::ofstream f(tmp / "bank.json");
        f << R"({"mix_gain_db":{"effects":12,"party_footsteps":6},"cues":{}})";
    }
    const auto boosted = audio::loadBank(tmp);
    check(std::abs(boosted.categoryGain[1] - 3.9810717f) < 1e-5f &&
              std::abs(boosted.categoryGain[4] - 1.9952623f) < 1e-5f && boosted.categoryGain[2] == 1,
          "Bus dB controls convert once; absent categories retain unity");
    for (const char* malformed :
         {R"({"mix_gain_db":2,"cues":{}})", R"({"mix_gain_db":{"effects":100},"cues":{}})"}) {
        {
            std::ofstream f(tmp / "bank.json");
            f << malformed;
        }
        check(!audio::loadBank(tmp).warning.empty(), "Malformed/excessive mix gain is diagnosed");
    }
    std::filesystem::remove_all(tmp);
    std::cout << "Audio policy and events passed\n";
}
