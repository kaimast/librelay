#pragma once

#include "librelay/Connection.h"
#include <yael/network/Address.h>

namespace relay
{

std::shared_ptr<Connection> create_connection(const yael::network::Address &address, Callback &callback);

}
