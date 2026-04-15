#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <opencv2/opencv.hpp>

// 图像与识别框的组合数据结构（仅定义一次）
struct ImageWithBoxes {
    cv::Mat image;                  // 图像数据
    std::vector<cv::Rect2d> boxes;  // 识别框集合
};

template <typename T>
class ThreadSafeQueue
{
public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    // 禁止拷贝构造和赋值
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // 入队操作
    void enqueue(const T& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(value);
        m_condition.notify_one();
    }

    // 出队操作（支持超时）
    bool dequeue(T& value, int timeoutMs = -1)
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (timeoutMs < 0) {
            // 无限等待直到队列非空
            m_condition.wait(lock, [this] { return !m_queue.empty(); });
        } else {
            // 超时等待
            if (!m_condition.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                                    [this] { return !m_queue.empty(); })) {
                return false;  // 超时返回false
            }
        }

        value = m_queue.front();
        m_queue.pop();
        return true;
    }

    // 检查队列是否为空
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    // 获取队列大小
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    // 清空队列
    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_queue.empty()) {
            m_queue.pop();
        }
    }

private:
    mutable std::mutex m_mutex;
    std::queue<T> m_queue;
    std::condition_variable m_condition;
};

// 显式实例化模板，避免链接错误
template class ThreadSafeQueue<ImageWithBoxes>;

#endif // THREADSAFEQUEUE_H
