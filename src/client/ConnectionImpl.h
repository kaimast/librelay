#pragma once

#include <set>
#include <yael/NetworkSocketListener.h>
#include "librelay/Connection.h"

namespace relay
{

class ConnectionImpl : public yael::NetworkSocketListener, public Connection
{
public:
    ConnectionImpl(const yael::network::Address &address, Callback &callback, std::set<channel_id_t> subscriptions);
    ~ConnectionImpl();

    void send(std::set<channel_id_t> channels, bitstream &&data, bool blocking) override;
    
    void close() override { yael::NetworkSocketListener::close_socket(); }

private:
    void on_network_message(yael::network::Socket::message_in_t &msg) override;
    void on_disconnect() override;

    Callback &m_callback;
    const std::set<channel_id_t> m_subscriptions;
};

}
