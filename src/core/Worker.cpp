#include "core/Worker.h"
#include <random>
#include "base/Config.h"

namespace Gyanis::core
{
    static auto g_worker_config
        = base::Config::LookUp("workers",
                               std::unordered_map<std::string, std::unordered_map<std::string, std::string>>(),
                               "worker config");


    WorkerGroup::WorkerGroup(const uint32_t batch_size, Scheduler* s) : m_batchSize(batch_size),
                                                                        m_finish(false),
                                                                        m_scheduler(s),
                                                                        m_sem(batch_size)
    {
    }

    WorkerGroup::~WorkerGroup()
    {
        waitAll();
    }

    void WorkerGroup::scheduler(const std::function<void()>& callback, const int thread)
    {
        m_sem.wait();
        m_scheduler->schedule(std::bind(&WorkerGroup::doWork
                                        , shared_from_this(), callback), thread);
    }

    void WorkerGroup::waitAll()
    {
        if (!m_finish)
        {
            m_finish = true;
            for (uint32_t i = 0; i < m_batchSize; ++i)
            {
                m_sem.wait();
            }
        }
    }

    std::shared_ptr<WorkerGroup> WorkerGroup::Create(uint32_t batch_size, Scheduler* scheduler)
    {
        return std::make_shared<WorkerGroup>(batch_size, scheduler);
    }

    void WorkerGroup::doWork(const std::function<void()>& callback)
    {
        callback();
        m_sem.notify();
    }

    WorkerManager::WorkerManager() : m_stop(false)
    {
    }

    void WorkerManager::add(const std::shared_ptr<Scheduler>& scheduler)
    {
        m_datas[scheduler->getName()].push_back(scheduler);
    }

    std::shared_ptr<Scheduler> WorkerManager::get(const std::string& name)
    {
        const auto it = m_datas.find(name);
        if (it == m_datas.end())
        {
            return nullptr;
        }
        if (it->second.size() == 1)
        {
            return it->second[0];
        }
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dis(0, it->second.size() - 1);
        return it->second[dis(gen)];
    }

    std::shared_ptr<IOManager> WorkerManager::getAsIOManager(const std::string& name)
    {
        return std::dynamic_pointer_cast<IOManager>(get(name));
    }

    bool WorkerManager::init()
    {
        const auto workers = g_worker_config->getValue();
        return init(workers);
    }

    bool WorkerManager::init(const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& value)
    {
        for (const auto& [fst, snd] : value)
        {
            std::string name = fst;
            int32_t thread_num = base::GetParamValue(snd, "thread_num", 1);
            const int32_t worker_num = base::GetParamValue(snd, "worker_num", 1);

            for (int32_t x = 0; x < worker_num; ++x)
            {
                std::shared_ptr<Scheduler> scheduler = nullptr;
                if (!x)
                {
                    scheduler = std::make_shared<IOManager>(thread_num, name);
                }
                else
                {
                    scheduler = std::make_shared<IOManager>(thread_num, name + "-" + std::to_string(x));
                }
                add(scheduler);
            }
        }
        return m_stop = m_datas.empty();
    }

    void WorkerManager::stop()
    {
        if (m_stop)
        {
            return;
        }
        for (auto& [fst, snd] : m_datas)
        {
            for (const auto& n : snd)
            {
                n->schedule([]
                {
                });
                n->stop();
            }
        }
        m_datas.clear();
        m_stop = true;
    }

    bool WorkerManager::isStopped() const
    {
        return m_stop;
    }

    std::ostream& WorkerManager::dump(std::ostream& os)
    {
        for (auto& [fst, snd] : m_datas)
        {
            for (const auto& n : snd)
            {
                n->dump(os) << std::endl;
            }
        }
        return os;
    }

    uint32_t WorkerManager::getCount() const
    {
        return m_datas.size();
    }
}
