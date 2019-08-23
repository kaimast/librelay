#pragma once

#include <memory>
#include <set>
#include <bitstream.h>

namespace relay
{

using channel_id_t = uint16_t;

class Callback
{
public:
    virtual ~Callback() = default;

    virtual void on_new_message(std::set<channel_id_t> channels, bitstream &&data) = 0;

    virtual void on_disconnect() = 0;
};

class Connection
{
public:
    virtual ~Connection() = default;

    virtual void send(std::set<channel_id_t> channels, bitstream &&data, bool blocking) = 0;

    virtual void close() = 0;
};

}
