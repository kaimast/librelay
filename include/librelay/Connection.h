#pragma once

#include <memory>
#include <bitstream.h>

namespace relay
{

class Callback
{
public:
    virtual ~Callback() = default;

    virtual void on_new_message(bitstream &&data) = 0;

    virtual void on_disconnect() = 0;
};

class Connection
{
public:
    virtual ~Connection() = default;

    virtual void send(bitstream &&data, bool blocking) = 0;

    virtual void close() = 0;
};

}
