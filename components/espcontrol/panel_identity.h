#pragma once

#include <string>
#include "esphome/core/component.h"
#include "esphome/core/string_ref.h"
#include "panel_identity_model.h"

namespace espcontrol {
class PanelIdentity : public esphome::Component {
 public:
  void setup() override;
  float get_setup_priority() const override { return 1000.0f; }
  void set_web_auth_credentials(const char *username, const char *password) {
    username_ = username;
    password_ = password;
  }
  bool save(const std::string &name);
  int storage_error() const { return storage_error_; }
  bool ready() const { return ready_; }
  const std::string &saved_name() const { return saved_name_; }
  const std::string &suffix() const { return suffix_; }
  std::string target_name() const { return saved_name_.empty() ? default_friendly_ : saved_name_; }
  std::string target_hostname() const { return saved_name_.empty() ? default_hostname_ : panel_hostname(saved_name_, suffix_); }
  bool restart_required() const { return saved_name_ != boot_name_; }
  const char *username() const { return username_; }
  const char *password() const { return password_; }
 private:
  std::string saved_name_, boot_name_, suffix_, default_hostname_, default_friendly_;
  // Never modify these buffers after setup: the native API retains StringRefs.
  std::string running_hostname_, running_friendly_;
  const char *username_{""};
  const char *password_{""};
  bool ready_{false};
  int storage_error_{0};
};
extern PanelIdentity *panel_identity;
}  // namespace espcontrol
