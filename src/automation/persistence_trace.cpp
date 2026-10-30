#include "automation/trace.hpp"
namespace blobber::automation {
namespace {
std::string hex(const std::string& text, std::size_t offset, std::size_t maximum) {
    constexpr const char* digits = "0123456789abcdef";
    const auto size = std::min(maximum, text.size() - offset);
    std::string out(size * 2, '0');
    for (std::size_t i = 0; i < size; ++i) {
        const auto byte = static_cast<unsigned char>(text[offset + i]);
        out[2 * i] = digits[byte >> 4];
        out[2 * i + 1] = digits[byte & 15];
    }
    return out;
}
std::string unhex(const std::string& text, std::size_t size) {
    if (text.size() != size * 2)
        throw std::runtime_error("Invalid persistence chunk size");
    const auto nibble = [](char c) -> unsigned {
        if (c >= '0' && c <= '9')
            return unsigned(c - '0');
        if (c >= 'a' && c <= 'f')
            return unsigned(c - 'a' + 10);
        throw std::runtime_error("Invalid persistence byte encoding");
    };
    std::string out(size, '\0');
    for (std::size_t i = 0; i < size; ++i)
        out[i] = char(nibble(text[2 * i]) * 16 + nibble(text[2 * i + 1]));
    return out;
}
} // namespace

CampaignIoReply TraceCampaignFiles::exchange(const CampaignIoRequest& request) {
    constexpr std::size_t chunkBytes = 256u * 1024u;
    const auto chunks = [&](const char* phase, const std::string& data) {
        const Json meta = {{"phase", phase}, {"bytes", data.size()}, {"encoding", "hex"}};
        if (reader_) {
            if (reader_("persistence_payload") != meta)
                throw std::runtime_error("Persistence trace payload mismatch");
            for (std::size_t offset = 0; offset < data.size(); offset += chunkBytes)
                if (reader_("persistence_chunk") != Json(hex(data, offset, chunkBytes)))
                    throw std::runtime_error("Persistence write replay mismatch");
        } else if (writer_) {
            writer_->append("persistence_payload", meta);
            for (std::size_t offset = 0; offset < data.size(); offset += chunkBytes)
                writer_->append("persistence_chunk", hex(data, offset, chunkBytes));
        }
    };
    const Json meta = {{"operation", request.operation}, {"expected", request.expected}};
    if (reader_) {
        if (reader_("persistence_request") != meta)
            throw std::runtime_error("Persistence operation replay mismatch");
        if (request.operation != "identity")
            chunks("input", request.data);
        auto result = reader_("persistence_result");
        fields(result, {"error", "exists", "published", "durable", "bytes", "encoding"});
        if (result.at("encoding") != "hex")
            throw std::runtime_error("Unsupported persistence byte encoding");
        CampaignIoReply reply;
        reply.error = result.at("error").get<std::string>();
        reply.exists = result.at("exists").get<bool>();
        reply.published = result.at("published").get<bool>();
        reply.durable = result.at("durable").get<bool>();
        const auto bytes = bounded(result.at("bytes"), campaignMaxBytes);
        for (std::size_t offset = 0; offset < bytes; offset += chunkBytes) {
            auto chunk = reader_("persistence_chunk").get<std::string>();
            reply.data += unhex(chunk, std::min<std::size_t>(chunkBytes, bytes - offset));
        }
        return reply;
    }
    if (writer_)
        writer_->append("persistence_request", meta);
    if (request.operation != "identity")
        chunks("input", request.data);
    if (!disk_)
        throw std::runtime_error("Persistence backend unavailable");
    auto reply = disk_->exchange(request);
    if (writer_) {
        writer_->append("persistence_result", {{"error", reply.error},
                                               {"exists", reply.exists},
                                               {"published", reply.published},
                                               {"durable", reply.durable},
                                               {"bytes", reply.data.size()},
                                               {"encoding", "hex"}});
        for (std::size_t offset = 0; offset < reply.data.size(); offset += chunkBytes)
            writer_->append("persistence_chunk", hex(reply.data, offset, chunkBytes));
    }
    return reply;
}
} // namespace blobber::automation
