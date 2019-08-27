#include "NetworkConfig.h"

#include <fstream>
#include <streambuf>

#include <json/Document.h>
#include <glog/logging.h>

namespace relay
{

constexpr uint16_t SERVER_PORT = 5050;

yael::network::Address read_address(const std::string &addr_str)
{
    size_t found = addr_str.find(':');

    if(found == std::string::npos)
    {
        // use default port
        return yael::network::Address(addr_str, SERVER_PORT);
    }
    else
    {
        auto host = addr_str.substr(0, found);
        auto port = std::atoi(addr_str.substr(found+1, std::string::npos).c_str());

        if(port <= 0 || port > std::numeric_limits<uint16_t>::max())
        {
            LOG(ERROR) << "Not a valid port number: " << port;
        }

        return yael::network::resolve_URL(host, port);
    }
}

NetworkConfig::NetworkConfig(const std::string &local_name, const std::string &filename)
    : m_local_name(local_name)
{
    try {
        std::ifstream f(filename);

        std::string str((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());

        json::Document doc(str);

        m_num_channels = json::Document(doc, "num_channels").as_integer();

        json::Document nodes(doc, "nodes");

        for(size_t i = 0; i < nodes.get_size(); ++i)
        {
            auto name = nodes.get_key(i);
            auto ip_str = nodes.get_child(i).as_string();

            m_nodes.emplace(name, read_address(ip_str));
        }

        json::Document edges(doc, "edges");
        for(size_t i = 0; i < edges.get_size(); ++i)
        {
            json::Document entry(edges, i);

            auto from = json::Document(entry, "from").as_string();
            auto to = json::Document(entry, "to").as_string();
            auto delay = json::Document(entry, "delay").as_integer();

            m_edges.emplace_back(edge_t{from, to, static_cast<uint32_t>(delay)});
        }
    } catch(std::exception &e) {
        LOG(FATAL) << "Failed to load network config: " << e.what();
    }
}

}
