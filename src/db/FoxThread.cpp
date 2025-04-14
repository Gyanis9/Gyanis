#include "db/FoxThread.h"
#include "base/Log.h"
#include "base/Config.h"
#include "base/Macro.h"
#include "base/Utils.h"

namespace Gyanis::db
{
    static auto g_logger = LOG_NAME("system");
    static auto g_thread_info_set
        = base::Config::LookUp("fox_thread",
                               std::unordered_map<std::string, std::unordered_map<std::string, std::string>>()
                               , "config for thread");

    static std::shared_mutex s_thread_mutex;
    static std::unordered_map<uint64_t, std::string> s_thread_names;

    thread_local FoxThread* s_thread = nullptr;


    FoxThread::FoxThread(std::string name, event_base* base)
        : m_read(0)
          , m_write(0)
          , m_base(nullptr)
          , m_event(nullptr)
          , m_thread(nullptr)
          , m_name(std::move(name))
          , m_working(false)
          , m_start(false)
          , m_total(0)
    {
        int fds[2];
        if (evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1)
        {
            LOG_FATAL(g_logger)
                << "FoxThread::FoxThread - evutil_socketpair failed "
                << " | Status: Critical";
            throw std::logic_error("thread init error");
        }

        evutil_make_socket_nonblocking(fds[0]);
        evutil_make_socket_nonblocking(fds[1]);

        m_read = fds[0];
        m_write = fds[1];

        if (base)
        {
            m_base = base;
            setThis();
        }
        else
        {
            m_base = event_base_new();
        }
        m_event = event_new(m_base, m_read, EV_READ | EV_PERSIST, read_cb, this);
        event_add(m_event, nullptr);
    }

    FoxThread::~FoxThread()
    {
        if (m_read)
        {
            close(m_read);
        }
        if (m_write)
        {
            close(m_write);
        }

        stop();

        join();


        delete m_thread;

        if (m_event)
        {
            event_free(m_event);
        }
        if (m_base)
        {
            event_base_free(m_base);
        }
    }

    FoxThread* FoxThread::GetThis()
    {
        return s_thread;
    }

    const std::string& FoxThread::GetFoxThreadName()
    {
        if (FoxThread* thread = GetThis())
        {
            return thread->m_name;
        }

        const uint64_t tid = base::GetThreadID();
        do
        {
            std::shared_lock lock(s_thread_mutex);
            if (const auto it = s_thread_names.find(tid); it != s_thread_names.end())
            {
                return it->second;
            }
        }
        while (false);
        do
        {
            std::unique_lock lock(s_thread_mutex);
            s_thread_names[tid] = "UNNAME_" + std::to_string(tid);
            return s_thread_names[tid];
        }
        while (false);
    }

    void FoxThread::GetAllFoxThreadName(std::unordered_map<uint64_t, std::string>& names)
    {
        std::shared_lock lock(s_thread_mutex);
        for (auto& s_thread_name : s_thread_names)
        {
            names.insert(s_thread_name);
        }
    }

    void FoxThread::setThis()
    {
        m_name = m_name + "_" + std::to_string(base::GetThreadID());
        s_thread = this;

        std::unique_lock lock(s_thread_mutex);
        s_thread_names[base::GetThreadID()] = m_name;
    }

    void FoxThread::unsetThis()
    {
        s_thread = nullptr;
        std::unique_lock lock(s_thread_mutex);
        s_thread_names.erase(base::GetThreadID());
    }

    void FoxThread::start()
    {
        if (m_thread)
        {
            LOG_FATAL(g_logger)
                << "FoxThread::start - FoxThread is running. "
                << " | Status: Critical";
            throw std::logic_error("FoxThread is running");
        }

        m_thread = new std::thread([this] { thread_cb(); });
        m_start = true;
    }

    bool FoxThread::dispatch(const callback& cb)
    {
        std::unique_lock lock(m_mutex);
        m_callbacks.push_back(cb);
        lock.unlock();
        constexpr uint8_t cmd = 1;
        if (send(m_write, &cmd, sizeof(cmd), 0) <= 0)
        {
            return false;
        }
        return true;
    }

    bool FoxThread::dispatch(uint32_t id, const callback& cb)
    {
        return dispatch(cb);
    }

    bool FoxThread::batchDispatch(const std::vector<callback>& cbs)
    {
        std::unique_lock lock(m_mutex);
        for (auto& i : cbs)
        {
            m_callbacks.push_back(i);
        }
        lock.unlock();
        constexpr uint8_t cmd = 1;
        if (send(m_write, &cmd, sizeof(cmd), 0) <= 0)
        {
            return false;
        }
        return true;
    }

    void FoxThread::broadcast(const callback& cb)
    {
        dispatch(cb);
    }

    void FoxThread::join()
    {
        if (m_thread)
        {
            m_thread->join();
            delete m_thread;
            m_thread = nullptr;
        }
    }

    void FoxThread::stop()
    {
        std::unique_lock lock(m_mutex);
        m_callbacks.emplace_back(nullptr);
        if (m_thread)
        {
            constexpr uint8_t cmd = 0;
            send(m_write, &cmd, sizeof(cmd), 0);
        }
    }

    bool FoxThread::isStart() const
    {
        return m_start;
    }

    event_base* FoxThread::getBase() const
    {
        return m_base;
    }

    std::thread::id FoxThread::getId() const
    {
        if (m_thread)
        {
            return m_thread->get_id();
        }
        return {};
    }

    void* FoxThread::getData(const std::string& name)
    {
        const auto it = m_datas.find(name);
        return it == m_datas.end() ? nullptr : it->second;
    }

    void FoxThread::setData(const std::string& name, void* value)
    {
        m_datas[name] = value;
    }

    void FoxThread::setInitCb(const init_cb& value)
    {
        m_initCb = value;
    }

    void FoxThread::dump(std::ostream& os)
    {
        std::shared_lock lock(m_mutex);
        os << "[Thread Name: " << m_name
            << ", Working Status: " << (m_working ? "Active" : "Idle")
            << ", Pending Tasks: " << m_callbacks.size()
            << ", Total Tasks Completed: " << m_total
            << "]" << std::endl;
    }

    uint64_t FoxThread::getTotal()
    {
        return m_total;
    }

    void FoxThread::thread_cb()
    {
        setThis();
        pthread_setname_np(pthread_self(), m_name.substr(0, 15).c_str());
        if (m_initCb)
        {
            m_initCb(this);
            m_initCb = nullptr;
        }
        event_base_loop(m_base, 0);
    }

    void FoxThread::read_cb(const int sock, short which, void* args)
    {
        const auto thread = static_cast<FoxThread*>(args);
        uint8_t cmd[4096];
        if (recv(sock, cmd, sizeof(cmd), 0) > 0)
        {
            std::list<callback> callbacks;
            std::unique_lock lock(thread->m_mutex);
            callbacks.swap(thread->m_callbacks);
            lock.unlock();
            thread->m_working = true;
            for (auto& callback : callbacks)
            {
                if (callback)
                {
                    try
                    {
                        callback();
                    }
                    catch (std::exception& ex)
                    {
                        LOG_ERROR(g_logger) << "exception:" << ex.what();
                    } catch (const char* c)
                    {
                        LOG_ERROR(g_logger) << "exception:" << c;
                    } catch (...)
                    {
                        LOG_ERROR(g_logger) << "uncatch exception";
                    }
                }
                else
                {
                    event_base_loopbreak(thread->m_base);
                    thread->m_start = false;
                    unsetThis();
                    break;
                }
            }
            base::Atomic::addFetch(thread->m_total, callbacks.size());
            thread->m_working = false;
        }
    }

    FoxThreadPool::FoxThreadPool(const uint32_t size, const std::string& name, const bool advance)
        : m_size(size)
          , m_cur(0)
          , m_name(name)
          , m_advance(advance)
          , m_start(false)
          , m_total(0)
    {
        m_threads.resize(m_size);
        for (size_t i = 0; i < size; ++i)
        {
            const auto thread(new FoxThread(name + "_" + std::to_string(i)));
            m_threads[i] = thread;
        }
    }

    FoxThreadPool::~FoxThreadPool()
    {
        for (size_t i = 0; i < m_size; ++i)
        {
            delete m_threads[i];
        }
    }

    void FoxThreadPool::start()
    {
        for (size_t i = 0; i < m_size; ++i)
        {
            m_threads[i]->setInitCb(m_initCb);
            m_threads[i]->start();
            m_freeFoxThreads.push_back(m_threads[i]);
        }
        if (m_initCb)
        {
            m_initCb = nullptr;
        }
        m_start = true;
        check();
    }

    void FoxThreadPool::stop()
    {
        for (size_t i = 0; i < m_size; ++i)
        {
            m_threads[i]->stop();
        }
        m_start = false;
    }

    void FoxThreadPool::join()
    {
        for (size_t i = 0; i < m_size; ++i)
        {
            m_threads[i]->join();
        }
    }

    bool FoxThreadPool::dispatch(const callback& cb)
    {
        do
        {
            base::Atomic::addFetch(m_total, static_cast<uint64_t>(1));
            std::unique_lock lock(m_mutex);
            if (!m_advance)
            {
                return m_threads[m_cur++ % m_size]->dispatch(cb);
            }
            m_callbacks.push_back(cb);
        }
        while (false);
        check();
        return true;
    }

    bool FoxThreadPool::dispatch(const uint32_t id, const callback& cb)
    {
        base::Atomic::addFetch(m_total, static_cast<uint64_t>(1));
        return m_threads[id % m_size]->dispatch(cb);
    }

    bool FoxThreadPool::batchDispatch(const std::vector<callback>& cbs)
    {
        base::Atomic::addFetch(m_total, cbs.size());
        std::unique_lock lock(m_mutex);
        if (!m_advance)
        {
            for (const auto& cb : cbs)
            {
                m_threads[m_cur++ % m_size]->dispatch(cb);
            }
            return true;
        }
        for (const auto& cb : cbs)
        {
            m_callbacks.push_back(cb);
        }
        lock.unlock();
        check();
        return true;
    }

    FoxThread* FoxThreadPool::getRandFoxThread()
    {
        return m_threads[m_cur++ % m_size];
    }

    void FoxThreadPool::setInitCb(const FoxThread::init_cb& value)
    {
        m_initCb = value;
    }

    void FoxThreadPool::dump(std::ostream& os)
    {
        std::shared_lock lock(m_mutex);
        os << "[FoxThreadPool Status: "
            << "Name=\"" << m_name << "\", "
            << "Thread Count=" << m_threads.size() << ", "
            << "Pending Tasks=" << m_callbacks.size() << ", "
            << "Total Tasks=" << m_total << ", "
            << "Advanced Mode=" << (m_advance ? "Enabled" : "Disabled")
            << "]" << std::endl;
        for (const auto& m_thread : m_threads)
        {
            os << "    ";
            m_thread->dump(os);
        }
    }

    void FoxThreadPool::broadcast(const callback& cb)
    {
        for (const auto& m_thread : m_threads)
        {
            m_thread->dispatch(cb);
        }
    }

    uint64_t FoxThreadPool::getTotal()
    {
        return m_total;
    }

    void FoxThreadPool::releaseFoxThread(FoxThread* thread)
    {
        do
        {
            std::unique_lock lock(m_mutex);
            m_freeFoxThreads.push_back(thread);
        }
        while (false);
        check();
    }

    void FoxThreadPool::check()
    {
        do
        {
            if (!m_start)
            {
                break;
            }
            std::unique_lock lock(m_mutex);
            if (m_freeFoxThreads.empty() || m_callbacks.empty())
            {
                break;
            }

            std::shared_ptr<FoxThread> thr(m_freeFoxThreads.front(),
                                           [this](auto&& PH1) { releaseFoxThread(std::forward<decltype(PH1)>(PH1)); });
            m_freeFoxThreads.pop_front();

            callback cb = m_callbacks.front();
            m_callbacks.pop_front();
            lock.unlock();

            if (thr->isStart())
            {
                thr->dispatch([this, thr, cb]
                {
                    wrapcb(thr, cb);
                });
            }
            else
            {
                std::unique_lock lock1(m_mutex);
                m_callbacks.push_front(cb);
            }
        }
        while (true);
    }

    void FoxThreadPool::wrapcb(const std::shared_ptr<FoxThread>&, const callback& cb)
    {
        cb();
    }

    void FoxThreadManager::dispatch(const std::string& name, const callback& cb)
    {
        const auto result = get(name);
        ASSERT(result);
        result->dispatch(cb);
    }

    void FoxThreadManager::dispatch(const std::string& name, uint32_t id, const callback& cb)
    {
        const auto result = get(name);
        ASSERT(result);
        result->dispatch(id, cb);
    }

    void FoxThreadManager::batchDispatch(const std::string& name, const std::vector<callback>& cbs)
    {
        const auto ti = get(name);
        ASSERT(ti);
        ti->batchDispatch(cbs);
    }

    void FoxThreadManager::broadcast(const std::string& name, const callback& cb)
    {
        const auto ti = get(name);
        ASSERT(ti);
        ti->broadcast(cb);
    }

    void FoxThreadManager::dumpFoxThreadStatus(std::ostream& os)
    {
        os << "FoxThreadManager: " << std::endl;
        for (const auto& [fst, snd] : m_threads)
        {
            snd->dump(os);
        }

        os << "All FoxThreads:" << std::endl;
        std::unordered_map<uint64_t, std::string> names;
        FoxThread::GetAllFoxThreadName(names);
        for (auto& [fst, snd] : names)
        {
            os << std::setw(30) << fst
                << ": " << snd << std::endl;
        }
    }

    void FoxThreadManager::init()
    {
        const auto m = g_thread_info_set->getValue();
        for (const auto& [fst, snd] : m)
        {
            auto num = base::GetParamValue(snd, "num", 0);
            auto name = fst;
            auto advance = base::GetParamValue(snd, "advance", 0);
            if (num <= 0)
            {
                LOG_ERROR(g_logger)
                    << "[ThreadPool Error] "
                    << "Name: " << name
                    << ", Number of Threads: " << num
                    << ", Advanced Mode: " << (advance ? "Enabled" : "Disabled")
                    << " | Status: Invalid";

                continue;
            }
            if (num == 1)
            {
                m_threads[fst] = std::make_shared<FoxThread>(fst);
                LOG_INFO(g_logger) << "[Thread Initialization] Thread: " << fst;
            }
            else
            {
                m_threads[fst] = std::make_shared<FoxThreadPool>(num, name, advance);

                LOG_INFO(g_logger)
                    << "[ThreadPool Initialization] "
                    << "Name: " << name
                    << ", Number of Threads: " << num
                    << ", Advanced Mode: " << (advance ? "Enabled" : "Disabled")
                    << " | Status: Normal";
            }
        }
    }

    void FoxThreadManager::start() const
    {
        for (const auto& [fst, snd] : m_threads)
        {
            LOG_INFO(g_logger) << "[Thread Start] Thread: " << fst << " is starting.";
            snd->start();
            LOG_INFO(g_logger) << "[Thread Start Complete] Thread: " << fst << " has successfully started.";
        }
    }

    void FoxThreadManager::stop() const
    {
        for (const auto& [fst, snd] : m_threads)
        {
            LOG_INFO(g_logger) << "[Thread Stop Start] Thread: " << fst << " is beginning to stop.";

            snd->stop();
            LOG_INFO(g_logger) << "[Thread Stop Complete] Thread: " << fst << " has successfully stopped.";
        }
        for (const auto& [fst, snd] : m_threads)
        {
            LOG_INFO(g_logger) << "[Thread Join Start] Thread: " << fst << " is beginning the join process.";
            snd->join();
            LOG_INFO(g_logger) << "[Thread Join Complete] Thread: " << fst << " has successfully joined.";
        }
    }

    std::shared_ptr<IFoxThread> FoxThreadManager::get(const std::string& name)
    {
        const auto it = m_threads.find(name);
        return it == m_threads.end() ? nullptr : it->second;
    }

    void FoxThreadManager::add(const std::string& name, const std::shared_ptr<IFoxThread>& thread)
    {
        m_threads[name] = thread;
    }
}
