#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include <QMutex>
#include <QQueue>
#include <QByteArray>

struct FrameNode {
    QByteArray data;
    int64_t pts;
};

class FrameBuffer
{
public:
    explicit FrameBuffer(int maxSize = 3) : m_maxSize(maxSize) {}

    void push(const QByteArray& data, int64_t pts) {
        QMutexLocker locker(&m_mutex);
        if (m_queue.size() >= m_maxSize) {
            m_queue.dequeue();  // 丢弃最旧帧
        }
        FrameNode node;
        node.data = data;
        node.pts = pts;
        m_queue.enqueue(node);
    }

    bool pop(QByteArray& data, int64_t& pts) {
        QMutexLocker locker(&m_mutex);
        if (m_queue.isEmpty()) return false;
        FrameNode node = m_queue.dequeue();
        data = node.data;
        pts = node.pts;
        return true;
    }

    void clear() {
        QMutexLocker locker(&m_mutex);
        m_queue.clear();
    }

    int size() {
        QMutexLocker locker(&m_mutex);
        return m_queue.size();
    }

private:
    QQueue<FrameNode> m_queue;
    QMutex m_mutex;
    int m_maxSize;
};

#endif // FRAME_BUFFER_H
