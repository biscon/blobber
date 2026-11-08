#pragma once
#include "ui/dialogue_layout.hpp"
#include "ui/fonts.hpp"
namespace blobber {
inline void prepareDialogueUi(DialogueCatalog& catalog, const UiFonts& fonts) {
    prepareDialogueLayout(
        catalog,
        [](const char* text, void* context) {
            return static_cast<const UiFonts*>(context)->width(text, UiTextStyle::Dialogue);
        },
        const_cast<UiFonts*>(&fonts), dialogueLayout::body.width,
        unsigned(dialogueLayout::body.height / fonts.lineHeight(UiTextStyle::Dialogue)));
}
} // namespace blobber
