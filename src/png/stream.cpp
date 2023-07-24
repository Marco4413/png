#include "png/stream.h"

#include <thread>

PNG::Result PNG::IStreamWrapper::ReadBuffer(void* buf, size_t bufLen, size_t* bytesRead)
{
    if (bytesRead)
        *bytesRead = m_Stream.readsome((char*)buf, bufLen);
    else m_Stream.read((char*)buf, bufLen);

    if (!m_Stream) {
        if (m_Stream.eof())
            return Result::UnexpectedEOF;
        return Result::Unknown;
    }

    return Result::OK;
}

PNG::Result PNG::ByteStream::ReadBuffer(void* buf, size_t bufLen, size_t* bytesRead)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    size_t avail = m_BufferLen - m_ReadCursor;
    if (avail < bufLen) {
        memcpy(buf, m_Buffer + m_ReadCursor, avail);
        m_ReadCursor = m_BufferLen;
        if (!bytesRead)
            return Result::UnexpectedEOF;
        *bytesRead = avail;
        return PNG::Result::OK;
    }

    memcpy(buf, m_Buffer + m_ReadCursor, bufLen);
    m_ReadCursor += bufLen;
    if (bytesRead)
        *bytesRead = bufLen;

    return PNG::Result::OK;
}

PNG::Result PNG::DynamicByteStream::ReadBuffer(void* buf, size_t bufLen, size_t* bytesRead)
{
    {
        m_IMutex.lock();
        size_t avail = m_IBuffer.size() - m_ICursor;
        m_IMutex.unlock();
        if (avail < bufLen)
            std::this_thread::sleep_for(m_ITimeout);
    }

    std::lock_guard<std::mutex> lock(m_IMutex);

    size_t avail = m_IBuffer.size() - m_ICursor;
    if (avail < bufLen) {
        memcpy(buf, m_IBuffer.data() + m_ICursor, avail);
        m_ICursor = m_IBuffer.size();
        if (!bytesRead)
            return Result::UnexpectedEOF;
        *bytesRead = avail;
        return PNG::Result::OK;
    }

    memcpy(buf, m_IBuffer.data() + m_ICursor, bufLen);
    m_ICursor += bufLen;
    if (bytesRead)
        *bytesRead = bufLen;

    return PNG::Result::OK;
}

PNG::Result PNG::DynamicByteStream::WriteBuffer(void* buf, size_t bufLen)
{
    std::lock_guard<std::mutex> lock(m_OMutex);
    size_t cur = m_OBuffer.size();
    m_OBuffer.resize(m_OBuffer.size() + bufLen);
    memcpy(m_OBuffer.data() + cur, buf, bufLen);
    return PNG::Result::OK;
}

PNG::Result PNG::DynamicByteStream::Flush()
{
    std::lock_guard<std::mutex> iLock(m_IMutex);
    std::lock_guard<std::mutex> oLock(m_OMutex);

    size_t writeCursor = m_IBuffer.size();
    m_IBuffer.resize(m_IBuffer.size()+m_OBuffer.size());
    memcpy(m_IBuffer.data()+writeCursor, m_OBuffer.data(), m_OBuffer.size());
    m_OBuffer.resize(0);
    
    return Result::OK;
}
