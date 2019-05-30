#pragma once

#include <yael/NetworkSocketListener.h>
#include "librelay/Connection.h"

namespace relay
{

class ConnectionImpl : public yael::NetworkSocketListener, public Connection
{
public:
    ConnectionImpl(const yael::network::Address &address, Callback &callback);
    ~ConnectionImpl();

    void send(bitstream &&data, bool blocking) override;

private:
    void on_network_message(yael::network::Socket::message_in_t &msg) override;
    void on_disconnect() override;

    Callback &m_callback;
};

}
