#include "ConnectionImpl.h"
#include "common/defines.h"

#include <stdbitstream.h>
#include <yael/network/TcpSocket.h>

namespace relay
{

ConnectionImpl::ConnectionImpl(const yael::network::Address &address, Callback &callback, std::set<channel_id_t> subscriptions)
    : m_callback(callback), m_subscriptions(subscriptions)
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
    hello << std::string("client") << m_subscriptions;

    NetworkSocketListener::send(hello.data(), hello.size(), true);
}

ConnectionImpl::~ConnectionImpl() = default;

void ConnectionImpl::send(const std::set<channel_id_t> &channels, bitstream &&data, bool blocking)
{
    try {
        // prepend channel id to message
        data.move_to(0);
        data.make_space(sizeof(uint32_t) + channels.size()*sizeof(channel_id_t));
        data << channels;

        // pass data to the network layer in form a simple buffer
        uint8_t *ptr = 0;
        uint32_t size;
        data.detach(ptr, size);

        NetworkSocketListener::send(std::unique_ptr<uint8_t[]>(ptr), size, blocking);
    } catch(const yael::network::socket_error &e) {
        LOG(ERROR) << "Failed to send message to relay network " << e.what();
    }
}

void ConnectionImpl::on_network_message(yael::network::Socket::message_in_t &msg)
{
    if(!m_set_up)
    {
        // we don't actually need to handle the hello message
        m_set_up = true;
        return;
    }

    std::set<channel_id_t> channels;

    bitstream bs;
    bs.assign(msg.data, msg.length, false);
    bs >> channels;

    bs.move_to(0);
    bs.remove_space(sizeof(uint32_t) + channels.size()*sizeof(channel_id_t));

    m_callback.on_new_message(channels, std::move(bs));
}

void ConnectionImpl::on_disconnect()
{
    m_callback.on_disconnect();
}

}
