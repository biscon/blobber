#include "game/combat.hpp"
#include <algorithm>

namespace blobber {
EffectResult EffectController::reserveBless(const EffectRequest& request) noexcept {
    if (!request.caster.npc || request.kind != EffectKind::Bless)
        return {EffectError::InvalidKind};
    if (const auto error = validate(request); error != EffectError::None)
        return {error};
    // A reserve never replaces concentration. Cast policy only starts without it.
    if (concentrationSource(request.caster).serial || gameplayMs_ > UINT64_MAX - 60600)
        return {EffectError::Capacity};
    for (const auto& e : instances_)
        if (e.pendingBless && e.caster == request.caster)
            return {EffectError::Capacity};
    for (auto& e : instances_)
        if (!e.active && !e.pendingBless) {
            e = {};
            e.id = {request.world, nextSerial_++};
            e.caster = request.caster;
            e.targets = request.targets;
            e.count = request.count;
            e.difficulty = request.difficulty;
            e.pendingBless = true;
            return {EffectError::None, e.id};
        }
    return {EffectError::Capacity};
}
void EffectController::cancelBless(EffectId id) noexcept {
    if (id.world != actors_.snapshot().world || !id.serial)
        return;
    for (auto& e : instances_)
        if (e.pendingBless && e.id.serial == id.serial) {
            e = {};
            return;
        }
}
bool EffectController::releaseBless(EffectId id, std::uint64_t now, CombatEvents& events) noexcept {
    if (id.world != actors_.snapshot().world || !id.serial)
        return false;
    for (auto& e : instances_) {
        if (!e.pendingBless || e.id.serial != id.serial)
            continue;
        if (!canAct(e.caster) || concentrationSource(e.caster).serial || gameplayMs_ > UINT64_MAX - 60000) {
            e = {};
            return false;
        }
        std::size_t count = 0;
        for (std::size_t i = 0; i < e.count; ++i)
            if (living(e.targets[i]))
                e.targets[count++] = e.targets[i];
        e.count = count;
        if (!count) {
            e = {};
            return false;
        }
        e.pendingBless = false;
        e.active = true;
        e.expiresMs = gameplayMs_ + 60000;
        for (std::size_t i = 0; i < count; ++i)
            emit(e, EffectNotice::Applied, EffectEnd::None, e.targets[i], {}, 0, now, events);
        return true;
    }
    return false;
}
} // namespace blobber
