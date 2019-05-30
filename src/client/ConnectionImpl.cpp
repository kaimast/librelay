#include "ConnectionImpl.h"
#include "common/defines.h"

#include <stdbitstream.h>
#include <yael/network/TcpSocket.h>

namespace relay
{

ConnectionImpl::ConnectionImpl(const yael::network::Address &address, Callback &callback)
    : m_callback(callback)
{
    using yael::network::TcpSocket;
    using yael::network::MessageMode;

    auto sock = new TcpSocket(MessageMode::Datagram, MAX_SEND_QUEUE);
    auto res = sock->connect(address);

    if(!res)
    {
        LOG(FATAL) << "Failed to connect to relay network";
    }

    set_socket(std::unique_ptr<yael::network::Socket>(sock), yael::SocketType::Connection);
    
    bitstream hello;
    hello << std::string("client");

    NetworkSocketListener::send(hello.data(), hello.size(), true);
}

ConnectionImpl::~ConnectionImpl() = default;

void ConnectionImpl::send(bitstream &&data, bool blocking)
{
    try {
        NetworkSocketListener::send(data.data(), data.size(), blocking);
    } catch(const yael::network::socket_error &e) {
        LOG(ERROR) << "Failed to send message to relay network " << e.what();
    }
}

void ConnectionImpl::on_network_message(yael::network::Socket::message_in_t &msg)
{
    bitstream block;
    block.assign(msg.data, msg.length, false);

    m_callback.on_new_message(std::move(block));
}

void ConnectionImpl::on_disconnect()
{
    m_callback.on_disconnect();
}

}
