#include "app/starting_build_diagnostics.hpp"
#include <stdexcept>
namespace blobber {
rules::PresetParty spellCaptureParty(const rules::Catalog& catalog, rules::SpellKind kind) {
    using namespace rules;
    auto party = *catalog.preset();
    const auto* r = spellRules(kind);
    if (!r)
        throw std::invalid_argument("Unknown diagnostic spell");
    auto& b = party.characters[r->casting == Casting::Wisdom ? 1 : 3];
    const auto bit = spellBit(catalog.findSpell(r->id));
    if (!bit)
        throw std::invalid_argument("Missing diagnostic spell");
    auto include = [&](std::uint32_t& mask, unsigned maximum) {
        mask |= bit;
        for (int i = 31; i >= 0 && spellCount(mask) > maximum; --i) {
            const auto remove = spellBit({std::uint16_t(i)});
            if (remove != bit)
                mask &= ~remove;
        }
    };
    if (!r->level) {
        if (!(b.spells.racial & bit))
            include(b.spells.cantrips, 3);
    } else if (!(b.spells.domain & bit)) {
        if (r->casting == Casting::Intelligence) {
            // Remove a non-prepared learned choice first so the rest of preparation remains legal.
            b.spells.learned |= bit;
            for (int i = 31; i >= 0 && spellCount(b.spells.learned) > 6; --i) {
                auto remove = spellBit({std::uint16_t(i)});
                if (remove != bit && !(b.spells.prepared & remove))
                    b.spells.learned &= ~remove;
            }
        }
        include(b.spells.prepared, unsigned(preparationLimit(catalog, b.view())));
    }
    CharacterDraft draft;
    if (!draftFromBuild(catalog, b, draft))
        throw std::runtime_error("Diagnostic spell selection is not a legal starting build");
    return party;
}
} // namespace blobber
