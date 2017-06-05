#include "server/config_validation/dispatcher.h"

#include "common/common/assert.h"

namespace Envoy {
namespace Event {

Network::ClientConnectionPtr
    ValidationDispatcher::createClientConnection(Network::Address::InstanceConstSharedPtr) {
  NOT_IMPLEMENTED;
}

Network::ClientConnectionPtr
ValidationDispatcher::createSslClientConnection(Ssl::ClientContext&,
                                                Network::Address::InstanceConstSharedPtr) {
  NOT_IMPLEMENTED;
}

Network::DnsResolverPtr ValidationDispatcher::createDnsResolver() { NOT_IMPLEMENTED; }

Network::ListenerPtr ValidationDispatcher::createListener(Network::ConnectionHandler&,
                                                          Network::ListenSocket&,
                                                          Network::ListenerCallbacks&,
                                                          Stats::Scope&,
                                                          const Network::ListenerOptions&) {
  NOT_IMPLEMENTED;
}

Network::ListenerPtr
ValidationDispatcher::createSslListener(Network::ConnectionHandler&, Ssl::ServerContext&,
                                        Network::ListenSocket&, Network::ListenerCallbacks&,
                                        Stats::Scope&, const Network::ListenerOptions&) {
  NOT_IMPLEMENTED;
}

} // Event
} // Envoy
