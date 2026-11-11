// Internal typed sparse archive. Included inside campaign_codec.cpp's namespace.
// A missing member inherits its typed baseline; containers carry indexed changes,
// so neither pool capacity nor default object fields turn into JSON placeholders.
template <class T> const T& campaignDefault() {
    if constexpr (std::is_same_v<T, rules::DiceRng>) {
        static const T value{0};
        return value;
    } else {
        static const T value{};
        return value;
    }
}
struct DeltaArchive {
    Json& object;
    const rules::Catalog& catalog;
    const NpcDefinitions& definitions;
    bool reading;
    std::size_t fields = 0;
    FloorState* scratch = nullptr; // One reusable preparation buffer for baseline comparisons.

    template <class T> void operator()(const char* name, T& value, const T& baseline) {
        if (reading) {
            if (!object.is_object())
                invalid("Expected campaign delta object");
            const auto it = object.find(name);
            if (it != object.end()) {
                ++fields;
                transfer(*it, value, baseline);
            }
        } else {
            Json change;
            if (transfer(change, value, baseline)) {
                if (object.is_null())
                    object = Json::object();
                object[name] = std::move(change);
            }
        }
    }
    template <class T> void counted(const char* name, T& value, std::size_t count, const T& base) {
        if ((!reading || !Vector<T>::value) && count > value.size())
            invalid("Campaign logical count exceeds capacity");
        if (reading) {
            const auto it = object.find(name);
            if (it != object.end()) {
                ++fields;
                transfer(*it, value, base, count);
            }
        } else {
            Json change;
            if (transfer(change, value, base, count)) {
                if (object.is_null())
                    object = Json::object();
                object[name] = std::move(change);
            }
        }
    }
    static void initializeActors(FloorState& state, const CampaignLocationState& baseline) {
        state.actors.count = baseline.initialActors.size();
        state.actors.typeCount = baseline.types.size();
        std::copy(baseline.initialActors.begin(), baseline.initialActors.end(), state.actors.actors.begin());
    }
    void floor(const char* name, std::unique_ptr<FloorState>& state, const CampaignLocationState& baseline) {
        if (reading) {
            const auto it = object.find(name);
            if (it == object.end())
                return;
            ++fields;
            if (!it->is_object() || it->size() != 1 || !it->contains("value"))
                invalid("Invalid campaign floor changes");
            state = std::make_unique<FloorState>();
            initializeActors(*state, baseline);
            transfer((*it)["value"], *state, campaignDefault<FloorState>());
        } else if (state) {
            if (!scratch)
                invalid("Missing campaign baseline workspace");
            *scratch = FloorState{};
            initializeActors(*scratch, baseline);
            Json change;
            transfer(change, *state, *scratch);
            if (object.is_null())
                object = Json::object();
            object[name] = {{"value", change.is_null() ? Json::object() : std::move(change)}};
        }
    }
    template <class T> void required(const char* name, T& value, const T& base) {
        if (reading) {
            if (!object.is_object() || !object.contains(name))
                invalid("Missing required campaign field");
            (*this)(name, value, base);
        } else {
            if (object.is_null())
                object = Json::object();
            Archive values{object[name], catalog, definitions, false};
            values.transfer(object[name], value);
        }
    }
    template <class T> static bool equalLeaf(const T& a, const T& b) {
        if constexpr (std::is_same_v<T, rules::ItemId> || std::is_same_v<T, rules::MonsterId> ||
                      std::is_same_v<T, rules::RaceId> || std::is_same_v<T, rules::ClassId> ||
                      std::is_same_v<T, rules::PortraitId> || std::is_same_v<T, NpcTypeId>)
            return a.index == b.index;
        else if constexpr (std::is_same_v<T, rules::SpellGrants>)
            return a.cantrips == b.cantrips && a.learned == b.learned && a.prepared == b.prepared &&
                   a.domain == b.domain && a.racial == b.racial;
        else
            return a == b;
    }
    template <class T>
    static constexpr bool leaf = std::is_arithmetic_v<T> || std::is_enum_v<T> ||
                                 std::is_same_v<T, std::string> || std::is_same_v<T, rules::ItemId> ||
                                 std::is_same_v<T, rules::MonsterId> || std::is_same_v<T, rules::RaceId> ||
                                 std::is_same_v<T, rules::ClassId> || std::is_same_v<T, rules::PortraitId> ||
                                 std::is_same_v<T, NpcTypeId> || std::is_same_v<T, rules::SpellGrants> ||
                                 std::is_same_v<T, blobber::Mt19937> || std::is_same_v<T, rules::DiceRng>;

    static std::size_t index(const Json& value, std::size_t limit) {
        if (!value.is_number_unsigned() || value.get<std::uint64_t>() >= limit)
            invalid("Campaign sparse index out of range");
        return value.get<std::size_t>();
    }
    template <class T> bool transfer(Json& j, T& v, const T& base, std::size_t limit = SIZE_MAX) {
        if constexpr (leaf<T>) {
            if (!reading && equalLeaf(v, base))
                return false;
            Archive values{j, catalog, definitions, reading};
            values.transfer(j, v);
            return true;
        } else if constexpr (Bits<T>::value) {
            if (reading) {
                if (!j.is_array() || j.size() > v.size())
                    invalid("Invalid campaign bit changes");
                std::size_t previous = 0;
                bool first = true;
                for (const auto& bit : j) {
                    const auto i = index(bit, v.size());
                    if (!first && i <= previous)
                        invalid("Duplicate or unordered campaign bit");
                    first = false;
                    previous = i;
                    v.flip(i);
                }
                return true;
            }
            for (std::size_t i = 0; i < v.size(); ++i)
                if (v[i] != base[i]) {
                    if (j.is_null())
                        j = Json::array();
                    j.push_back(i);
                }
            return !j.is_null();
        } else if constexpr (Array<T>::value || Vector<T>::value) {
            if (reading) {
                Json* entries = &j;
                if constexpr (Vector<T>::value) {
                    if (!j.is_object() || j.size() != 2 || !j.contains("size") || !j.contains("entries") ||
                        !j["size"].is_number_unsigned())
                        invalid("Invalid campaign vector changes");
                    const auto n = j["size"].get<std::uint64_t>();
                    std::size_t capacity = generation::maxCells;
                    if constexpr (std::is_same_v<typename T::value_type, MerchantState>)
                        capacity = merchantCapacity;
                    if constexpr (std::is_same_v<typename T::value_type, BuildingPlacement>)
                        capacity = buildingCapacity;
                    if constexpr (std::is_same_v<typename T::value_type, SceneryPlacement>)
                        capacity = sceneryCapacity;
                    if constexpr (std::is_same_v<typename T::value_type, AmbientFire>)
                        capacity = ambientFireCapacity;
                    if (n > capacity)
                        invalid("Campaign vector capacity exceeded");
                    entries = &j["entries"];
                    v.resize(std::size_t(n));
                }
                if (limit != SIZE_MAX && limit > v.size())
                    invalid("Campaign logical count exceeds capacity");
                const auto n = std::min(v.size(), limit);
                if (!entries->is_array() || entries->size() > n)
                    invalid("Invalid campaign sparse entries");
                std::size_t previous = 0;
                bool first = true;
                for (auto& entry : *entries) {
                    if (!entry.is_array() || entry.size() != 2)
                        invalid("Invalid campaign sparse record");
                    const auto i = index(entry[0], n);
                    if (!first && i <= previous)
                        invalid("Duplicate or unordered campaign sparse index");
                    first = false;
                    previous = i;
                    const auto& original =
                        i < base.size() ? base[i] : campaignDefault<typename T::value_type>();
                    transfer(entry[1], v[i], original);
                }
                return true;
            }
            Json entries;
            for (std::size_t i = 0; i < std::min(v.size(), limit); ++i) {
                Json change;
                const auto& original = i < base.size() ? base[i] : campaignDefault<typename T::value_type>();
                if (transfer(change, v[i], original)) {
                    if (entries.is_null())
                        entries = Json::array();
                    entries.push_back(Json::array({i, std::move(change)}));
                }
            }
            if constexpr (Vector<T>::value) {
                if (entries.is_null() && v.size() == base.size())
                    return false;
                j = {{"size", v.size()}, {"entries", entries.is_null() ? Json::array() : std::move(entries)}};
            } else {
                if (entries.is_null())
                    return false;
                j = std::move(entries);
            }
            return true;
        } else if constexpr (Optional<T>::value || Pointer<T>::value) {
            if (reading) {
                if (j.is_null()) {
                    v.reset();
                    return true;
                }
                if (!j.is_object() || j.size() != 1 || !j.contains("value"))
                    invalid("Invalid optional campaign change");
                if (!v) {
                    if constexpr (Optional<T>::value)
                        v.emplace();
                    else
                        v = std::make_unique<typename T::element_type>();
                }
                transfer(j["value"], *v,
                         base ? *base : campaignDefault<std::remove_reference_t<decltype(*v)>>());
                return true;
            }
            if (!v) {
                if (!base)
                    return false;
                j = nullptr;
                return true;
            }
            Json change;
            const bool changed =
                transfer(change, *v, base ? *base : campaignDefault<std::remove_reference_t<decltype(*v)>>());
            if (!changed && base)
                return false;
            // A present scalar equal to its default still needs its scalar encoding.
            if constexpr (leaf<std::remove_reference_t<decltype(*v)>>) {
                if (!changed) {
                    Archive values{change, catalog, definitions, false};
                    values.transfer(change, *v);
                }
            }
            j = {{"value", change.is_null() ? Json::object() : std::move(change)}};
            return true;
        } else if constexpr (std::is_same_v<T, CampaignLocationSave>) {
            if (reading || !v.definition || !base.definition)
                invalid("Invalid campaign capture definitions");
            DeltaArchive nested{j, catalog, definitions, false, 0, scratch};
            if (v.definition != base.definition)
                visit(nested, const_cast<CampaignLocationState&>(*v.definition), *base.definition);
            nested("visited", v.visited, base.visited);
            nested.floor("state", v.state, *base.definition);
            return !j.is_null();
        } else {
            if (reading && !j.is_object())
                invalid("Expected campaign delta fields");
            DeltaArchive nested{j, catalog, definitions, reading, 0, scratch};
            visit(nested, v, base);
            if (reading && j.size() != nested.fields)
                invalid("Unknown campaign delta field");
            return reading || !j.is_null();
        }
    }
};

// Allocation-free typed equality for definition admission; never stringify values
// merely to compare them. The same explicit field list defines both operations.
struct CampaignEqual {
    bool equal = true;
    template <class T> void operator()(const char*, T& value, const T& base) {
        equal = equal && same(value, base);
    }
    template <class T> void counted(const char*, T& value, std::size_t count, const T& base) {
        if (count > value.size()) {
            equal = false;
            return;
        }
        for (std::size_t i = 0; equal && i < count; ++i)
            equal = same(value[i], i < base.size() ? base[i] : campaignDefault<typename T::value_type>());
    }
    void floor(const char*, std::unique_ptr<FloorState>& value, const CampaignLocationState& base) {
        equal = equal && same(value, base.state);
    }
    template <class T> void required(const char* name, T& value, const T& base) {
        (*this)(name, value, base);
    }
    template <class T> static bool same(const T& a, const T& b) {
        if constexpr (DeltaArchive::leaf<T>)
            return DeltaArchive::equalLeaf(a, b);
        else if constexpr (Bits<T>::value)
            return a == b;
        else if constexpr (Array<T>::value || Vector<T>::value) {
            if (a.size() != b.size())
                return false;
            for (std::size_t i = 0; i < a.size(); ++i)
                if (!same(a[i], b[i]))
                    return false;
            return true;
        } else if constexpr (Optional<T>::value || Pointer<T>::value)
            return bool(a) == bool(b) && (!a || same(*a, *b));
        else {
            CampaignEqual fields;
            visit(fields, const_cast<T&>(a), b);
            return fields.equal;
        }
    }
};
