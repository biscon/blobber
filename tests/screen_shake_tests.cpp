#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include "support/spell_parties.hpp"
#include "ui/interaction.hpp"
#include "ui/screen_shake.hpp"
#include <cmath>
#include <iostream>
#include <limits>
using namespace blobber;
namespace {
CombatOutcome impact() {
    CombatOutcome out;
    out.event = CombatEvent::Attack;
    out.hit = true;
    out.damage = 3;
    out.enemyAttack = true;
    out.occurredMs = 100;
    out.attackId = {1, 1};
    return out;
}
bool same(WorldViewTransform a, WorldViewTransform b) {
    return a.x == b.x && a.y == b.y && a.zoom == b.zoom;
}
void classification() {
    auto out = impact();
    check(shakeForImpact(out) == ShakeSize::Little, "Enemy melee damage is little");
    out.critical = true;
    check(shakeForImpact(out) == ShakeSize::Medium, "Enemy melee critical replaces little");
    for (bool incoming : {false, true})
        for (auto kind : {rules::AttackKind::Melee, rules::AttackKind::Ranged}) {
            out.enemyAttack = incoming;
            out.kind = kind;
            out.spell = rules::SpellKind::FireBolt;
            check(shakeForImpact(out) == ShakeSize::Medium, "Both sides and spell criticals are medium");
        }
    for (unsigned i = 0; i < 8; ++i) {
        out = impact();
        out.critical = true;
        switch (i) {
        case 0:
            out.hit = false;
            break;
        case 1:
            out.damage = 0;
            break;
        case 2:
            out.targetGone = true;
            break;
        case 3:
            out.mirror.effect.serial = 1;
            break;
        case 4:
            out.event = CombatEvent::AttackStarted;
            break;
        case 5:
            out.event = CombatEvent::ProjectileReleased;
            break;
        case 6:
            out.event = CombatEvent::AttackCanceled;
            break;
        case 7:
            out.reason = CommandReason::Unavailable;
            break;
        }
        check(!shakeForImpact(out), "Uncommitted, non-damaging and canceled criticals excluded");
    }
    out = impact();
    out.enemyAttack = false;
    check(!shakeForImpact(out), "Ordinary outgoing attack does not shake");
    out.enemyAttack = true;
    out.kind = rules::AttackKind::Ranged;
    check(!shakeForImpact(out), "Ordinary incoming ranged excluded");
    out.kind = rules::AttackKind::Melee;
    out.spell = rules::SpellKind::InflictWounds;
    check(!shakeForImpact(out), "Ordinary incoming spell excluded");
    for (unsigned i = 0; i < unsigned(rules::SpellKind::Count); ++i) {
        const auto kind = rules::SpellKind(i);
        out = {};
        out.event = CombatEvent::Cast;
        out.spell = kind;
        const bool area = kind == rules::SpellKind::BurningHands || kind == rules::SpellKind::Thunderwave ||
                          kind == rules::SpellKind::Shatter;
        check(bool(shakeForImpact(out)) == area, "Only the three approved area spells shake on cast");
        if (area)
            check(shakeForImpact(out) == ShakeSize::Big, "Area landing is big");
        out.event = CombatEvent::Attack;
        out.hit = true;
        out.damage = 10;
        check(!shakeForImpact(out), "Recipients never multiply area shake");
    }
}
void timingAndCapacity() {
    auto out = impact();
    ScreenShakePlayer player;
    check(player.record(out, 100, 1), "Record little impact");
    check(player.sample(100, 1).identity(), "Smooth attack begins at identity");
    const auto peak = player.sample(110, 1);
    check(peak.zoom > 1 && (peak.x != 0 || peak.y != 0), "Impact displaces and covers viewport");
    check(same(peak, player.sample(110, 1)), "Paused clock and repeated sampling are identical");
    player.sample(125, 1);
    check(same(peak, player.sample(110, 1)), "Sampling has no accumulated frame-step state");
    check(player.sample(99, 1).identity() && player.sample(240, 1).identity() &&
              player.sample(UINT64_MAX, 1).identity(),
          "Future, exact expiry and huge jumps are idle");
    check(!player.record(out, 240, 1) && !player.record(out, 99, 1), "Stale and future events suppressed");
    check(player.sample(110, 2).identity() && !player.record(out, 110, 2), "Old world excluded");
    out.attackId.world = 2;
    check(player.record(out, 110, 2), "New world accepts its own impacts");
    player.reset(2);
    check(player.sample(110, 2).identity(), "Explicit reset clears transient state");
    out = impact();
    out.event = CombatEvent::Cast;
    out.spell = rules::SpellKind::Shatter;
    player.reset(1);
    for (unsigned i = 0; i < ScreenShakePlayer::capacity; ++i)
        check(player.record(out, 100, 1), "Fixed capacity accepts its defined number of impulses");
    const auto before = player.sample(130, 1);
    check(!player.record(out, 100, 1) && same(before, player.sample(130, 1)),
          "Overflow drops new cosmetics without replacing existing impulses");
    for (unsigned t = 100; t <= 460; ++t) {
        const auto transform = player.sample(t, 1);
        check(std::abs(transform.x) <= 7 && std::abs(transform.y) <= 7, "Summed displacement bounded");
        const auto a = transform.forward({viewX, viewY});
        const auto b = transform.forward({viewX + viewWidth, viewY + viewHeight});
        check(a.x <= viewX + .0001f && a.y <= viewY + .0001f && b.x >= viewX + viewWidth - .0001f &&
                  b.y >= viewY + viewHeight - .0001f,
              "Animated crop covers every viewport edge");
    }
    out.occurredMs = 460;
    check(player.record(out, 460, 1) && player.activeCount(460) == 1, "Expired slots reused");
    for (unsigned i = 0; i < 5; ++i) {
        auto tuning = screenShakeTuning;
        if (i == 0)
            tuning.multiplier = 0;
        if (i == 1)
            tuning.frequencyHz = 0;
        if (i == 2)
            tuning.limitPixels = -1;
        if (i == 3)
            tuning.multiplier = std::numeric_limits<float>::quiet_NaN();
        if (i == 4)
            tuning.presets[2].durationMs = 0;
        ScreenShakePlayer disabled(tuning);
        check(!disabled.record(out, 460, 1) && disabled.sample(470, 1).identity(),
              "Disabled or invalid tuning is an idle no-op");
    }
    ScreenShakePlayer a, b;
    a.record(out, 460, 1);
    b.record(out, 460, 1);
    a.sample(465, 1);
    a.sample(480, 1);
    check(same(a.sample(500, 1), b.sample(500, 1)), "Frame subdivision does not affect sampling");
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 50; ++i) {
        out.occurredMs = 1000 + i * 10;
        a.record(out, out.occurredMs, 1);
        a.sample(out.occurredMs + 17, 1);
    }
    watching = false;
    check(allocations == 0, "Trigger, reclaim, saturation and sampling allocate nothing");
}
void mapping() {
    const WorldViewTransform t{4, -3, 1.05f};
    for (ArtPoint p : {ArtPoint{100, 80}, ArtPoint{250, 150}, ArtPoint{450, 260}}) {
        const auto q = t.inverse(t.forward(p));
        check(q && std::abs(q->x - p.x) < .0001f && std::abs(q->y - p.y) < .0001f,
              "World pointer maps back to its original image point");
    }
    check(!t.inverse({viewX - .01f, viewY}) && !t.inverse({viewX + viewWidth, viewY}) &&
              !t.inverse({viewX, viewY + viewHeight}) && !t.inverse({600, 30}),
          "Viewport boundaries exclude stationary HUD space");
    const auto clip = t.nativeClip({viewX, viewY, viewWidth, viewHeight});
    check(clip.x == viewX * 3 && clip.y == viewY * 3 && clip.width == viewWidth * 3 &&
              clip.height == viewHeight * 3,
          "Transformed clipping cannot extend beyond the viewport");
    const auto empty = t.nativeClip({0, 0, 1, 1});
    check(empty.width == 0 && empty.height == 0, "Offscreen depth fragments remain empty");
    const WorldViewTransform identity;
    check(identity.forward({25, 30}).x == 25 && identity.forward({25, 30}).y == 30 &&
              identity.inverse({25, 30})->x == 25,
          "Identity preserves original coordinates exactly");
}
void hoverFollowsImage() {
    const auto map = combatMap();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    const auto places = combatPlacements(map);
    GameSession game(map, catalog, places, 5, {{3, 3}, Heading::North}, false);
    spellReady(game, 0);
    CommandController hud(game);
    hud.refresh(allCombatTargets(game), false, false, true);
    const auto first = game.actors().id(2), second = game.actors().id(3);
    hud.selectAction(PartyAction::Melee);
    hud.selectTarget(first);
    UiInput input;
    input.pointer = ArtPoint{210, 140};
    routeHudInput(hud, input, first);
    check(hud.snapshot().hover.enemy && sameActor(*hud.snapshot().hover.enemy, first),
          "Stationary pointer initially hovers the visible creature");
    routeHudInput(hud, input, second, true, {}, {}, {}, {}, true);
    check(hud.snapshot().hover.enemy && sameActor(*hud.snapshot().hover.enemy, second),
          "World movement recomputes hover without pointer motion");
    // The final move back to identity also counts as image motion.
    routeHudInput(hud, input, {}, true, {}, {}, {}, {}, true);
    check(hud.snapshot().hover.kind == HoverKind::None && hud.snapshot().selection.target &&
              sameActor(*hud.snapshot().selection.target, first),
          "Settling clears stale hover while preserving the selected command target");
}
void realAreaEvents() {
    const auto catalog = creationCatalog(true);
    const auto map = hybridRoom();
    for (auto kind :
         {rules::SpellKind::BurningHands, rules::SpellKind::Thunderwave, rules::SpellKind::Shatter}) {
        for (int count : {0, 8}) {
            const auto places = spellPlacements(map, count, true);
            auto party = std::make_unique<Party>(catalog, leveledSpellParty(catalog, kind));
            prepareSpellParty(*party, kind);
            GameSession game(map, std::move(party), places, 11, spellSpawn, false, {}, {}, {}, {});
            if (count)
                spellReady(game, 3);
            CommandController hud(game);
            auto command = spellCommand(game, kind);
            if (kind == rules::SpellKind::Shatter)
                command.aimedCell = count ? Cell{6, 7} : Cell{6, 4};
            const auto outcome = game.submit(command);
            if (!count) {
                check(outcome.reason == CommandReason::MissingTarget,
                      (std::string(rules::spellRules(kind)->id) + " empty: " + commandMessage(outcome.reason))
                          .c_str());
                hud.record(game.events());
                check(hud.snapshot().worldTransform.identity(), "Rejected empty cast cannot shake");
                continue;
            }
            check(outcome.reason == CommandReason::Resolved,
                  (std::string(rules::spellRules(kind)->id) + " count=" + std::to_string(count) + ": " +
                   commandMessage(outcome.reason))
                      .c_str());
            ScreenShakePlayer player;
            unsigned casts = 0, requests = 0;
            for (const auto& event : game.events()) {
                casts += event.event == CombatEvent::Cast;
                requests += player.record(event, game.timeMs(), game.actors().snapshot().world);
            }
            check(casts == 1 && requests == 1, "Multi-target areas emit exactly one big shake");
            hud.record(game.events());
            const auto time = game.timeMs();
            hud.record(game.update(.025, false));
            hud.refresh(allCombatTargets(game), false, false, true);
            check(same(hud.snapshot().worldTransform,
                       player.sample(game.timeMs(), game.actors().snapshot().world)),
                  "Command feedback publishes the cast's shake without per-target multiplication");
            const auto paused = hud.snapshot().worldTransform;
            hud.record(game.update(.5, true));
            hud.refresh(allCombatTargets(game), false, false, true);
            check(game.timeMs() == time + 25 && same(paused, hud.snapshot().worldTransform),
                  "Session pause freezes shake through production feedback path");
            allocations = 0;
            watching = true;
            hud.refresh(allCombatTargets(game), false, false, true);
            watching = false;
            check(allocations == 0, "HUD shake refresh allocates nothing");
        }
    }
}
} // namespace
int main() {
    try {
        classification();
        timingAndCapacity();
        mapping();
        realAreaEvents();
        hoverFollowsImage();
        std::cout
            << "Screen shake classification, timing, mapping, integration and allocation checks passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
