#include "game/commands.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace blobber {
void CommandController::openCamp() noexcept {
    if (!session_ || state_.suspended)
        return;
    closeOverlay();
    cancel();
    state_.inventory.itemTargeting = false;
    state_.camp = {};
    state_.camp.open = true;
    state_.camp.request.world = actors_.snapshot().world;
    state_.camp.request.token = session_->restToken();
    session_->clearPending();
    refreshCamp();
}
void CommandController::refreshCamp() noexcept {
    if (!session_)
        return;
    auto& h = state_.camp;
    h.preview = session_->previewRest(h.request);
    h.catalog = &party_.catalog();
    auto probe = h.request;
    probe.kind = RestKind::Long;
    probe.dice = {};
    h.canPrepare = party_.alternativePreparation(probe) && bool(session_->previewRest(probe));
    h.supplier = h.preview.supplier.index < 4 ? party_.character(h.preview.supplier)->build.name.c_str() : "";
    for (std::uint16_t i = 0; i < 4; ++i) {
        const auto& c = *party_.character({i});
        h.members[i] = {c.build.name.c_str(),
                        c.hitPoints,
                        c.stats.maxHitPoints,
                        c.mana,
                        c.maxMana,
                        c.availableHitDice,
                        party_.catalog().characterClass(c.build.characterClass)->hitDie,
                        c.stats.modifiers[2],
                        c.life != rules::Life::Dead};
    }
}
void CommandController::campKind(RestKind kind) noexcept {
    if (!state_.camp.open || state_.camp.playing)
        return;
    auto& h = state_.camp;
    if (h.request.kind == RestKind::Long)
        h.staged = h.request.preparation;
    h.request.preparation = kind == RestKind::Long ? h.staged : PreparationRequest{};
    state_.camp.request.kind = kind;
    state_.camp.request.dice = {};
    refreshCamp();
}
void CommandController::campDice(unsigned c, int delta) noexcept {
    auto& h = state_.camp;
    if (!h.open || h.playing || c >= 4 || h.request.kind != RestKind::Short)
        return;
    const auto& m = h.members[c];
    const int limit = m.living && m.hp < m.maxHp ? m.available : 0;
    h.request.dice[c] = static_cast<std::uint8_t>(std::clamp(int(h.request.dice[c]) + delta, 0, limit));
    refreshCamp();
}
void CommandController::startRest() noexcept {
    auto& h = state_.camp;
    if (!session_ || !h.open || h.playing || h.preparing)
        return;
    refreshCamp();
    if (!h.preview)
        return;
    h.result = session_->rest(h.request);
    record(session_->events());
    if (!h.result) {
        refreshCamp();
        return;
    }
    h.playing = true;
    h.elapsedMs = 0;
    state_.log.append(h.request.kind == RestKind::Short ? "One hour passes. Short rest complete."
                                                        : "Eight hours pass. Long rest complete.");
    char line[192];
    for (unsigned i = 0; i < 4; ++i) {
        if (!h.result.healed[i] && !h.result.mana[i] && !h.result.diceSpent[i])
            continue;
        std::snprintf(line, sizeof(line), "%s: +%d HP, +%d mana, %d Hit Dice spent.", h.members[i].name,
                      h.result.healed[i], h.result.mana[i], h.result.diceSpent[i]);
        state_.log.append(line);
    }
    if (h.result.preparationChanged)
        state_.log.append("Prepared spells updated.");
    if (h.result.usedSupply)
        state_.log.append("One camp kit consumed.");
}
void CommandController::tickCamp(double seconds, bool focused) noexcept {
    auto& h = state_.camp;
    if (h.open && h.preparing)
        h.searchEdit.tick(float(seconds), focused);
    if (!h.open || !h.playing || !focused || !std::isfinite(seconds) || seconds < 0)
        return;
    h.elapsedMs += std::min(seconds, .1) * 1000;
    if (h.elapsedMs >= 2300)
        h.open = false;
}
void CommandController::floorHover(std::optional<LootId> id) noexcept {
    state_.lootHover[0] = 0;
    if (!id || !session_)
        return;
    const auto* s = session_->loot().find(*id);
    if (!s)
        return;
    const auto p = session_->lootAccess(*id);
    const auto* c = party_.character(state_.selection.character);
    unsigned count = 0;
    for (const auto& stack : session_->loot().snapshot().stacks)
        count += stack.active && stack.cell == s->cell;
    std::snprintf(state_.lootHover.data(), state_.lootHover.size(), "Loot bag | %u stacks | %s | %s", count,
                  c ? c->build.name.c_str() : "No recipient", p ? "Click to browse" : p.reason);
}
void CommandController::collectLoot(LootId id) noexcept {
    if (!session_ || state_.modal() || state_.suspended)
        return;
    const auto* s = session_->loot().find(id);
    if (!s)
        return;
    const auto item = s->item;
    const auto q = s->quantity;
    const auto result = session_->pickup({id, state_.selection.character, party_.inventory().revision()});
    record(session_->events());
    char line[192];
    if (result)
        std::snprintf(line, sizeof(line), "%s collects %u %s.",
                      party_.character(state_.selection.character)->build.name.c_str(), unsigned(q),
                      party_.catalog().item(item)->name.c_str());
    else
        std::snprintf(line, sizeof(line), "%s", result.reason);
    state_.log.append(line);
    floorHover({});
}
void CommandController::refreshRestAvailability() noexcept {
    state_.restAvailable = false;
    state_.restReason = "Rest unavailable.";
    if (!session_)
        return;
    RestRequest request{RestKind::Short, {}, actors_.snapshot().world, session_->restToken(), {}};
    for (std::uint16_t i = 0; i < 4; ++i) {
        const auto& c = *party_.character({i});
        if (c.life != rules::Life::Dead && c.hitPoints < c.stats.maxHitPoints && c.availableHitDice > 0)
            request.dice[i] = 1;
    }
    const auto shortRest = session_->previewRest(request);
    request.kind = RestKind::Long;
    request.dice = {};
    auto longRest = session_->previewRest(request);
    if (!longRest && party_.alternativePreparation(request))
        longRest = session_->previewRest(request);
    state_.restAvailable = !state_.suspended && !state_.modal() && (shortRest || longRest);
    state_.restReason =
        state_.restAvailable ? "Make camp to recover or prepare spells."
        : state_.modal()     ? "Close the current window first."
        : state_.suspended
            ? "Wait until movement ends and the game is active."
            : restMessage(shortRest.error == RestError::NoBenefit ? longRest.error : shortRest.error);
}
} // namespace blobber
