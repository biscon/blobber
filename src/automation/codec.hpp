#pragma once
#include "automation/session.hpp"
#include <nlohmann/json.hpp>
namespace blobber::automation {
using Json = nlohmann::json;
constexpr std::size_t maxRequestBytes = 2u * 1024u * 1024u;
std::uint64_t serial(const Json&);
unsigned bounded(const Json&, unsigned maximum);
void fields(const Json&, std::initializer_list<const char*>);
Json parseRequest(const std::string&);
// Trace envelopes add two structural levels and bounded metadata to an input request.
constexpr std::size_t maxTraceRecordBytes = maxRequestBytes + 4096;
Json parseTraceRecord(const std::string&);
ActionRequest decodeAction(const Json&, const GameSession&);
Json encodeObservation(const Observation&);
Json encodeEvent(const CombatOutcome&, const rules::Catalog&, bool diagnostic);
Json describeActions(const GameSession&, const Observation&);
const char* phaseName(CombatPhase) noexcept;
Json spellList(const rules::Catalog&, std::uint32_t);
} // namespace blobber::automation
