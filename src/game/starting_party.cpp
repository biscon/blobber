#include "game/starting_party.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace blobber {
rules::PresetParty startingParty(const rules::Catalog& c, const StartingParty& party) {
    rules::PresetParty p;
    for (unsigned i = 0; i < party.size(); ++i) {
        const auto& member = party[i];
        if (!member.id[0] || std::find(member.id.begin(), member.id.end(), '\0') == member.id.end() ||
            !rules::previewBuild(c, member.draft))
            throw std::runtime_error("Choose four valid level-one characters.");
        for (unsigned j = 0; j < i; ++j)
            if (!std::strcmp(member.id.data(), party[j].id.data()))
                throw std::runtime_error("Choose four distinct characters.");
        p.characters[i] = rules::finishBuild(c, member.draft, member.id.data());
        p.formation[i] = {std::uint16_t(i)};
    }
    return p;
}
} // namespace blobber
