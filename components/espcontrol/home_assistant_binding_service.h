#pragma once

#include <utility>

#include "ha_read_coordinator.h"

// Tracks the current callback owner independently of the transport-specific
// read coordinator. The application core owns the active instance; standalone
// binding services retain a local fallback for parser and host-test use.
class HomeAssistantCallbackOwnerService {
 public:
  class Scope {
   public:
    Scope(HomeAssistantCallbackOwnerService &service, void *owner)
        : service_(service), previous_(service.callback_owner_) {
      service_.callback_owner_ = owner;
    }
    ~Scope() { service_.callback_owner_ = previous_; }

    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;

   private:
    HomeAssistantCallbackOwnerService &service_;
    void *previous_ = nullptr;
  };

  void *callback_owner() const { return callback_owner_; }
  void *&callback_owner_ref() { return callback_owner_; }
  Scope scope(void *owner) { return Scope(*this, owner); }

 private:
  void *callback_owner_ = nullptr;
};

inline HomeAssistantCallbackOwnerService *&
home_assistant_callback_owner_service_binding() {
  static HomeAssistantCallbackOwnerService *service = nullptr;
  return service;
}

inline void set_home_assistant_callback_owner_service(
    HomeAssistantCallbackOwnerService *service) {
  home_assistant_callback_owner_service_binding() = service;
}

// Owns Home Assistant callback scope and read/subscription state. Transport and
// heap policies keep the service host-testable and leave ESPHome as wiring.
template<typename Transport, typename HeapProbe>
class HomeAssistantBindingService {
 public:
  using ReadCoordinator = HaReadCoordinator<Transport, HeapProbe>;
  using CallbackOwnerScope = HomeAssistantCallbackOwnerService::Scope;

  explicit HomeAssistantBindingService(
      Transport transport = Transport(), HeapProbe heap_probe = HeapProbe())
      : read_coordinator_(std::move(transport), std::move(heap_probe)) {}

  ReadCoordinator &read_coordinator() { return read_coordinator_; }
  const ReadCoordinator &read_coordinator() const { return read_coordinator_; }

  void *callback_owner() const { return callback_owner_service().callback_owner(); }
  void *&callback_owner_ref() { return callback_owner_service().callback_owner_ref(); }
  CallbackOwnerScope callback_owner_scope(void *owner) {
    return callback_owner_service().scope(owner);
  }

 private:
  HomeAssistantCallbackOwnerService &callback_owner_service() const {
    if (HomeAssistantCallbackOwnerService *service =
            home_assistant_callback_owner_service_binding()) {
      return *service;
    }
    return local_callback_owner_service_;
  }

  ReadCoordinator read_coordinator_{};
  mutable HomeAssistantCallbackOwnerService local_callback_owner_service_{};
};
