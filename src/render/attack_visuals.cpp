#include "render/attack_visuals.hpp"
namespace blobber {
bool ProjectilePlayer::update(const AttackVisualSnapshot* snapshot, std::uint64_t now) noexcept {
    const auto oldCount = count_;
    const bool changed = world_ != (snapshot ? snapshot->world : 0) ||
                         revision_ != (snapshot ? snapshot->revision : 0) || (count_ && now != now_);
    count_ = 0;
    now_ = now;
    world_ = snapshot ? snapshot->world : 0;
    revision_ = snapshot ? snapshot->revision : 0;
    if (snapshot && capacity_) {
        const auto add = [&](const AttackVisual& v) {
            if (v.stage == AttackStage::Empty || v.stage == AttackStage::Canceled ||
                now < (v.ranged ? v.releaseMs : v.impactMs) || now >= v.impactMs + 120)
                return;
            if (!v.ranged && v.stage != AttackStage::Resolved)
                return;
            if (count_ < capacity_)
                shots_[count_++] = v;
            else {
                auto oldest = shots_.begin();
                for (auto i = shots_.begin() + 1; i != shots_.begin() + count_; ++i)
                    if (i->id.serial < oldest->id.serial)
                        oldest = i;
                if (v.id.serial > oldest->id.serial)
                    *oldest = v;
            }
        };
        for (const auto& v : snapshot->attacks)
            add(v);
        for (const auto& v : snapshot->spellExtras)
            add(v);
        for (const auto& v : snapshot->spellPulses)
            add(v);
    }
    return changed || oldCount != count_;
}
} // namespace blobber
