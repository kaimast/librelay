#include "Peer.h"
#include "Node.h"
#include "common/defines.h"

#include <stdbitstream.h>
#include <yael/network/TcpSocket.h>

namespace relay
{

Peer::Peer(std::unique_ptr<yael::network::Socket> &&socket, Node &node, const NetworkConfig &config)
    : DelayedNetworkSocketListener(0, std::move(socket), yael::SocketType::Connection), m_node(node), m_config(config)
{}

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

    bitstream hello;
    hello << m_config.local_name();

    send(hello.data(), hello.size());
}

void Peer::on_network_message(yael::network::Socket::message_in_t &msg)
{
    bitstream input;
    input.assign(msg.data, msg.length, false);

    if(m_name.empty())
    {
        std::string name;
        input >> name;
        set_name(name);
        return;
    }

    auto except = std::dynamic_pointer_cast<Peer>(shared_from_this());
    m_node.queue_broadcast(std::move(input), except);
}

void Peer::on_disconnect()
{
    LOG(INFO) << "Peer @" << socket().get_remote_address() << " disconnected";

    auto myptr = std::dynamic_pointer_cast<Peer>(shared_from_this());
    m_node.remove_peer(myptr);
}

}
