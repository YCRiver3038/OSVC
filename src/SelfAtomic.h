#ifndef SELF_ATOMIC_H_INCLUDED 
#define SELF_ATOMIC_H_INCLUDED

#include <mutex>

template <typename T>
class SelfAtomic {
    private:
        T m_value;
        std::mutex m_lock;

    public:
        SelfAtomic() {
            m_value = (T)0;
        }
        void store(const T& value) {
            std::lock_guard<std::mutex> storeLock(m_lock);
            m_value = value;
        }

        T& load() {
            std::lock_guard<std::mutex> storeLock(m_lock);
            return m_value;
        }

        operator T() {
            return m_value;
        }

        void operator=(T& value) {
            store(value);
        }

};

class SelfAtomicBool {
    private:
        bool m_value;
        std::mutex m_lock;

    public:
        SelfAtomicBool() {
            m_value = false;
        }
        void store(const bool value) {
            std::lock_guard<std::mutex> storeLock(m_lock);
            m_value = value;
        }

        const bool load() {
            std::lock_guard<std::mutex> storeLock(m_lock);
            return m_value;
        }

        operator bool() {
            return m_value;
        }

        void operator=(bool value) {
            store(value);
        }

};

#endif