#pragma once

#include <yael/DelayedNetworkSocketListener.h>

#include "NetworkConfig.h"

namespace relay
{

class Node;

class Peer : public yael::DelayedNetworkSocketListener
{
public:
    Peer(std::unique_ptr<yael::network::Socket> &&socket, Node &node, const NetworkConfig &config);

    Peer(const yael::network::Address &addr, Node &node, const NetworkConfig &config, const std::string &name);

    const std::string& name() const { return m_name; }
 
private:
    void on_network_message(yael::network::Socket::message_in_t &msg) override;
    void on_disconnect() override;

    void set_name(const std::string &name);

    Node &m_node;
    const NetworkConfig &m_config;
    std::string m_name;
};

inline void Peer::set_name(const std::string &name)
{
    if(name.empty())
    {
        // edge node
        // set to some dummy value
        m_name = "\n";
        return;
    }
    
    m_name = name;
    auto delay = 0;

    for(auto &e: m_config.edges())
    {
        if(e.from == m_config.local_name()
            && e.to == m_name)
        {
            delay = e.delay;
    
            LOG(INFO) << "Connected to relay " << m_name;
            DelayedNetworkSocketListener::set_delay(delay);

            return;
        }
    }
}

}
