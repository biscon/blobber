#pragma once
#include "rules/creation.hpp"
namespace blobber {
class Party;
// Diagnostic-only legal selection of one spell; no runtime access override.
rules::PresetParty spellCaptureParty(const rules::Catalog&, rules::SpellKind);
// Explicit capture preparation: earn and apply normal Party advancement before the script.
void prepareSpellCaptureParty(Party&, rules::SpellKind);
} // namespace blobber
