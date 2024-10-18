#pragma once

#include <bitstream.h>
#include <condition_variable>
#include <list>
#include <memory>
#include <set>
#include <thread>
#include <vector>
#include <yael/NetworkSocketListener.h>
#include <yael/network/Address.h>

#include "MessageCache.h"
#include "NetworkConfig.h"
#include "Storage.h"
#include "librelay/Connection.h"

namespace relay {

class Peer;

class Node : public yael::NetworkSocketListener {
  public:
    // Constructor for full nodes
    Node(const std::string &name, const std::string &config_file,
         uint32_t wait);

    // Constructor for edge nodes
    Node(const std::string &address, const std::string &peer,
         const std::string &config_file, uint32_t wait);

    ~Node();

    void remove_peer(std::shared_ptr<Peer> peer);

    void queue_broadcast(std::set<channel_id_t> channels, bitstream &&msg,
                         const std::shared_ptr<Peer> &excpet);

  private:
    struct Task {
        std::set<channel_id_t> channels;
        bitstream msg;
        std::shared_ptr<Peer> except;
    };

    void work();

    void broadcast(std::set<channel_id_t> channel, bitstream &&msg,
                   const std::shared_ptr<Peer> &except);

    void connect(const std::string &name, const yael::network::Address &addr);

    void
    on_new_connection(std::unique_ptr<yael::network::Socket> &&socket) override;

    std::mutex m_peer_mutex;
    std::vector<std::shared_ptr<Peer>> m_peers;

    std::mutex m_task_mutex;
    std::list<Task *> m_tasks;
    std::vector<std::thread> m_workers;

    std::condition_variable_any m_in_condition;
    std::condition_variable_any m_out_condition;

    const NetworkConfig m_config;

    Storage m_message_cache;
};

} // namespace relay
