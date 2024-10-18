#pragma once

#include <glog/logging.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <yael/network/Address.h>

namespace relay {

class NetworkConfig {
  public:
    struct edge_t {
        std::string from;
        std::string to;
        uint32_t delay;
    };

    NetworkConfig(const std::string &local_name, const std::string &filename);

    const std::string &get_name(const yael::network::Address &addr) const {
        for (auto &[name, addr_] : m_nodes) {
            if (addr == addr_) {
                return name;
            }
        }

        LOG(FATAL) << "No such node " << addr;
    }

    const yael::network::Address &get_node(const std::string &name) const {
        auto it = m_nodes.find(name);
        return it->second;
    }

    const std::string &local_name() const { return m_local_name; }

    const std::vector<edge_t> &edges() const { return m_edges; }

    uint32_t num_channels() const { return m_num_channels; }

  private:
    const std::string m_local_name;

    uint32_t m_num_channels;
    std::unordered_map<std::string, yael::network::Address> m_nodes;
    std::vector<edge_t> m_edges;
};

} // namespace relay
