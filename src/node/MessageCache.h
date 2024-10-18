#pragma once

#include <bitstream.h>
#include <shared_mutex>

namespace relay {

class MessageCache {
  public:
    bitstream insert(bitstream &&msg) {
        std::unique_lock lock(m_mutex);
        auto &bs = m_messages.emplace_back(std::move(msg));
        return bs.make_view();
    }

    std::vector<bitstream> get_all() {
        std::shared_lock lock(m_mutex);

        std::vector<bitstream> result;
        result.reserve(m_messages.size());

        for (auto &msg : m_messages) {
            result.emplace_back(msg.make_view());
        }

        return result;
    }

  private:
    std::shared_mutex m_mutex;
    std::list<bitstream> m_messages;
};

} // namespace relay
