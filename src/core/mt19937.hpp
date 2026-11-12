#pragma once
#include <array>
#include <cstdint>
#include <istream>
#include <ostream>

namespace blobber {
// MT19937 with the existing 624-word-plus-index disk representation. Library stream
// formats differ between libstdc++ and MSVC; simulation and saves cannot depend on them.
class Mt19937 {
  public:
    using result_type = std::uint32_t;
    Mt19937() noexcept : Mt19937(5489u) {}
    explicit Mt19937(result_type value) noexcept { seed(value); }
    void seed(result_type value = 5489u) noexcept {
        words_[0] = value;
        for (std::uint32_t i = 1; i < words_.size(); ++i)
            words_[i] = 1812433253u * (words_[i - 1] ^ (words_[i - 1] >> 30)) + i;
        index_ = static_cast<unsigned>(words_.size());
    }
    static constexpr result_type min() noexcept { return 0; }
    static constexpr result_type max() noexcept { return UINT32_MAX; }
    result_type operator()() noexcept {
        if (index_ == words_.size()) {
            for (unsigned i = 0; i < words_.size(); ++i) {
                const auto value = (words_[i] & 0x80000000u) | (words_[(i + 1) % 624] & 0x7fffffffu);
                words_[i] = words_[(i + 397) % 624] ^ (value >> 1) ^ ((value & 1) ? 0x9908b0dfu : 0);
            }
            index_ = 0;
        }
        auto value = words_[index_++];
        value ^= value >> 11;
        value ^= (value << 7) & 0x9d2c5680u;
        value ^= (value << 15) & 0xefc60000u;
        return value ^ (value >> 18);
    }
    void discard(unsigned long long count) noexcept {
        while (count--)
            (void)(*this)();
    }
    friend bool operator==(const Mt19937& a, const Mt19937& b) noexcept {
        return a.index_ == b.index_ && a.words_ == b.words_;
    }
    friend bool operator!=(const Mt19937& a, const Mt19937& b) noexcept { return !(a == b); }
    friend std::ostream& operator<<(std::ostream& out, const Mt19937& value) {
        for (auto word : value.words_)
            out << word << ' ';
        return out << value.index_;
    }
    friend std::istream& operator>>(std::istream& in, Mt19937& value) {
        Mt19937 candidate;
        std::uint64_t word = 0;
        bool nonzero = false;
        for (auto& element : candidate.words_) {
            if (!(in >> word) || word > UINT32_MAX) {
                in.setstate(std::ios::failbit);
                return in;
            }
            element = static_cast<std::uint32_t>(word);
            nonzero |= element != 0;
        }
        if (!(in >> word) || word > 624 || !nonzero) {
            in.setstate(std::ios::failbit);
            return in;
        }
        candidate.index_ = static_cast<unsigned>(word);
        value = candidate;
        return in;
    }

  private:
    std::array<result_type, 624> words_{};
    unsigned index_ = 624;
};
} // namespace blobber
