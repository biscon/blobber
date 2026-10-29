#pragma once
#include "rules/creation.hpp"
namespace blobber {
// Diagnostic-only legal selection of one spell; no runtime access override.
rules::PresetParty spellCaptureParty(const rules::Catalog&, rules::SpellKind);
} // namespace blobber
