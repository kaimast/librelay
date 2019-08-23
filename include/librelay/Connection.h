#pragma once

#include <memory>
#include <bitstream.h>

namespace relay
{

using channel_id_t = int16_t;
constexpr channel_id_t ALL_CHANNELS = -1;

class Callback
{
public:
    virtual ~Callback() = default;

    virtual void on_new_message(channel_id_t cid, bitstream &&data) = 0;

    virtual void on_disconnect() = 0;
};

class Connection
{
public:
    virtual ~Connection() = default;

    virtual void send(channel_id_t cid, bitstream &&data, bool blocking) = 0;

    virtual void close() = 0;
};

}
