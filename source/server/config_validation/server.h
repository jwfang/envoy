#pragma once

#include <iostream>

#include "envoy/common/optional.h"
#include "envoy/server/drain_manager.h"
#include "envoy/server/instance.h"
#include "envoy/ssl/context_manager.h"
#include "envoy/tracing/http_tracer.h"

#include "common/access_log/access_log_manager_impl.h"
#include "common/common/assert.h"
#include "common/runtime/runtime_impl.h"
#include "common/ssl/context_manager_impl.h"
#include "common/stats/stats_impl.h"
#include "common/thread_local/thread_local_impl.h"

#include "server/config_validation/api.h"
#include "server/config_validation/cluster_manager.h"
#include "server/config_validation/connection_handler.h"
#include "server/config_validation/dns.h"
#include "server/http/admin.h"
#include "server/server.h"
#include "server/worker.h"

namespace Envoy {
namespace Server {

/**
 * validateConfig() takes over from main() for a config-validation run of Envoy. It returns true if
 * the config is valid, false if invalid.
 */
bool validateConfig(Options& options, ComponentFactory& component_factory,
                    const LocalInfo::LocalInfo& local_info);

/**
 * ValidationInstance does the bulk of the work for config-validation runs of Envoy. It implements
 * Server::Instance, but some functionality not needed until serving time, such as updating
 * health-check status, is not implemented. Everything else is written in terms of other
 * validation-specific interface implementations, with the end result that we can load and
 * initialize a configuration, skipping any steps that affect the outside world (such as
 * hot-restarting or connecting to upstream clusters) but otherwise exercising the entire startup
 * flow.
 *
 * If we finish initialization, and reach the point where an ordinary Envoy run would begin serving
 * requests, the validation is considered successful.
 */
class ValidationInstance : Logger::Loggable<Logger::Id::main>, public Instance {
public:
  ValidationInstance(Options& options, Stats::IsolatedStoreImpl& store,
                     Thread::BasicLockable& access_log_lock, ComponentFactory& component_factory,
                     const LocalInfo::LocalInfo& local_info);

  // Server::Instance
  Admin& admin() override { NOT_IMPLEMENTED; }
  Api::Api& api() override { return handler_.api(); }
  Upstream::ClusterManager& clusterManager() override { return config_->clusterManager(); }
  Ssl::ContextManager& sslContextManager() override { return *ssl_context_manager_; }
  Event::Dispatcher& dispatcher() override { return handler_.dispatcher(); }
  Network::DnsResolver& dnsResolver() override { return dns_resolver_; }
  bool draining() override { NOT_IMPLEMENTED; }
  void drainListeners() override { NOT_IMPLEMENTED; }
  DrainManager& drainManager() override { NOT_IMPLEMENTED; }
  AccessLog::AccessLogManager& accessLogManager() override { return access_log_manager_; }
  void failHealthcheck(bool) override { NOT_IMPLEMENTED; }
  int getListenSocketFd(const std::string&) override { NOT_IMPLEMENTED; }
  Network::ListenSocket* getListenSocketByIndex(uint32_t) override { NOT_IMPLEMENTED; }
  void getParentStats(HotRestart::GetParentStatsInfo&) override { NOT_IMPLEMENTED; }
  HotRestart& hotRestart() override { NOT_IMPLEMENTED; }
  Init::Manager& initManager() override { return init_manager_; }
  Runtime::RandomGenerator& random() override { return random_generator_; }
  RateLimit::ClientPtr
  rateLimitClient(const Optional<std::chrono::milliseconds>& timeout) override {
    return config_->rateLimitClientFactory().create(timeout);
  }
  Runtime::Loader& runtime() override { return *runtime_loader_; }
  void shutdown() override;
  void shutdownAdmin() override { NOT_IMPLEMENTED; }
  bool healthCheckFailed() override { NOT_IMPLEMENTED; }
  Options& options() override { return options_; }
  time_t startTimeCurrentEpoch() override { NOT_IMPLEMENTED; }
  time_t startTimeFirstEpoch() override { NOT_IMPLEMENTED; }
  Stats::Store& stats() override { return stats_store_; }
  Tracing::HttpTracer& httpTracer() override { return config_->httpTracer(); }
  ThreadLocal::Instance& threadLocal() override { return thread_local_; }
  const LocalInfo::LocalInfo& localInfo() override { return local_info_; }

private:
  void initialize(Options& options, ComponentFactory& component_factory);

  Options& options_;
  Stats::IsolatedStoreImpl& stats_store_;
  ThreadLocal::InstanceImpl thread_local_;
  ValidationConnectionHandler handler_;
  Runtime::LoaderPtr runtime_loader_;
  Runtime::RandomGeneratorImpl random_generator_;
  std::unique_ptr<Ssl::ContextManagerImpl> ssl_context_manager_;
  std::unique_ptr<Configuration::Main> config_;
  Network::ValidationDnsResolver dns_resolver_;
  const LocalInfo::LocalInfo& local_info_;
  AccessLog::AccessLogManagerImpl access_log_manager_;
  std::unique_ptr<Upstream::ValidationClusterManagerFactory> cluster_manager_factory_;
  InitManagerImpl init_manager_;
};

} // Server
} // Envoy
