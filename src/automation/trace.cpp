#include "automation/trace.hpp"
#include <cerrno>
#include <cstring>
#include <stdexcept>
namespace blobber::automation {
bool readLine(std::istream& input, std::string& line, std::size_t limit) {
    line.clear();
    char c;
    while (input.get(c)) {
        if (c == '\n')
            return true;
        if (line.size() == limit)
            throw std::runtime_error("Input line exceeds byte limit");
        line.push_back(c);
    }
    if (input.bad())
        throw std::runtime_error("Input read failed");
    return !line.empty();
}
TraceWriter::TraceWriter(const std::filesystem::path& path, std::size_t limit) : limit_(limit) {
    file_ = std::fopen(path.string().c_str(), "wx");
    if (!file_)
        throw std::runtime_error("Cannot exclusively create trace: " + std::string(std::strerror(errno)));
}
TraceWriter::~TraceWriter() {
    if (file_)
        std::fclose(file_);
}
void TraceWriter::append(const char* type, const Json& data) {
    if (!file_)
        throw std::runtime_error("Trace already closed");
    const auto text = Json{{"type", type}, {"data", data}}.dump() + "\n";
    if (text.size() - 1 > maxTraceRecordBytes)
        throw std::runtime_error("Trace record exceeds byte limit; recording incomplete");
    if (text.size() > limit_ - bytes_)
        throw std::runtime_error("Trace reached 256 MiB limit; recording incomplete");
    if (std::fwrite(text.data(), 1, text.size(), file_) != text.size() || std::fflush(file_) != 0)
        throw std::runtime_error("Trace write failed; recording incomplete");
    bytes_ += text.size();
}
void TraceWriter::finish() {
    append("complete", {{"complete", true}});
    auto* file = file_;
    file_ = nullptr;
    if (std::fclose(file) != 0)
        throw std::runtime_error("Trace close failed");
}
void RecordedOutput::emit(const Json& full, const Json& shown) {
    if (trace_)
        trace_->append("event", full);
    if (!shown.is_null()) {
        out_ << shown.dump() << '\n';
        if (!out_)
            throw std::runtime_error("Protocol output failed");
    }
}
void RecordedOutput::response(const Json& r, const Json& state) {
    if (trace_) {
        trace_->append("response", r);
        trace_->append("state", state);
    }
    out_ << r.dump() << '\n';
    out_.flush();
    if (!out_)
        throw std::runtime_error("Protocol output failed");
}
namespace {
class ReplayOutput final : public Output {
  public:
    explicit ReplayOutput(std::istream& in) : input(in) {}
    Json next(const char* type) {
        if (!readLine(input, line, maxTraceRecordBytes))
            throw std::runtime_error("Incomplete recording at operation " + std::to_string(operation));
        bytes += line.size() + 1;
        if (bytes > maxTraceBytes)
            throw std::runtime_error("Trace exceeds byte limit");
        const auto record = parseTraceRecord(line);
        if (record.at("type") != type)
            throw std::runtime_error("Unexpected trace record at operation " + std::to_string(operation) +
                                     ": expected " + type);
        return record.at("data");
    }
    void check(const char* type, const Json& actual) {
        const auto expected = next(type);
        if (expected != actual) {
            const auto diff = Json::diff(expected, actual);
            throw std::runtime_error("Replay mismatch at operation " + std::to_string(operation) + " " +
                                     type + " " + diff.front().at("path").get<std::string>());
        }
    }
    void emit(const Json& full, const Json&) override { check("event", full); }
    std::istream& input;
    std::string line;
    std::size_t operation = 0, bytes = 0;
};
} // namespace
void replay(Protocol& protocol, const std::filesystem::path& path, const Json& expectedHeader) {
    std::ifstream input(path);
    if (!input)
        throw std::runtime_error("Cannot open recording");
    ReplayOutput out(input);
    out.check("header", expectedHeader);
    while (true) {
        if (!readLine(input, out.line, maxTraceRecordBytes))
            throw std::runtime_error("Incomplete recording: no complete footer");
        out.bytes += out.line.size() + 1;
        if (out.bytes > maxTraceBytes)
            throw std::runtime_error("Trace exceeds byte limit");
        const auto record = parseTraceRecord(out.line);
        if (record.at("type") == "complete") {
            if (record.at("data") != Json{{"complete", true}} ||
                readLine(input, out.line, maxTraceRecordBytes))
                throw std::runtime_error("Invalid recording footer or trailing data");
            return;
        }
        if (record.at("type") != "request")
            throw std::runtime_error("Expected trace request");
        ++out.operation;
        const auto response = protocol.request(record.at("data"), out);
        out.check("response", response);
        out.check("state", protocol.state());
    }
}
} // namespace blobber::automation
