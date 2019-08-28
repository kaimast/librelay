#include "Peer.h"
#include "Node.h"
#include "common/defines.h"

#include <stdbitstream.h>
#include <yael/network/TcpSocket.h>

namespace relay
{

Peer::Peer(std::unique_ptr<yael::network::Socket> &&socket, Node &node, const NetworkConfig &config)
    : DelayedNetworkSocketListener(0, std::move(socket), yael::SocketType::Connection), m_node(node), m_config(config)
{
    std::set<channel_id_t> subscriptions;
    for(uint32_t i = 0; i < m_config.num_channels(); ++i)
    {
        subscriptions.insert(i);
    }

    bitstream hello;
    hello << m_config.local_name() << subscriptions;

    send(hello.data(), hello.size());
}

Peer::Peer(const yael::network::Address &addr, Node &node, const NetworkConfig &config, const std::string &name)
    : DelayedNetworkSocketListener(0), m_node(node), m_config(config)
{
    using yael::network::TcpSocket;
    using yael::network::MessageMode;

    auto s = std::make_unique<TcpSocket>(MessageMode::Datagram, MAX_SEND_QUEUE);
    bool res = s->connect(addr);

    if(res)
    {
        LOG(INFO) << "Connected to peer " << name << " @" << addr;
    }
    else
    {
        LOG(FATAL) << "Connection to peer " << name << " @" << addr << " failed";
    }

    set_socket(std::move(s), yael::SocketType::Connection);
    set_name(name);

    std::set<channel_id_t> subscriptions;
    for(uint32_t i = 0; i < m_config.num_channels(); ++i)
    {
        subscriptions.insert(i);
    }

    bitstream hello;
    hello << m_config.local_name() << subscriptions;

    send(hello.data(), hello.size());
}

void Peer::on_network_message(yael::network::message_in_t &msg)
{
    bitstream input;
    input.assign(msg.data, msg.length, false);

    if(!m_set_up)
    {
        std::string name;
        input >> name >> m_subscriptions;

        if(m_name.empty())
        {
            set_name(name);
        }

        m_set_up = true;
        return;
    }

    std::set<channel_id_t> channels;
    input >> channels;
    input.move_to(0);
    input.remove_space(sizeof(uint32_t) + channels.size()*sizeof(channel_id_t));

    auto except = std::dynamic_pointer_cast<Peer>(shared_from_this());
    m_node.queue_broadcast(channels, std::move(input), except);
}

void Peer::on_disconnect()
{
    LOG(INFO) << "Peer @" << socket().get_remote_address() << " disconnected";

    auto myptr = std::dynamic_pointer_cast<Peer>(shared_from_this());
    m_node.remove_peer(myptr);
}

}
