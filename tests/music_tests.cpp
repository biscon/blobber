#include "audio/bank.hpp"
#include "audio/events.hpp"
#include "audio/music.hpp"
#include "audio/music_banner.hpp"
#include "game/combat.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/temp_directory.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <set>
using namespace blobber;
int main() {
    audio::Playlist p;
    p.reset(21, 42);
    unsigned last = audio::musicCapacity;
    for (unsigned cycle = 0; cycle < 4; ++cycle) {
        std::set<unsigned> seen;
        for (unsigned n = 0; n < 21; ++n) {
            const auto next = p.next();
            check(next != last, "No immediate repeat across wrap");
            seen.insert(next);
            last = next;
        }
        check(seen.size() == 21, "Every song consumed once per shuffle");
    }
    p.reset(0, 0);
    check(p.next() == 64, "Empty playlist safe");
    p.reset(1, 1);
    check(p.next() == 0 && p.next() == 0, "Single song can repeat");
    audio::MusicTransport t;
    t.playlist.reset(5, 21);
    const auto order = t.playlist.order;
    audio::MusicInput in;
    in.encounter = 1;
    in.active = true;
    in.remaining = 200;
    t.update(in);
    check(t.track == order[0] && t.playlist.cursor == 1, "Encounter starts first track at zero");
    const auto rev = t.revision;
    t.update(in);
    check(t.revision == rev, "Repeated snapshots never restart");
    in.ended = true;
    t.update(in);
    t.update(in);
    in.ended = false;
    check(t.track == order[2], "Long combat reaches third track");
    in.active = false;
    in.dt = 2.5f;
    t.update(in);
    check(t.fading() && std::abs(t.level - .5f) < 1e-6, "Five second linear fade");
    t.update(in);
    check(t.track == 64 && t.playlist.cursor == 3, "Fade never consumes next track");
    in.active = true;
    ++in.encounter;
    in.dt = 0;
    t.update(in);
    check(t.track == order[3], "Next encounter starts fourth song");
    in.active = false;
    in.remaining = 1;
    in.dt = .5f;
    t.update(in);
    check(std::abs(t.level - .5f) < 1e-6, "Short remaining song gets shortened fade");
    in.active = true;
    ++in.encounter;
    t.update(in);
    check(t.track == order[4] && t.level == 1, "New encounter interrupts fade with next song");
    in.active = false;
    in.ended = true;
    t.update(in);
    check(t.track == 64 && t.playlist.cursor == 5, "Combat end beats simultaneous EOF");
    in.active = true;
    in.ended = false;
    ++in.encounter;
    in.audible = false;
    t.update(in);
    check(t.track == 64 && t.playlist.cursor == 5, "Muted encounter consumes nothing");
    in.audible = true;
    t.update(in);
    const auto current = t.track;
    const auto generation = t.revision;
    in.audible = false;
    t.update(in);
    check(t.track == current && t.suspended, "Mute freezes current track");
    in.audible = true;
    t.update(in);
    check(t.track == current && t.revision == generation, "Unmute resumes without new announcement");
    in.paused = true;
    const auto age = t.bannerAge;
    in.dt = 30;
    t.update(in);
    check(t.bannerAge == age && t.suspended, "Explicit pause freezes announcement");
    in.paused = false;
    in.bannerVisible = false;
    t.update(in);
    check(t.bannerAge == 6 && !t.suspended, "Tactical overlay discards banner without pausing music");
    in.bannerVisible = true;
    in.dt = .01f;
    t.update(in);
    check(t.bannerAge == 6 && t.revision == generation,
          "Closing overlay never resumes or restarts the discarded announcement");
    in.bannerVisible = false;
    in.ended = true;
    t.update(in);
    const auto hiddenRevision = t.revision;
    check(hiddenRevision == generation + 1 && t.bannerAge == 6,
          "Track starting behind an overlay has no pending announcement");
    in.ended = false;
    in.bannerVisible = true;
    t.update(in);
    check(t.bannerAge == 6 && t.revision == hiddenRevision,
          "Quick overlay exit does not announce a song that already started");
    in.ended = true;
    t.update(in);
    check(t.bannerAge == 0 && t.revision == hiddenRevision + 1,
          "A later song starting visibly still gets its announcement");
    in.ended = false;
    in.paused = true;
    in.bannerVisible = false;
    t.update(in);
    check(t.bannerAge == 6 && t.suspended, "A hidden paused screen also discards the announcement");
    in.paused = false;
    in.audible = false;
    in.active = false;
    t.update(in);
    check(t.track == 64, "Combat end while muted discards current song");

    audio::MusicTransport menu;
    menu.policy = audio::MusicPolicy::Once;
    menu.playlist.reset(1, 12);
    audio::MusicInput mi;
    mi.active = true;
    mi.audible = false;
    menu.update(mi);
    check(menu.playlist.cursor == 0, "Muted menu entry does not consume its song");
    mi.audible = true;
    menu.update(mi);
    const auto menuRevision = menu.revision;
    mi.paused = true;
    mi.dt = 10;
    menu.update(mi);
    check(menu.suspended && menu.revision == menuRevision, "Menu pause preserves song");
    mi.paused = false;
    mi.ended = true;
    menu.update(mi);
    mi.ended = false;
    for (unsigned i = 0; i < 10; ++i)
        menu.update(mi);
    mi.audible = false;
    menu.update(mi);
    mi.audible = true;
    menu.update(mi);
    check(menu.track == audio::musicCapacity && menu.revision == menuRevision && menu.playlist.cursor == 1,
          "Single-play completion stays silent across frames and mute toggles");
    menu.stop();
    menu.update(mi);
    check(menu.track == 0 && menu.revision == menuRevision + 1, "New menu session restarts the one song");
    audio::Journal journal;
    journal.enabled = false;
    CombatOutcome event;
    event.event = CombatEvent::Started;
    journal.combat(event);
    journal.combat(event);
    check(journal.encounter == 1 && journal.encounterActive,
          "Lifecycle independent of disabled cues, duplicate start idempotent");
    journal.clear();
    check(journal.encounterActive, "Draining effects preserves lifecycle");
    event.event = CombatEvent::Disengaged;
    journal.combat(event);
    check(journal.encounterActive, "Temporary disengagement keeps song");
    event.event = CombatEvent::Finished;
    journal.combat(event);
    check(!journal.encounterActive, "End lifecycle independent of cue emission");
    event.event = CombatEvent::Started;
    journal.combat(event);
    check(journal.encounter == 2, "New episode gets new identity");

    const auto root = testDirectory();
    struct Cleanup {
        std::filesystem::path p;
        ~Cleanup() { std::filesystem::remove_all(p); }
    } cleanup{root};
    std::vector<audio::MusicTrack> tracks{
        {"a", "a.wav", "A", "Artist"}, {"b", "b.wav", "B", "Artist"}, {"c", "c.wav", "C", "Artist"}};
    p.reset(3, 51);
    p.next();
    p.next();
    std::string warning;
    check(audio::savePlaylist(root / "state.json", tracks, p, warning), "Playlist saved");
    audio::Playlist restored;
    restored.reset(3, 99);
    audio::loadPlaylist(root / "state.json", tracks, restored, warning);
    check(warning.empty() &&
              std::equal(restored.order.begin(), restored.order.begin() + p.count, p.order.begin()) &&
              restored.cursor == 2 && restored.random == p.random && restored.last == p.last,
          "Exact roundtrip");
    const auto next = p.next();
    check(restored.next() == next, "Restart continues next song, not partial current song");
    tracks.erase(tracks.begin() + p.order[0]);
    tracks.push_back({"new", "new.wav", "New", "Artist"});
    restored.reset(3, 72);
    audio::loadPlaylist(root / "state.json", tracks, restored, warning);
    check(restored.count == 3 && restored.cursor == 1, "Removed played ID reconciles cursor");
    check(tracks[restored.order[2]].id == "new", "New tracks join unplayed tail");
    std::ofstream(root / "state.json") << "broken";
    const auto before = restored.order;
    audio::loadPlaylist(root / "state.json", tracks, restored, warning);
    check(!warning.empty() && before == restored.order, "Corrupt state leaves fresh playlist intact");
    std::ofstream(root / "settings.json") << R"({"version":1,"audio":[0.6,0.8,0.3,0.4,0.2]})";
    warning.clear();
    auto settings = audio::loadSettings(root / "settings.json", warning);
    check(settings.values[0] == .6f && settings.musicEnabled && settings.musicVolume == .5f,
          "Old settings retain volumes and gain music defaults");
    settings.musicEnabled = false;
    settings.musicVolume = .23f;
    check(audio::saveSettings(root / "settings.json", settings, warning), "Music settings saved");
    settings = audio::loadSettings(root / "settings.json", warning);
    check(!settings.musicEnabled && settings.musicVolume == .23f, "Toggle preserves remembered volume");
    std::ofstream(root / "a.wav") << "owned fixture";
    std::ofstream(root / "playlist.json")
        << R"({"tracks":[{"id":"a","path":"a.wav","title":"A","artist":"Artist","gain_db":-6},{"id":"bad","path":"../escape.wav","title":"Bad","artist":"Artist"}]})";
    warning.clear();
    const auto loaded = audio::loadMusicTracks(root, warning);
    check(loaded.size() == 1 && std::abs(loaded[0].gain - .501187f) < 1e-5f && !warning.empty(),
          "Manifest validates owned tracks and gains, rejects escaping path");
    char letters[96];
    musicBannerLetters(letters, "Paid In Full", 1.2f, 7);
    check(std::strcmp(letters, "Paid In Full") == 0, "Reveal settles exactly to title");
    check(musicBannerMotion(0).alpha == 0 && musicBannerMotion(3).offset == 0 &&
              musicBannerMotion(5.8f).offset < 0 && musicBannerMotion(6).alpha == 0,
          "Banner entry hold retract and end");
    allocations = 0;
    watching = true;
    in = {};
    in.active = true;
    in.encounter = 999;
    in.dt = .01f;
    for (unsigned i = 0; i < 100; ++i) {
        in.ended = i % 10 == 0;
        t.update(in);
        musicBannerLetters(letters, "Fixture song", t.bannerAge, t.revision);
    }
    watching = false;
    check(allocations == 0, "Playlist transition and banner animation allocate no C++ memory");
    std::cout << "Music transport, persistence, settings, banner and allocation checks passed\n";
}
