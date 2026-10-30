#include "game/commands.hpp"
#include <algorithm>
#include <cstdio>
namespace blobber {
void CommandController::interactFixture(std::optional<FixtureId> id) noexcept {
    if (!session_ || state_.modal() || state_.suspended || state_.magic.casting() || state_.magic.armed ||
        state_.inventory.itemTargeting)
        return;
    auto request = session_->nearbyFixture();
    if (id)
        request = {*id, session_->fixtures().revision()};
    const auto result = session_->interact(request);
    state_.log.append(result.reason);
}

void CommandController::openLoot(LootId id) noexcept {
    if (!session_ || state_.modal() || state_.suspended)
        return;
    const auto* item = session_->loot().find(id);
    const auto access = session_->lootAccess(id);
    if (!item || !access) {
        state_.log.append(access.reason);
        return;
    }
    const auto cell = item->cell;
    closeOverlay();
    cancel();
    state_.inventory.itemTargeting = false;
    state_.hover = {};
    state_.lootHover[0] = 0;
    state_.loot = {};
    state_.loot.open = true;
    state_.loot.cell = cell;
    state_.loot.world = id.world;
    session_->clearPending();
    refreshLoot();
    refreshRestAvailability();
}
void CommandController::refreshLoot() noexcept {
    auto& h = state_.loot;
    if (!h.open || !session_)
        return;
    if (h.world != actors_.snapshot().world) {
        h.open = false;
        return;
    }
    h.count = 0;
    const auto& stacks = session_->loot().snapshot().stacks;
    for (std::uint16_t i = 0; i < stacks.size(); ++i)
        if (stacks[i].active && stacks[i].cell == h.cell)
            h.ids[h.count++] = {h.world, i, stacks[i].generation};
    h.page = std::min(h.page, h.count ? (h.count - 1) / LootHud::visibleCount : 0);
    h.entries = {};
    for (unsigned i = 0; i < LootHud::visibleCount && h.page * LootHud::visibleCount + i < h.count; ++i) {
        auto& e = h.entries[i];
        e.id = h.ids[h.page * LootHud::visibleCount + i];
        const auto* s = session_->loot().find(e.id);
        e.definition = party_.catalog().item(s->item);
        e.quantity = s->quantity;
    }
    h.recipient = state_.selection.character;
    const auto* c = party_.character(h.recipient);
    h.name = c ? c->build.name.c_str() : "No recipient";
    h.weight = c ? c->stats.carriedWeight : 0;
    h.capacity = c ? c->stats.carryingCapacity : 0;
    for (const auto& card : state_.cards)
        if (card.id.index == h.recipient.index)
            h.portrait = card.portrait;
    h.canTake = false;
    h.canTakeAny = false;
    h.reason = h.count ? "Hover an item for details." : "This bag is empty.";
    if (h.count) {
        const auto access = session_->lootAccess(h.ids[0]);
        h.canTakeAny = bool(access) && session_->effects().canAct(h.recipient);
        if (!access)
            h.reason = access.reason;
        else if (!h.canTakeAny)
            h.reason = "Select a conscious, able companion.";
    }
    if (h.hovered >= 0 && unsigned(h.hovered) < h.entries.size() && h.entries[h.hovered].definition) {
        const auto preview =
            session_->pickup({h.entries[h.hovered].id, h.recipient, party_.inventory().revision()}, true);
        h.canTake = bool(preview);
        h.reason = preview ? "Click to take this stack." : preview.reason;
    }
}
void CommandController::lootHover(int index) noexcept {
    if (!state_.loot.open)
        return;
    state_.loot.hovered = index >= 0 && unsigned(index) < LootHud::visibleCount ? index : -1;
    refreshLoot();
}
void CommandController::lootPage(int delta) noexcept {
    auto& h = state_.loot;
    if (!h.open)
        return;
    const int last = h.count ? int((h.count - 1) / LootHud::visibleCount) : 0;
    h.page = unsigned(std::clamp(int(h.page) + delta, 0, last));
    h.hovered = -1;
    refreshLoot();
}
void CommandController::lootRecipient(std::size_t formation) noexcept {
    if (!state_.loot.open || formation >= 4)
        return;
    state_.selection.character = party_.snapshot().formation[formation];
    refreshMagic();
    refreshLoot();
}
void CommandController::lootTake(unsigned index) noexcept {
    auto& h = state_.loot;
    if (!h.open || !session_ || state_.suspended || index >= h.entries.size() || !h.entries[index].definition)
        return;
    const auto e = h.entries[index];
    const auto out = session_->pickup({e.id, state_.selection.character, party_.inventory().revision()});
    record(session_->events());
    if (out)
        std::snprintf(h.feedback.data(), h.feedback.size(), "%s takes %u %s.", h.name, e.quantity,
                      e.definition->name.c_str());
    else
        std::snprintf(h.feedback.data(), h.feedback.size(), "%s", out.reason);
    state_.log.append(h.feedback.data());
    refreshLoot();
}
void CommandController::lootTakeAll() noexcept {
    auto& h = state_.loot;
    if (!h.open || !session_ || state_.suspended)
        return;
    refreshLoot();
    unsigned taken = 0, left = 0;
    const char* reason = "";
    // IDs and dialog cell remain stable while individual transactions commit.
    for (unsigned i = 0; i < h.count; ++i) {
        const auto out =
            session_->pickup({h.ids[i], state_.selection.character, party_.inventory().revision()});
        if (out)
            ++taken;
        else {
            ++left;
            if (!reason[0])
                reason = out.reason;
        }
    }
    std::snprintf(h.feedback.data(), h.feedback.size(), "Took %u stacks. %u remain.%s%s", taken, left,
                  left ? " " : "", reason);
    state_.log.append(h.feedback.data());
    h.hovered = -1;
    refreshLoot();
    closeOverlay();
    refreshRestAvailability();
}
} // namespace blobber
