#include "librelay/librelay.h"
#include "ConnectionImpl.h"

#include <yael/EventLoop.h>

namespace relay {

std::shared_ptr<Connection>
create_connection(const yael::network::Address &address, Callback &callback,
                  std::set<channel_id_t> subscriptions) {
    auto &el = yael::EventLoop::get_instance();
    auto conn = el.make_event_listener<ConnectionImpl>(address, callback,
                                                       subscriptions);
    return std::dynamic_pointer_cast<Connection>(conn);
}

} // namespace relay
