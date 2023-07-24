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
    // If bytesRead is not nullptr then the caller wants to receive a variable amount of bytes
    if (bytesRead) {
        size_t avail = GetAvailable();
        while (avail == 0) {
            // If the Stream is closed, EOF has been reached
            if (m_Closed)
                return Result::UnexpectedEOF;
            // Otherwise we wait for m_IPollInterval and check again
            std::this_thread::sleep_for(m_IPollInterval);
            avail = GetAvailable();
        }

        if (avail < bufLen) {
            std::lock_guard<std::mutex> lock(m_IMutex);
            memcpy(buf, m_IBuffer.data() + m_ICursor, avail);
            m_ICursor = m_IBuffer.size();
            *bytesRead = avail;
        } else {
            std::lock_guard<std::mutex> lock(m_IMutex);
            memcpy(buf, m_IBuffer.data() + m_ICursor, bufLen);
            m_ICursor += bufLen;
            *bytesRead = bufLen;
        }
        return PNG::Result::OK;
    }

    while (GetAvailable() < bufLen) {
        if (m_Closed)
            return Result::UnexpectedEOF;
        std::this_thread::sleep_for(m_IPollInterval);
    }

    std::lock_guard<std::mutex> lock(m_IMutex);
    memcpy(buf, m_IBuffer.data() + m_ICursor, bufLen);
    m_ICursor += bufLen;

    return PNG::Result::OK;
}

PNG::Result PNG::DynamicByteStream::WriteBuffer(void* buf, size_t bufLen)
{
    if (m_Closed)
        return Result::UpdatingClosedStreamError;

    std::lock_guard<std::mutex> lock(m_OMutex);
    size_t cur = m_OBuffer.size();
    m_OBuffer.resize(m_OBuffer.size() + bufLen);
    memcpy(m_OBuffer.data() + cur, buf, bufLen);
    return PNG::Result::OK;
}

PNG::Result PNG::DynamicByteStream::Flush()
{
    if (m_Closed)
        return Result::UpdatingClosedStreamError;

    std::lock_guard<std::mutex> iLock(m_IMutex);
    std::lock_guard<std::mutex> oLock(m_OMutex);

    size_t writeCursor = m_IBuffer.size();
    m_IBuffer.resize(m_IBuffer.size()+m_OBuffer.size());
    memcpy(m_IBuffer.data()+writeCursor, m_OBuffer.data(), m_OBuffer.size());
    m_OBuffer.resize(0);
    
    return Result::OK;
}

PNG::Result PNG::DynamicByteStream::Close()
{
    PNG_RETURN_IF_NOT_OK(Flush);
    m_Closed = true;
    return Result::OK;
}
