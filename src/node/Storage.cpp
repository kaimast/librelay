#include "node/Storage.h"

namespace relay
{

Storage::Storage(const std::string &prefix, size_t max_mem_size)
   : m_prefix("./" + prefix + ".store"), m_max_mem_size(max_mem_size), m_data_shards{}
{
    // truncate storage files
    //FIXME we currently do not recover from previous state
    for(size_t sid = 0; sid < NUM_SHARDS; ++sid)
    {
        auto path = m_prefix / (std::to_string(sid) + ".dat");

        if(std::filesystem::exists(path))
        {
            DLOG(INFO) << "Removing old storage file " << path;
            std::filesystem::resize_file(path, 0);
        }
    }

    m_write_thread = std::thread(&Storage::write_worker_loop, this);

    try {
        std::filesystem::create_directory(m_prefix);
    } catch(const std::exception &e) {
        LOG(FATAL) << "Failed to created storage folder at " << m_prefix << ": " << e.what();
    }
}

Storage::~Storage()
{
    // stop worker thread
    m_okay = false;
    
    std::unique_lock lock(m_write_queue_mutex);
    m_write_queue_cond.notify_all();
    lock.unlock();

    m_write_thread.join();
}

void Storage::shard_t::make_space(size_t max_mem_size)
{
    auto shard_max_mem_size = max_mem_size / NUM_SHARDS;

    for(auto it = lru.begin(); it != lru.end() && current_mem_size > shard_max_mem_size;)
    {
        auto &val = (*it)->second;
        auto &entry = val.second;

        if(entry->usage_count > 0)
        {
            // can't evict
            ++it;
            continue;
        }

        if(current_mem_size < entry->data.size())
        {
            LOG(FATAL) << "Invalid state!";
        }
        
        current_mem_size -= entry->mem_size();

        // unique ptr will delete for us
        val.second.reset();
        it = lru.erase(it);
    }

    if(current_mem_size > shard_max_mem_size)
    {
        LOG(ERROR) << "Did not find enough to evict from relay storage; might run out of memory";
    }
}

std::unique_ptr<Storage::entry_t>
Storage::shard_t::get_entry_from_disk(std::filesystem::path path, size_t offset)
{
    std::ifstream file(path, std::fstream::in | std::fstream::binary);
    file.seekg(offset);

    std::set<channel_id_t> channels;

    size_t num_channels;
    file.read(reinterpret_cast<char*>(&num_channels), sizeof(num_channels));

    for(size_t i = 0; i < num_channels; ++i)
    {
        channel_id_t id;
        file.read(reinterpret_cast<char*>(&id), sizeof(id));

        channels.insert(id);
    }
 
    size_t data_size;
    file.read(reinterpret_cast<char*>(&data_size), sizeof(data_size));
    bitstream data;
    data.resize(data_size);

    file.read(reinterpret_cast<char*>(data.data()), data_size);

    current_mem_size += data_size;
    return std::make_unique<entry_t>(std::move(channels), std::move(data));
}

std::optional<Storage::entry_handle_t>
Storage::get_entry(size_t pos)
{
    auto sid = to_shard(pos);
    auto &shard = m_data_shards[sid];

    std::unique_lock lock(shard.mutex);

    auto it = shard.data.find(pos);

    if(it == shard.data.end())
    {
        return std::nullopt;
    }

    auto &val = it->second;
    entry_handle_t hdl;

    if(val.second == nullptr)
    {
        auto path = m_prefix / (std::to_string(sid) + ".dat");
        val.second = shard.get_entry_from_disk(path, val.first);

        // increase read count before we evict stuff
        hdl = entry_handle_t{val.second.get()};

        // evict other stuff to make space
        shard.make_space(m_max_mem_size);
    }
    else
    {
        hdl = entry_handle_t{val.second.get()};
        shard.lru.erase(val.second->lru_it);
    }

    shard.lru.push_back(it);
    val.second->lru_it = shard.lru.end();
    val.second->lru_it--;

    return hdl;
}

Storage::entry_handle_t Storage::insert(std::set<channel_id_t> channels, bitstream value)
{
    auto key = m_num_entries.fetch_add(1);

    auto sid = to_shard(key);
    auto &shard = m_data_shards[sid];

    std::unique_lock lock(shard.mutex);

    std::pair<size_t, std::unique_ptr<entry_t>> new_val =
        {shard.storage_pos, std::make_unique<entry_t>(std::move(channels), std::move(value))};

    auto [it, res] = shard.data.emplace(key, std::move(new_val));

    auto &val = it->second;
    auto path = m_prefix / (std::to_string(sid) + ".dat");

    if(!res)
    {
        LOG(FATAL) << "Failed to insert message";
    }
    entry_handle_t hdl{val.second.get()};

    shard.lru.push_back(it);
    val.second->lru_it = shard.lru.end();
    val.second->lru_it--;

    // queue to be written to disk
    {
        shard.current_mem_size += val.second->mem_size();
        shard.storage_pos += val.second->disk_size();
        shard.entry_cond.notify_all();

        std::unique_lock lock(m_write_queue_mutex);

        entry_handle_t hdl{val.second.get()};
        m_write_queue.emplace_back(std::pair{sid, std::move(hdl)});
        m_write_queue_cond.notify_one();
    }

    // evict other stuff?
    shard.make_space(m_max_mem_size);

    return hdl;
}

void Storage::write_worker_loop()
{
    while(m_okay)
    {
        std::unique_lock lock(m_write_queue_mutex);
        while(m_write_queue.empty() && m_okay)
        {
            m_write_queue_cond.wait(lock);
        }

        if(!m_okay)
        {
            // TODO we should make sure everything is written to disk before we terminate
            return;
        }

        auto it = m_write_queue.begin();
        if(it == m_write_queue.end())
        {
            LOG(FATAL) << "Invalid state";
        }

        auto [sid, entry] = std::move(*it);
        m_write_queue.erase(it);
        lock.unlock();

        auto &shard = m_data_shards[sid];

        std::unique_lock file_lock(shard.file_mutex);
        auto path = m_prefix / (std::to_string(sid) + ".dat");
        std::ofstream file(path, std::fstream::app | std::fstream::binary);

        size_t num_channels = entry.channels().size();
        file.write(reinterpret_cast<const char*>(&num_channels), sizeof(num_channels));
 
        for(auto id: entry.channels())
        {
            file.write(reinterpret_cast<const char*>(&id), sizeof(id));
        }

        auto data = entry.data();
        size_t data_size = data.size();

        file.write(reinterpret_cast<const char*>(&data_size), sizeof(data_size));
        file.write(reinterpret_cast<const char*>(data.data()), data_size);

        if(file.bad())
        {
            LOG(FATAL) << "Write to disk failed. Storage full?";
        }
    }
}


}
