#include "Node.h"
#include "Peer.h"

#include <thread>
#include <chrono>

#include <yael/EventLoop.h>
#include <yael/network/TcpSocket.h>

namespace relay
{

// Cache at most 100Mb
constexpr size_t MEM_SIZE = 100 * 1024 * 1024;

inline yael::network::Address read_address(const std::string &addr_str)
{
    size_t found = addr_str.find(':');

    if(found == std::string::npos)
    {
        LOG(FATAL) << "No port specified in address";
    }

    auto host = addr_str.substr(0, found);
    auto port = std::atoi(addr_str.substr(found+1, std::string::npos).c_str());

    if(port <= 0 || port > std::numeric_limits<uint16_t>::max())
    {
        LOG(ERROR) << "Not a valid port number: " << port;
    }

    return yael::network::resolve_URL(host, port);
}


Node::Node(const std::string &name, const std::string &config_file, uint32_t wait)
    : m_config(name, config_file), m_message_cache("relay-" + name, MEM_SIZE)
{
    LOG(INFO) << "Starting relay node " << name;

    auto sock = new yael::network::TcpSocket();
    auto addr = m_config.get_node(name);
    addr.IP = "0.0.0.0";
    auto res = sock->listen(addr, 100);

    if(res)
    {
        LOG(INFO) << "Listening for relay connections on " << addr;
    }
    else
    {
        LOG(FATAL) << "Failed to bind socket!";
    }

    this->set_socket(std::unique_ptr<yael::network::Socket>(sock), yael::SocketType::Acceptor);

    // Wait for other nodes to come up
    std::this_thread::sleep_for(std::chrono::seconds(wait));

    // Set up topology
    for(auto &e: m_config.edges())
    {
        if(e.from == m_config.local_name())
        {
            auto &to_addr = m_config.get_node(e.to);
            connect(e.to, to_addr);
        }
    }

    // start worker threads
    for(size_t i = 0; i < 8; ++i)
    {
        m_workers.emplace_back(std::thread(&Node::work, this));
    }
}

Node::Node(const std::string &addr_str, const std::string &peer_addr_str, const std::string &config_file, uint32_t wait)
    : m_config("", config_file), m_message_cache("relay-edge-node", MEM_SIZE)
{
    LOG(INFO) << "Starting relay edge node";

    auto sock = new yael::network::TcpSocket();
    auto addr = read_address(addr_str);
    auto res = sock->listen(addr, 100);

    if(res)
    {
        LOG(INFO) << "Listening for relay connections on " << addr;
    }
    else
    {
        LOG(FATAL) << "Failed to bind socket!";
    }

    this->set_socket(std::unique_ptr<yael::network::Socket>(sock), yael::SocketType::Acceptor);

    // Wait for other nodes to come up
    std::this_thread::sleep_for(std::chrono::seconds(wait));

    // Connect to the peer node
    auto peer_addr = read_address(peer_addr_str);
    connect("", peer_addr);

    // start worker threads
    for(size_t i = 0; i < 8; ++i)
    {
        m_workers.emplace_back(std::thread(&Node::work, this));
    }
}

Node::~Node()
{
    m_in_condition.notify_all();
    m_out_condition.notify_all();

    for(auto &t : m_workers)
    {
        t.join();
    }

    for(auto tasks : m_tasks)
    {
        delete tasks;
    }
}

void Node::connect(const std::string &name, const yael::network::Address &addr)
{
    auto &el = yael::EventLoop::get_instance();
    auto p = el.make_event_listener<Peer>(addr, *this, m_config, name);

    std::unique_lock lock(m_peer_mutex);
    m_peers.push_back(p);
}

void Node::on_new_connection(std::unique_ptr<yael::network::Socket> &&socket)
{
    auto &el = yael::EventLoop::get_instance();
    auto peer = el.make_event_listener<Peer>(std::move(socket), *this, m_config);

    std::unique_lock lock(m_peer_mutex);
    m_peers.push_back(peer);
    lock.unlock();

    auto it = m_message_cache.iterate();

    // loop until we sent everything we saw
    while(peer->is_connected())
    {
        auto hdl = it.next();

        if(!hdl)
        {
            break;
        }

        auto msg = hdl->data();

        // there might be a lot of messages queued up,
        // so we might need to block
        //
        // TODO move this to the peer class so it does not block the acceptor
        bool blocking = true;
        peer->send(msg.data(), msg.size(), blocking);
    }
}

void Node::work()
{
    while(is_valid())
    {
        Task *task = nullptr;

        {
            std::unique_lock lock(m_task_mutex);
            while(is_valid() && task == nullptr)
            {
                if(m_tasks.empty())
                {
                    m_in_condition.wait(lock);
                }
                else
                {
                    task = m_tasks.front();
                    m_tasks.pop_front();
                    m_out_condition.notify_all();
                }
            }
        }

        if(task)
        {
            broadcast(std::move(task->msg), task->except);
            delete task;
        }
    }
}

void Node::queue_broadcast(bitstream &&msg, const std::shared_ptr<Peer> &except)
{
    std::unique_lock lock(m_task_mutex);
    
/* Find a way to do this without locking up all worker threads
 * while(m_tasks.size() > MAX_QUEUE_LENGTH && is_valid())
    {
        m_out_condition.wait(lock);
    }*/

    m_tasks.push_back(new Task{std::move(msg), except});
    m_in_condition.notify_one();
}

void Node::broadcast(bitstream &&msg, const std::shared_ptr<Peer> &except)
{
    auto hdl = m_message_cache.insert(std::move(msg));

    std::unique_lock lock(m_peer_mutex);
    std::vector<std::shared_ptr<Peer>> ccopy(m_peers.size());
    std::copy(m_peers.begin(), m_peers.end(), ccopy.begin());
    lock.unlock();

    for(auto p: ccopy)
    {
        if(p == except)
        {
            continue;
        }
    
        auto view = hdl.data();

        bool blocking = true;
        p->send(view.data(), view.size(), blocking);
    }
}

void Node::remove_peer(std::shared_ptr<Peer> peer)
{
    std::unique_lock lock(m_peer_mutex);
    for(auto it = m_peers.begin(); it != m_peers.end(); ++it)
    {
        if(*it == peer)
        {
            m_peers.erase(it);
            return;
        }
    }
}

}
