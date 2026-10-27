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
    if (!session_ || !h.open || h.playing)
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
    if (h.result.usedSupply)
        state_.log.append("One camp kit consumed.");
}
void CommandController::tickCamp(double seconds, bool focused) noexcept {
    auto& h = state_.camp;
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
    const auto p = session_->pickup({*id, state_.selection.character, party_.inventory().revision()}, true);
    const auto* c = party_.character(state_.selection.character);
    unsigned count = 0;
    for (const auto& stack : session_->loot().snapshot().stacks)
        count += stack.active && stack.cell == s->cell;
    const unsigned pages = (count + 5) / 6;
    const unsigned page = state_.lootPage.cell == s->cell ? state_.lootPage.page % pages : 0;
    std::snprintf(state_.lootHover.data(), state_.lootHover.size(),
                  "%s x%u | %s | %s | %u items, page %u/%u (wheel)",
                  party_.catalog().item(s->item)->name.c_str(), unsigned(s->quantity),
                  c ? c->build.name.c_str() : "No recipient", p ? "Click to collect" : p.reason, count,
                  page + 1, pages);
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
void CommandController::pageLoot(LootId id, int delta) noexcept {
    if (!session_)
        return;
    const auto* source = session_->loot().find(id);
    if (!source)
        return;
    const Cell cell = source->cell;
    unsigned count = 0;
    for (const auto& s : session_->loot().snapshot().stacks)
        count += s.active && s.cell == cell;
    const int pages = int((count + 5) / 6);
    if (!pages)
        return;
    auto& p = state_.lootPage;
    const int current = p.cell == cell ? int(std::min(p.page, unsigned(pages - 1))) : 0;
    p.cell = cell;
    p.page = (current + (delta > 0 ? 1 : pages - 1)) % pages;
    ++p.revision;
}
} // namespace blobber
