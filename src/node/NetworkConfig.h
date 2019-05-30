#pragma once

#include <yael/network/Address.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <glog/logging.h>

namespace relay
{

class NetworkConfig
{
public:
    struct edge_t
    {
        std::string from;
        std::string to;
        uint32_t delay;
    };

    NetworkConfig(const std::string &local_name, const std::string &filename);

    const std::string& get_name(const yael::network::Address &addr) const
    {
        for(auto &[name, addr_]: m_nodes)
        {
            if(addr == addr_)
            {
                return name;
            }
        }

        LOG(FATAL) << "No such node " << addr;
    }

    const yael::network::Address& get_node(const std::string &name) const
    {
        auto it = m_nodes.find(name);
        return it->second;
    }

    const std::string& local_name() const { return m_local_name; }

    const std::vector<edge_t>& edges() const { return m_edges; }

private:
    const std::string m_local_name;

    std::unordered_map<std::string, yael::network::Address> m_nodes;
    std::vector<edge_t> m_edges;
};

}
