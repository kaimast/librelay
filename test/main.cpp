#include "librelay/librelay.h"
#include <boost/program_options.hpp>
#include <glog/logging.h>
#include <yael/EventLoop.h>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
namespace po = boost::program_options;

std::mutex done_mutex;
std::condition_variable done_cond;

bool g_has_sent_data = false;
bool g_has_received_data = false;

size_t g_num_messages;
size_t g_num_clients;

std::mutex g_message_mutex;
std::unordered_map<size_t, size_t> g_messages;

void set_message(int32_t pos)
{
    std::unique_lock lock(g_message_mutex);

    auto it = g_messages.find(pos);

    if(it == g_messages.end())
    {
        g_messages[pos] = 1;
    }
    else
    {
        it->second += 1;
    }

    if(g_messages.size() < g_num_clients)
    {
        return;
    }

    for(auto [pos, count]: g_messages)
    {
        if(count < g_num_messages)
        {
            return;
        }
    }

    std::unique_lock loc(done_mutex);
    g_has_received_data = true;
    done_cond.notify_all();
}

class Callback : public relay::Callback
{
private:
    void on_new_message(std::set<relay::channel_id_t>, bitstream &&data) override
    {
        int32_t pos = 0;
        data >> pos;

        set_message(pos);
    }

    void on_disconnect() override
    {
        DLOG(INFO) << "Got disconnect";
    }
};

int main(int ac, char* av[])
{
    po::positional_options_description p;
    p.add("address", 1);
    p.add("message", 1);
    
    po::options_description desc("Allowed options");
    desc.add_options()
        ("address", po::value<std::string>()->required(), "")
        ("message", po::value<int32_t>()->required(), "")
        ("num_clients", po::value<size_t>()->default_value(5), "")
        ("num_messages", po::value<size_t>()->default_value(10'000), "")
    ;

    po::variables_map vm;
    po::store(po::command_line_parser(ac, av).options(desc).positional(p).run(), vm);
    po::notify(vm);

    yael::EventLoop::initialize();
    auto &el = yael::EventLoop::get_instance();

    auto addr_str = vm["address"].as<std::string>();

    g_num_clients = vm["num_clients"].as<size_t>();
    g_num_messages = vm["num_messages"].as<size_t>();

    auto found = addr_str.find(':');
    auto host = addr_str.substr(0, found);
    auto port = std::atoi(addr_str.substr(found+1, std::string::npos).c_str());

    if(port <= 0 || port > std::numeric_limits<uint16_t>::max())
    {
        LOG(ERROR) << "Not a valid port number: " << port;
    }

    auto msg = vm["message"].as<int32_t>();

    Callback callback;
    auto conn = relay::create_connection( yael::network::resolve_URL(host, port), callback, {});

    // Wait for other clients to start
    std::this_thread::sleep_for(0.5s);

    for(size_t i = 0; i < g_num_messages; ++i)
    {
        set_message(msg);

        bitstream block;
        block << msg;

        bool blocking = false;
        conn->send({7}, std::move(block), blocking);
    }

    g_has_sent_data = true;

    {
        std::unique_lock lock(done_mutex);

        while(!(g_has_sent_data && g_has_received_data))
        {
            done_cond.wait(lock);
        }

        LOG(INFO) << "Done";
        auto &el = yael::EventLoop::get_instance();
        el.stop();
    }

    el.wait();
    return 0;
}
