#pragma once

#include <shared_mutex>
#include <atomic>
#include <string>
#include <list>
#include <tuple>
#include <unordered_map>
#include <filesystem>
#include <glog/logging.h>
#include <bitstream.h>

namespace relay
{

// Very simple append only storage
class Storage
{
private:
    struct entry_t;
    using data_map_t = std::unordered_map<size_t, std::pair<size_t, std::unique_ptr<entry_t>>>;
    
    struct entry_t
    {
        friend class entry_handle_t;

        entry_t(bitstream data_)
            : data(std::move(data_)), usage_count(0)
        {}

        bitstream data;

        std::atomic<uint32_t> usage_count;

        std::list<data_map_t::iterator>::iterator lru_it;
    };

public:
    class entry_handle_t
    {
    public:
        entry_handle_t()
            : m_entry(nullptr)
        {}

        entry_handle_t(entry_t &entry)
            : m_entry(&entry)
        {
            m_entry->usage_count++;
        }

        entry_handle_t(entry_handle_t &&other)
            : m_entry(other.m_entry)
        {
            other.m_entry = nullptr;
        }
        
        void operator=(entry_handle_t &&other)
        {
            if(m_entry != nullptr)
            {
                LOG(FATAL) << "invalid state";
            }

            m_entry = other.m_entry;
            other.m_entry = nullptr;
        }

        ~entry_handle_t()
        {
            if(m_entry)
            {
                auto prev = m_entry->usage_count.fetch_sub(1);

                if(prev == 0)
                {
                    LOG(FATAL) << "Invalid state";
                }
            }
        }

        bitstream operator*() const
        {
            return data();
        }

        bitstream data() const
        {
            if(!m_entry)
            {
                LOG(FATAL) << "Cannot get data: invalid entry handle";
            }

            return m_entry->data.make_view();
        }

    private:
        entry_t *m_entry;
    };

    class iterator_t
    {
    public:
        iterator_t(Storage &storage, size_t end)
            : m_storage(storage), m_position(0), m_end(end)
        {}

        std::optional<entry_handle_t> next()
        {
            std::optional<entry_handle_t> hdl = std::nullopt;
           
            while(!hdl && m_position < m_end)
            {
                hdl = m_storage.get_entry(m_position);
                m_position++;
            }

            return hdl;
        }

        bool at_end() const
        {
            return m_end == m_position;
        }

    private:
        Storage &m_storage;
        size_t m_position;
        size_t m_end;
    };

    Storage(const std::string &prefix, size_t mem_size);

    entry_handle_t insert(bitstream value);

    std::optional<entry_handle_t> get_entry(size_t pos);

    size_t num_entries() const { return m_num_entries; }

    iterator_t iterate()
    {
        return iterator_t(*this, m_num_entries);
    }

private:
    static constexpr size_t NUM_SHARDS = 10;

    struct shard_t
    {
        std::mutex mutex;
        std::condition_variable_any entry_cond;

        size_t current_mem_size = 0;

        data_map_t data;
        std::list<data_map_t::iterator> lru;

        void make_space(size_t max_mem_size);
        std::unique_ptr<entry_t> get_entry_from_disk(std::filesystem::path path, size_t offset);

        size_t storage_pos;
    };

    std::atomic<size_t> m_num_entries = 0;

    const std::filesystem::path m_prefix;
    const size_t m_max_mem_size;

    std::array<shard_t, NUM_SHARDS> m_data;

    size_t to_shard(const size_t key)
    {
        return key % NUM_SHARDS;
    }
};

}
