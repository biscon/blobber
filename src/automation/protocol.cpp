#include "automation/protocol.hpp"
#include "game/campaign.hpp"
#include "generation/report.hpp"
#include "persistence/arena_store.hpp"
#include <stdexcept>
namespace blobber::automation {
Protocol::Protocol(const rules::Catalog& c, const NpcDefinitions& d, Mode m, VisibilityFactory f, void* x)
    : catalog_(c), definitions_(d), mode_(m), factory_(f), context_(x) {}
void Protocol::checkIdentity(const Json& r) const {
    if (!session_)
        throw std::invalid_argument("No session");
    if (serial(r.at("session")) != epoch_ || serial(r.at("revision")) != revision_)
        throw std::invalid_argument("Stale session or revision");
}
Json Protocol::observation(bool diagnostic) {
    session_->observe(observation_, diagnostic);
    auto j = encodeObservation(observation_);
    j["session"] = std::to_string(epoch_);
    j["revision"] = std::to_string(revision_);
    j["mode"] = mode_ == Mode::Visual ? "visual" : "diagnostic";
    j["diagnostic_inspection_used"] = inspected_;
    j["ticks"] = session_->ticks();
    j["mutations"] = mutations_;
    j["terminal"] = session_->terminal();
    j["decision"] = session_->decision();
    if (const auto* generated = session_->generated()) {
        j["scenario_kind"] = "generated_floor";
        j["generation"] = generation::report(*generated, observation_.diagnostic);
    }
    if (const auto* campaign = session_->game().campaign()) {
        j["scenario_kind"] = "campaign";
        const char* names[]{"active", "victory", "defeat"};
        j["campaign"] = {{"status", names[unsigned(campaign->status())]},
                         {"location", campaign->activeLocation()},
                         {"location_name", campaign->name()},
                         {"revision", std::to_string(campaign->revision())},
                         {"gameplay_ms", std::to_string(campaign->gameplayMs())},
                         {"visited_entries", Json::array()}};
        for (unsigned i = 1; i <= 3; ++i)
            if (campaign->location(i).visited)
                j["campaign"]["visited_entries"].push_back(i);
        j["campaign"]["travel"] = describeTravel(*campaign);
        if (observation_.diagnostic) {
            j["campaign"]["locations"] = Json::array();
            auto state = std::make_unique<FloorState>();
            for (unsigned i = 0; i < 4; ++i) {
                const auto& location = campaign->location(i);
                Json record = {{"location", i},
                               {"visited", location.visited},
                               {"fingerprint", location.generated
                                                   ? std::to_string(location.generated->fingerprint)
                                                   : "hub-v1"}};
                if (i == campaign->activeLocation()) {
                    session_->game().exportFloor(*state);
                    record["state_digest"] = std::to_string(state->digest());
                } else if (location.saved)
                    record["state_digest"] = std::to_string(location.saved->digest());
                j["campaign"]["locations"].push_back(record);
            }
        }
    }
    return j;
}
Json Protocol::state() {
    return session_ ? observation(true) : Json(nullptr);
}
void Protocol::emitEvents(const Json& id, const CombatEvents& events, Output& output) {
    for (std::size_t begin = 0; begin < events.count; begin += 256) {
        Json full = Json::array(), visible = Json::array();
        for (std::size_t i = begin; i < std::min(begin + 256, events.count); ++i) {
            full.push_back(encodeEvent(events.values[i], catalog_, true));
            auto e = encodeEvent(events.values[i], catalog_, mode_ == Mode::Diagnostic);
            if (!e.is_null())
                visible.push_back(std::move(e));
        }
        Json message = {{"version", 1}, {"type", "events"}, {"id", id}, {"events", full}};
        Json shown = nullptr;
        if (!visible.empty()) {
            shown = message;
            shown["events"] = std::move(visible);
        }
        output.emit(message, shown);
    }
}
Json Protocol::request(const Json& r, Output& output) {
    Json response = {{"version", 1}, {"type", "response"}, {"id", nullptr}, {"ok", false}};
    try {
        if (!r.is_object() || !r.contains("id") || !r.at("id").is_string() ||
            r.at("id").get_ref<const std::string&>().size() > 64)
            throw std::invalid_argument("Request id must be a string of at most 64 bytes");
        response["id"] = r.at("id");
        if (bounded(r.at("version"), 1) != 1)
            throw std::invalid_argument("Unsupported protocol version");
        if (closed_)
            throw std::invalid_argument("Protocol is closed");
        response["data"] = perform(r, output);
        response["ok"] = true;
    } catch (const Json::exception& e) {
        response["error"] = {{"code", "invalid_request"}, {"message", e.what()}};
    } catch (const std::invalid_argument& e) {
        response["error"] = {{"code", "invalid_request"}, {"message", e.what()}};
    }
    return response;
}
Json Protocol::perform(const Json& r, Output& output) {
    const auto op = r.at("op").get<std::string>();
    if (op == "close") {
        fields(r, {"version", "id", "op"});
        closed_ = true;
        return {{"closed", true}};
    }
    if (op == "start" || op == "reset") {
        fields(r, {"version", "id", "op", "session", "revision", "scenario", "seed"});
        if (session_)
            checkIdentity(r);
        else if (op == "reset")
            throw std::invalid_argument("No session");
        Scenario scenario;
        try {
            if (op == "start") {
                const auto serialized = r.at("scenario").dump();
                if (serialized.size() > arenaMaxBytes)
                    throw std::invalid_argument("Scenario exceeds 1 MiB");
                scenario = decodeScenario(catalog_, r.at("scenario"));
            } else {
                if (r.contains("scenario"))
                    throw std::invalid_argument("Reset does not replace scenario");
                scenario = session_->scenario();
            }
            if (r.contains("seed"))
                replaceSeed(scenario, bounded(r.at("seed"), UINT32_MAX));
            auto candidate =
                std::make_unique<Session>(catalog_, definitions_, scenario, mode_, factory_, context_);
            session_.swap(candidate);
        } catch (const std::runtime_error& e) {
            throw std::invalid_argument(e.what());
        }
        ++epoch_;
        revision_ = 1;
        mutations_ = 0;
        inspected_ = false;
        return observation(false);
    }
    if (!session_)
        throw std::invalid_argument("No session");
    if (op == "observe" || op == "inspect" || op == "actions") {
        fields(r, {"version", "id", "op"});
        if (op == "inspect")
            inspected_ = true;
        if (op == "actions") {
            session_->observe(observation_);
            auto j = describeActions(session_->game(), observation_);
            j["session"] = std::to_string(epoch_);
            j["revision"] = std::to_string(revision_);
            return j;
        }
        return observation(op == "inspect");
    }
    if (op == "preview" || op == "act") {
        fields(r, {"version", "id", "op", "session", "revision", "action"});
        checkIdentity(r);
        const auto action = decodeAction(r.at("action"), session_->game());
        if (op == "act" && mutations_ >= maxMutations)
            return {{"accepted", false}, {"reason", "mutation_limit"}};
        const auto* campaign = session_->game().campaign();
        const auto oldLocation = campaign ? campaign->activeLocation() : 0;
        const auto oldStatus = campaign ? campaign->status() : CampaignStatus::Active;
        const auto oldKits = session_->game().party().campSupplies().kits;
        std::array<bool, 4> oldVisited{};
        if (campaign)
            for (unsigned i = 0; i < 4; ++i)
                oldVisited[i] = campaign->location(i).visited;
        const auto result = session_->action(action, op == "preview");
        if (op == "act") {
            ++mutations_;
            ++revision_;
            if (campaign && (oldLocation != campaign->activeLocation() || oldStatus != campaign->status())) {
                Json event = {{"version", 1},
                              {"type", "events"},
                              {"id", r.at("id")},
                              {"events", Json::array({{{"event", "campaign_transition"},
                                                       {"from", oldLocation},
                                                       {"to", campaign->activeLocation()},
                                                       {"status", unsigned(campaign->status())}}})}};
                if (oldLocation != campaign->activeLocation()) {
                    auto& transition = event["events"][0];
                    transition["kind"] = travelKindName(
                        action.operation == Operation::Travel ? action.travel.kind : TravelKind::Stairs);
                    transition["kit_charge"] = oldKits - session_->game().party().campSupplies().kits;
                    if (!oldVisited[campaign->activeLocation()] && campaign->activeLocation() != 0)
                        event["events"].push_back(
                            {{"event", "entry_unlocked"}, {"depth", campaign->activeLocation()}});
                }
                output.emit(event, event);
            }
            if (result.accepted && action.operation != Operation::Continue)
                emitEvents(r.at("id"), session_->events(), output);
        }
        Json data = {{"accepted", result.accepted}, {"reason", result.reason}};
        if (op == "act")
            data["observation"] = observation(false);
        return data;
    }
    if (op == "advance") {
        fields(r, {"version", "id", "op", "session", "revision", "ticks", "until_decision"});
        checkIdentity(r);
        const auto ticks = bounded(r.at("ticks"), maxRequestTicks);
        const bool decision = r.value("until_decision", false);
        if (mutations_ >= maxMutations)
            return {{"ticks", 0}, {"stop", "mutation_limit"}, {"observation", observation(false)}};
        ++mutations_;
        ++revision_;
        const auto* campaign = session_->game().campaign();
        const auto oldStatus = campaign ? campaign->status() : CampaignStatus::Active;
        unsigned done = 0;
        while (done < ticks && session_->ticks() < maxRunTicks && !session_->result() &&
               !(decision && session_->decision())) {
            const auto& events = session_->tick();
            ++done;
            emitEvents(r.at("id"), events, output);
        }
        if (campaign && oldStatus != campaign->status()) {
            Json event = {{"version", 1},
                          {"type", "events"},
                          {"id", r.at("id")},
                          {"events", Json::array({{{"event", "campaign_result"},
                                                   {"status", unsigned(campaign->status())}}})}};
            output.emit(event, event);
        }
        const char* stop = session_->terminal()               ? "terminal"
                           : session_->result()               ? "encounter_result"
                           : session_->ticks() >= maxRunTicks ? "tick_limit"
                           : decision && session_->decision() ? "decision"
                                                              : "ticks";
        return {{"ticks", done}, {"stop", stop}, {"observation", observation(false)}};
    }
    throw std::invalid_argument("Unknown operation");
}
} // namespace blobber::automation
