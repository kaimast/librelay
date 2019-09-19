#pragma once

#include <shared_mutex>
#include <atomic>
#include <string>
#include <list>
#include <tuple>
#include <thread>
#include <condition_variable>
#include <unordered_map>
#include <filesystem>
#include <glog/logging.h>
#include <bitstream.h>

#include "librelay/Connection.h"

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

        entry_t(std::set<channel_id_t> channels_, bitstream data_)
            : channels(std::move(channels_)), data(std::move(data_)), usage_count(0)
        {}

        const std::set<channel_id_t> channels;
        const bitstream data;

        std::atomic<uint32_t> usage_count;

        std::list<data_map_t::iterator>::iterator lru_it;
        
        size_t disk_size() const
        {
            return mem_size();
        }

        /// Total amount of memory used by this file
        size_t mem_size() const
        {
            return data.size() + sizeof(channel_id_t)*channels.size()
                + 2*sizeof(size_t);
        }

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
            discard();
            
            m_entry = other.m_entry;
            other.m_entry = nullptr;
        }

        ~entry_handle_t()
        {
            discard();
        }

        const std::set<channel_id_t> channels() const
        {
            if(m_entry == nullptr)
            {
                LOG(FATAL) << "Invalid state";
            }

            return m_entry->channels;
        }

        void discard()
        {
            if(m_entry == nullptr)
            {
                // nothing to do
                return;
            }

            auto prev = m_entry->usage_count.fetch_sub(1);

            if(prev == 0)
            {
                LOG(FATAL) << "Invalid state";
            }

            m_entry = nullptr;
        }

        bitstream operator*() const
        {
            return data();
        }

        /// Returns a read-only view of the underlying data
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
    ~Storage();

    entry_handle_t insert(std::set<channel_id_t> channels, bitstream value);

    std::optional<entry_handle_t> get_entry(size_t pos);

    size_t num_entries() const { return m_num_entries; }

    iterator_t iterate()
    {
        return iterator_t(*this, m_num_entries);
    }

private:
    static constexpr size_t NUM_SHARDS = 10;

    void write_worker_loop();
 
    struct shard_t
    {
        std::mutex mutex;
        std::mutex file_mutex;

        std::condition_variable_any entry_cond;

        size_t current_mem_size = 0;

        data_map_t data;
        std::list<data_map_t::iterator> lru;

        void make_space(size_t max_mem_size);
        std::unique_ptr<entry_t> get_entry_from_disk(std::filesystem::path path, size_t offset);

        size_t storage_pos;
    };

    std::atomic<bool> m_okay = true;
    std::thread m_write_thread;

    std::mutex m_write_queue_mutex;
    std::condition_variable m_write_queue_cond;
    std::list<std::pair<size_t, entry_handle_t>> m_write_queue;

    std::atomic<size_t> m_num_entries = 0;

    const std::filesystem::path m_prefix;
    const size_t m_max_mem_size;

    std::array<shard_t, NUM_SHARDS> m_data_shards;

    size_t to_shard(const size_t key)
    {
        return key % NUM_SHARDS;
    }
};

}
