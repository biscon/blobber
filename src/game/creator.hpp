#pragma once
#include "input/text_edit.hpp"
#include "input/ui_input.hpp"
#include "rules/spell_selection.hpp"
namespace blobber {
enum class CreatorStep : unsigned {
    Identity,
    Class,
    Abilities,
    Skills,
    Cantrips,
    Racial,
    Learned,
    Prepared,
    Equipment,
    Review,
    Count
};
const char* creatorStepName(CreatorStep) noexcept;
struct CreatorModel {
    rules::CharacterDraft draft, baseline;
    rules::BuildPreview preview;
    CreatorStep step = CreatorStep::Identity;
    unsigned visited = 1;
    bool discard = false, canceled = false, saveRequested = false, saved = false;
    std::array<char, 192> message{};
    rules::SpellSelection selection;
    TextEdit nameEdit, searchEdit;
    unsigned reviewOffset = 0;
    void begin(const rules::Catalog&) noexcept;
    void refresh(const rules::Catalog&) noexcept;
    bool applicable(const rules::Catalog&, CreatorStep) const noexcept;
    bool validStep(const rules::Catalog&, CreatorStep) const noexcept;
    bool dirty() const noexcept;
    void go(const rules::Catalog&, CreatorStep) noexcept;
    void next(const rules::Catalog&, int direction) noexcept;
    void race(const rules::Catalog&, rules::RaceId) noexcept;
    void characterClass(const rules::Catalog&, rules::ClassId) noexcept;
    void ability(const rules::Catalog&, unsigned, int) noexcept;
    void recommend(const rules::Catalog&) noexcept;
    void toggleSpell(const rules::Catalog&, rules::SpellId) noexcept;
    void cancel() noexcept;
    void notify(const char*) noexcept;
    rules::SpellChoice choice() const noexcept;

  private:
    void prune(const rules::Catalog&) noexcept;
    void syncSelection(const rules::Catalog&) noexcept;
};
// Shared fixed controls: rendering and input use the same rectangles/action identities.
enum class FlowAction {
    None,
    Back,
    Next,
    Cancel,
    Keep,
    Discard,
    Save,
    Step,
    Name,
    Race,
    Portrait,
    Class,
    Minus,
    Plus,
    Recommend,
    Reset,
    Gear,
    Inspect,
    Toggle,
    Search,
    Scroll,
    Scrollbar,
    DetailsScrollbar,
    Member,
    Done
};
struct FlowControl {
    UiRect rect;
    FlowAction action = FlowAction::None;
    int value = 0;
    bool enabled = true;
};
struct FlowControls {
    std::array<FlowControl, 128> entries{};
    unsigned count = 0, focus = 0;
    bool overflow = false;
    void clear() noexcept {
        count = 0;
        overflow = false;
    }
    void add(UiRect, FlowAction, int = 0, bool = true) noexcept;
    FlowControl input(const UiInput&) noexcept;
};
void layoutSelection(FlowControls&, const rules::SpellSelection&) noexcept;
namespace selectionLayout {
inline constexpr UiRect listTrack{444, 109, 7, 153}, detailTrack{614, 230, 6, 27};
inline constexpr UiRect listPanel{174, 104, 277, 170}, detailPanel{458, 109, 164, 153};
} // namespace selectionLayout
UiRect selectionThumb(UiRect, unsigned count, unsigned page, unsigned offset) noexcept;
bool scrollSelection(rules::SpellSelection&, const rules::Catalog&, const UiInput&, FlowControl) noexcept;
void layoutCreator(FlowControls&, const CreatorModel&, const rules::Catalog&) noexcept;
void routeCreator(CreatorModel&, FlowControls&, const rules::Catalog&, const UiInput&, const TextInput&,
                  float) noexcept;
} // namespace blobber
