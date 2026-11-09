#pragma once
#include "game/dialogue.hpp"
#include "game/game_time.hpp"
#include "game/inventory.hpp"

namespace blobber {
inline constexpr unsigned merchantCapacity = 16, shopStockCapacity = 48, buybackCapacity = 48;
struct ShopPrice {
    unsigned buy = 0, sell = 0;
};
struct ShopStock {
    rules::ItemId item;
    unsigned quantity = 0;
};
struct ShopDefinition {
    DialogueKey id = 0;
    std::string key;
    unsigned restockDays = 3, buybackHours = 24, markupPercent = 25;
    std::array<ShopPrice, rules::Catalog::maxItems> prices{};
    std::vector<ShopStock> stock;
};
struct ShopCatalog {
    std::vector<ShopDefinition> shops;
    const ShopDefinition* find(DialogueKey) const noexcept;
};
struct BuybackStack {
    rules::ItemId item;
    unsigned quantity = 0, price = 0;
    std::uint64_t expiresMs = 0;
};
struct MerchantState {
    DialogueKey resident = 0, shop = 0;
    std::uint64_t revision = 1, restockMs = 0;
    std::array<unsigned, rules::Catalog::maxItems> stock{};
    std::array<BuybackStack, buybackCapacity> buyback{};
};
class MerchantLedger {
  public:
    const MerchantState* find(DialogueKey resident) const noexcept;
    MerchantState* visit(DialogueKey resident, const ShopDefinition&, GameTime) noexcept;
    std::vector<MerchantState> checkpoint() const;
    void restore(const std::vector<MerchantState>&, const rules::Catalog&);
    static bool refresh(MerchantState&, const ShopDefinition&, GameTime) noexcept;

  private:
    std::array<MerchantState, merchantCapacity> merchants_{};
    unsigned count_ = 0;
};
enum class TradeSide : std::uint8_t { Merchant, Player };
struct TradeRequest {
    DialogueKey resident = 0, shop = 0;
    std::uint64_t merchantRevision = 0, inventoryRevision = 0, inventoryIdentity = 0;
    rules::CharacterId character;
    TradeSide side = TradeSide::Merchant;
    unsigned offer = 0, quantity = 1; // 0..47 authored stock, 48..95 buyback.
    ItemHandle item;
};
enum class TradeError : std::uint8_t {
    None,
    Unavailable,
    Stale,
    Quantity,
    SoldOut,
    NotForSale,
    Funds,
    Full,
    Overweight,
    BuybackFull,
    GoldOverflow
};
struct TradeResult {
    TradeError error = TradeError::None;
    unsigned unitPrice = 0;
    std::uint64_t total = 0;
    explicit operator bool() const noexcept { return error == TradeError::None; }
};
const char* tradeMessage(TradeError) noexcept;
class Party;
TradeResult trade(Party&, MerchantLedger&, const ShopCatalog&, const TradeRequest&, GameTime,
                  bool preview = false) noexcept;
std::optional<EquipmentSlot> itemEquipmentSlot(const rules::ItemDefinition&) noexcept;
struct ItemComparison {
    const rules::ItemDefinition* equipped = nullptr;
    rules::WeaponPreview offeredWeapon, equippedWeapon;
    int offeredArmorClass = 0, equippedArmorClass = 0;
    rules::CharacterStats currentStats{}, offeredStats{};
    bool equippable = false;
    const char* restriction = "";
};
ItemComparison compareItem(const Party&, rules::CharacterId, const rules::ItemDefinition&) noexcept;
} // namespace blobber
