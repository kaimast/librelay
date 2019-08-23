#pragma once

#include <set>
#include "librelay/Connection.h"
#include <yael/network/Address.h>

namespace relay
{

[[nodiscard]]
std::shared_ptr<Connection> create_connection(const yael::network::Address &address, Callback &callback, std::set<channel_id_t> subscriptions);

}
