#pragma once
#include "automation/codec.hpp"
#include "persistence/campaign_store.hpp"
namespace blobber::automation {
struct Output {
    virtual ~Output() = default;
    // Full events are for traces; public output may be null when information is hidden.
    virtual void emit(const Json& full, const Json& publicMessage) = 0;
};
class Protocol {
  public:
    Protocol(const rules::Catalog&, const NpcDefinitions&, Mode, VisibilityFactory = nullptr, void* = nullptr,
             CampaignStore* = nullptr);
    Json request(const Json&, Output&);
    Json state(); // Diagnostic normalized boundary state, not a player observation.
    bool closed() const noexcept { return closed_; }
    Session* replaySession() noexcept { return session_.get(); } // Internal diagnostic rendering only.
    const Session* session() const noexcept { return session_.get(); }

  private:
    Json perform(const Json&, Output&);
    void checkIdentity(const Json&) const;
    void emitEvents(const Json& id, const CombatEvents&, Output&);
    Json observation(bool diagnostic);
    const rules::Catalog& catalog_;
    const NpcDefinitions& definitions_;
    Mode mode_;
    VisibilityFactory factory_;
    void* context_;
    std::unique_ptr<Session> session_;
    Observation observation_;
    std::uint64_t epoch_ = 0, revision_ = 0;
    unsigned mutations_ = 0;
    bool inspected_ = false, closed_ = false, terminalRecorded_ = false;
    CampaignStore* saves_ = nullptr;
    std::string persistenceWarning_;
};
} // namespace blobber::automation
