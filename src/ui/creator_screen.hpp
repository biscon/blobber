#pragma once
#include "game/creator.hpp"
#include "ui/hud_painter.hpp"
namespace blobber {
struct CreatorView {
    const rules::Catalog& catalog;
    CreatorModel& model;
    FlowControls controls;
};
void drawCreator(void*, const UiFonts&, const UiArt&);
unsigned drawBuildReview(const HudPainter&, const rules::Catalog&, const rules::CharacterDraft&,
                         unsigned offset);
void drawAdvancement(const HudPainter&, const AdvancementHud&);
void drawPreparation(const HudPainter&, const CampHud&);
} // namespace blobber
