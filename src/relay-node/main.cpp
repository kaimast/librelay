#include "node/Node.h"
#include "node/sighandler.h"

#include <boost/program_options.hpp>
#include <glog/logging.h>

namespace po = boost::program_options;
using namespace relay;

int main(int ac, char *av[]) {
    srand(::getpid() ^ ::time(nullptr));

    setup_sighandlers();

    google::SetLogDestination(google::GLOG_INFO, "relay-node.log.");
    google::InitGoogleLogging(av[0]);

    FLAGS_logbufsecs = 0;
    FLAGS_logbuflevel = google::GLOG_INFO;
    FLAGS_alsologtostderr = true;

    po::positional_options_description p;
    p.add("name", 1);
    p.add("config", 1);

    po::options_description desc("Allowed options");
    desc.add_options()("help", "show this help message")(
        "name", po::value<std::string>()->required(),
        "name of this node")("config", po::value<std::string>()->required(),
                             "the string of all peers to connect to")(
        "wait", po::value<uint32_t>()->default_value(1),
        "how long to wait before connecting to other peers");

    po::variables_map vm;

    try {
        po::store(
            po::command_line_parser(ac, av).options(desc).positional(p).run(),
            vm);
        po::notify(vm);
    } catch (boost::wrapexcept<boost::program_options::unknown_option> &err) {
        std::cerr << "ERROR: " << err.what() << std::endl;
        std::cout << desc << std::endl;
        return 1;
    } catch (boost::wrapexcept<boost::program_options::required_option> &err) {
        std::cerr << "ERROR: " << err.what() << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }

    yael::EventLoop::initialize();
    auto &el = yael::EventLoop::get_instance();

    auto node = el.make_event_listener<Node>(vm["name"].as<std::string>(),
                                             vm["config"].as<std::string>(),
                                             vm["wait"].as<uint32_t>());

    el.wait();

    return 0;
}
