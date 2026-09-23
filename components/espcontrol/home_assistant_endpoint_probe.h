#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace espcontrol {

enum class EndpointProbeOutcome { READY, TRANSPORT, RESPONSE, ACCESS_DENIED, RESOURCE };
struct EndpointProbeResult {
  uint32_t generation{0};
  EndpointProbeOutcome outcome{EndpointProbeOutcome::TRANSPORT};
  int status{0};
  int error{0};
};

class EndpointProbe {
 public:
  virtual ~EndpointProbe() = default;
  virtual bool start(const std::string &origin, uint32_t generation) = 0;
  virtual bool take(EndpointProbeResult &result) = 0;
  virtual void shutdown() = 0;
};

// One transient task, one result. A timed-out DNS/socket operation must finish
// before another task is admitted; obsolete jobs never accumulate.
class EndpointProbeService : public EndpointProbe {
 public:
  EndpointProbeService();
  ~EndpointProbeService() override;
  bool start(const std::string &origin, uint32_t generation) override;
  bool take(EndpointProbeResult &result) override;
  void shutdown() override;
 private:
  struct Job;
  std::shared_ptr<Job> job_;
  bool delivered_{false};
  static void run(void *argument);
};
}  // namespace espcontrol
