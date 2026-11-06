#include "game/campaign.hpp"
#include "game/commands.hpp"
#include "ui/dialogue_layout.hpp"
#include <cmath>
#include <cstdio>
namespace blobber {
namespace {
bool frontal(const GameSession& session, ActorId id) noexcept {
    const auto* a = session.actors().actor(id);
    if (!a || a->hostile() || a->life != ActorLife::Alive || !a->npc.residentId)
        return false;
    const auto p = session.movement().player;
    const float dx = a->npc.cell.x + .25f + a->npc.subcell.x * .5f - (p.cell.x + .5f);
    const float dz = a->npc.cell.y + .25f + a->npc.subcell.y * .5f - (p.cell.y + .5f);
    const float distance2 = dx * dx + dz * dz;
    const auto pf = forward(p.heading), nf = forward(a->npc.facing);
    const float view = dx * pf.x + dz * pf.y, front = -dx * nf.x - dz * nf.y;
    return distance2 > .01f && distance2 <= 4 && view > 0 && front > 0 && view * view >= distance2 * .5f &&
           front * front >= distance2 * .5f;
}
} // namespace
bool GameSession::beginConversation(ActorId id) noexcept {
    if (conversation_ || actionBusy() || combat_.snapshot().phase != CombatPhase::Exploration ||
        saveSafety() || !frontal(*this, id))
        return false;
    clearPending();
    conversation_ = id;
    return true;
}
void GameSession::endConversation() noexcept {
    if (conversation_)
        actors_.talk(*conversation_, false);
    conversation_.reset();
    clearPending();
}
CommandController::~CommandController() {
    finishDialogue();
}
void CommandController::bindDialogue(const DialogueCatalog* catalog, DialogueHistory* history) noexcept {
    finishDialogue();
    dialogueCatalog_ = catalog;
    dialogue_.bind(
        catalog, history,
        [](const DialogueCharacter& c, const DialogueResponse& r, void* context) noexcept {
            return static_cast<CommandController*>(context)->admitDialogueResponse(c, r);
        },
        this);
}
void CommandController::residentVisibility(const TargetSnapshot& snapshot) noexcept {
    residents_ = snapshot;
}
std::optional<ActorId> CommandController::nearbyResident() const noexcept {
    if (!dialogueCatalog_ || residents_.world != actors_.snapshot().world ||
        residents_.revision != actors_.snapshot().revision || residents_.count > residents_.ids.size())
        return {};
    std::optional<ActorId> result;
    float best = 5;
    for (unsigned i = 0; i < residents_.count; ++i) {
        const auto id = residents_.ids[i];
        const auto* a = actors_.actor(id);
        if (!frontal(session_, id) || !a || !dialogueCatalog_->find(a->npc.residentId))
            continue;
        const auto p = session_.movement().player.cell;
        const float x = a->npc.cell.x + .25f + a->npc.subcell.x * .5f - (p.x + .5f);
        const float z = a->npc.cell.y + .25f + a->npc.subcell.y * .5f - (p.y + .5f);
        if (x * x + z * z < best) {
            best = x * x + z * z;
            result = id;
        }
    }
    return result;
}
bool CommandController::startDialogue(std::optional<ActorId> requested) noexcept {
    if (state_.suspended || state_.modal() || state_.campaignTerminal || state_.magic.casting() ||
        state_.magic.armed || state_.inventory.itemTargeting)
        return false;
    const auto nearest = nearbyResident();
    const auto id = requested ? requested : nearest;
    if (!id || !frontal(session_, *id))
        return false;
    bool visible = false;
    if (residents_.world == actors_.snapshot().world && residents_.revision == actors_.snapshot().revision)
        for (unsigned i = 0; i < residents_.count && i < residents_.ids.size(); ++i)
            visible |= sameActor(*id, residents_.ids[i]);
    const auto* a = actors_.actor(*id);
    if (!visible || !a || !dialogueCatalog_ || !dialogueCatalog_->find(a->npc.residentId) ||
        !session_.beginConversation(*id))
        return false;
    if (!dialogue_.start(a->npc.residentId)) {
        state_.log.append("Can't talk right now.");
        session_.endConversation();
        return false;
    }
    cancel();
    state_.hover = {};
    state_.targetPicker.open = false;
    cancelPointerButtons();
    speaker_ = id;
    dialogue_.snapshot(state_.dialogue);
    syncDialogueSpeech();
    return true;
}
namespace {
std::optional<ActorId> findParticipant(const ActorWorld& world, DialogueKey id) noexcept {
    for (unsigned i = 0; i < world.snapshot().count; ++i) {
        const auto& a = world.snapshot().actors[i];
        if (a.npc.residentId == id && a.npc.role == NpcRole::Resident && a.life == ActorLife::Alive)
            return world.id(i);
    }
    return {};
}
} // namespace
std::optional<ActorId> CommandController::dialogueActor(unsigned index) const noexcept {
    const auto* c = dialogue_.character();
    if (!c || index >= c->participants.size())
        return {};
    return findParticipant(actors_, c->participants[index].id);
}
bool CommandController::admitDialogueResponse(const DialogueCharacter& c,
                                              const DialogueResponse& r) const noexcept {
    for (const auto& beat : r.beats) {
        if (beat.speaker >= c.participants.size() || beat.target >= c.participants.size())
            return false;
        const auto id = findParticipant(actors_, c.participants[beat.speaker].id);
        if (!id || !findParticipant(actors_, c.participants[beat.target].id))
            return false;
        const auto* actor = actors_.actor(*id);
        if (beat.animation && !dialogueTimings_[actor->npc.type.index][unsigned(*beat.animation)].count)
            return false;
    }
    return true;
}
void CommandController::syncDialogueSpeech() noexcept {
    dialogueAnimation_ = {};
    const auto* c = dialogue_.character();
    if (!c) {
        speaker_.reset();
        return;
    }
    const auto* beat = dialogue_.beat();
    speaker_ = dialogueActor(beat ? beat->speaker : c->primary);
    if (!beat || !dialogue_.cuesReady())
        return;
    dialogueAnimation_.actor = speaker_;
    dialogueAnimation_.clip = beat->animation.value_or(NpcAnimation::Talk);
    if (!dialogue_.revealing() &&
        (dialogueAnimation_.clip == NpcAnimation::Talk || dialogueAnimation_.clip == NpcAnimation::Laugh))
        dialogueAnimation_.clip = NpcAnimation::Idle;
    dialogueAnimation_.elapsedMs = dialogue_.beatElapsedMs();
}
void CommandController::tickDialogue(double seconds, bool focused) noexcept {
    if (!dialogue_.active())
        return;
    const auto* c = dialogue_.character();
    if (!c || !findParticipant(actors_, c->id) ||
        (dialogue_.currentResponse() && !admitDialogueResponse(*c, *dialogue_.currentResponse()))) {
        finishDialogue();
        return;
    }
    dialogue_.tick(seconds, focused);
    if (!dialogue_.active()) {
        finishDialogue();
        return;
    }
    syncDialogueSpeech();
    dialogue_.snapshot(state_.dialogue);
}
void CommandController::finishDialogue() noexcept {
    // An inactive controller may outlive its old session after a travel handoff.
    // Do not inspect the borrowed session unless this controller owns dialogue.
    if (speaker_ || dialogue_.active())
        session_.endConversation();
    speaker_.reset();
    dialogueAnimation_ = {};
    dialogue_.close();
    state_.dialogue = {};
    cancelPointerButtons();
}
void CommandController::routeDialogue(const UiInput& input) noexcept {
    if (!dialogue_.active() || !input.focused || dialogue_.leaving())
        return;
    const auto inside = [&](UiRect r) { return input.pointer && r.contains(*input.pointer); };
    const bool click = input.has(UiKey::Click);
    if (input.has(UiKey::Cancel))
        dialogue_.leave();
    else if (click && inside(dialogueLayout::skip) && state_.dialogue.introduction)
        dialogue_.skipIntroduction();
    else {
        bool chosen = false;
        constexpr UiKey numbers[]{UiKey::One,  UiKey::Two, UiKey::Three, UiKey::Four,
                                  UiKey::Five, UiKey::Six, UiKey::Seven};
        for (unsigned i = 0; i < 7; ++i)
            if (input.has(numbers[i]) || (click && inside(dialogueLayout::row(i)))) {
                dialogue_.choose(i);
                chosen = true;
                break;
            }
        if (!chosen) {
            if (input.has(UiKey::Up))
                dialogue_.select(-1);
            else if (input.has(UiKey::Down))
                dialogue_.select(1);
            else if (input.has(UiKey::Confirm))
                dialogue_.choose(dialogue_.cursor());
            else if (input.has(UiKey::DialogueAdvance) || (click && inside(dialogueLayout::response)))
                dialogue_.advance();
            else if (input.has(UiKey::PreviousPage) || (click && inside(dialogueLayout::previous)))
                dialogue_.scroll(-7);
            else if (input.has(UiKey::NextPage) || (click && inside(dialogueLayout::next)))
                dialogue_.scroll(7);
            else if (input.scroll)
                dialogue_.scroll(-input.scroll);
        }
    }
    syncDialogueSpeech();
    dialogue_.snapshot(state_.dialogue);
}
} // namespace blobber
