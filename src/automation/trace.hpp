#pragma once
#include "automation/protocol.hpp"
#include <cstdio>
#include <fstream>
namespace blobber::automation {
constexpr std::size_t maxTraceBytes = 256u * 1024u * 1024u;
bool readLine(std::istream&, std::string&, std::size_t limit = maxRequestBytes);
class TraceWriter {
  public:
    explicit TraceWriter(const std::filesystem::path&, std::size_t limit = maxTraceBytes);
    ~TraceWriter();
    TraceWriter(const TraceWriter&) = delete;
    TraceWriter& operator=(const TraceWriter&) = delete;
    void append(const char* type, const Json&);
    void finish();

  private:
    FILE* file_ = nullptr;
    std::size_t bytes_ = 0, limit_;
};
class RecordedOutput final : public Output {
  public:
    RecordedOutput(std::ostream& out, TraceWriter* trace) : out_(out), trace_(trace) {}
    void emit(const Json&, const Json&) override;
    void response(const Json&, const Json& state);

  private:
    std::ostream& out_;
    TraceWriter* trace_;
};
// Diagnostic observer; called only at checked request boundaries. No state mutation.
class ReplayObserver {
  public:
    virtual ~ReplayObserver() = default;
    virtual void boundary(Session*, std::size_t operation, const Json& state) = 0;
};
void replay(Protocol&, const std::filesystem::path&, const Json& expectedHeader,
            ReplayObserver* observer = nullptr);
} // namespace blobber::automation
