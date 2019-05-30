#include "node/sighandler.h"
#include "node/Node.h"

#include <boost/program_options.hpp>
#include <glog/logging.h>

namespace po = boost::program_options;
using namespace relay; 

int main(int ac, char* av[])
{
    srand(::getpid() ^ ::time(nullptr));

    setup_sighandlers();

    google::SetLogDestination(google::GLOG_INFO, "relay-node.log.");
    google::InitGoogleLogging(av[0]);

    FLAGS_logbufsecs = 0;
    FLAGS_logbuflevel = google::GLOG_INFO;
    FLAGS_alsologtostderr = true;

    po::positional_options_description p;
    p.add("addr", 1);
    p.add("peer_addr", 1);
    p.add("config", 1);
    
    po::options_description desc("Allowed options");
    desc.add_options()
        ("addr", po::value<std::string>()->required(), "address of this edge node to connect to")
        ("peer_addr", po::value<std::string>()->required(), "address of the peer to connect to")
        ("config", po::value<std::string>()->required(), "the string of all peers to connect to")
        ("wait", po::value<uint32_t>()->default_value(1), "how long to wait before connecting to other peers")
    ;
 
    po::variables_map vm;
    po::store(po::command_line_parser(ac, av).options(desc).positional(p).run(), vm);
    po::notify(vm);

    yael::EventLoop::initialize();
    auto &el = yael::EventLoop::get_instance();

    auto node = el.make_event_listener<Node>(vm["addr"].as<std::string>(), vm["peer_addr"].as<std::string>(), vm["config"].as<std::string>(), vm["wait"].as<uint32_t>());
    
    el.wait();
 
    return 0;
}
