#pragma once

#ifndef _PNG_STREAM_H
#define _PNG_STREAM_H

#include "png/base.h"

#include <istream>
#include <mutex>

namespace PNG
{
    class IStream
    {
    public:
        /**
         * @brief Reads `bufLen` bytes into `buf`.
         * @param buf The buffer to read into.
         * @param bufLen The length of `buf` in bytes.
         * @param bytesRead
         * A pointer to a variable which will hold the number of bytes read.
         * If `nullptr`, the function should return `PNG::Result::UnexpectedEOF` if it read less then `bufLen` bytes.
         * Otherwise, the function will try to read at least one byte into the buffer, returning `PNG::Result::UnexpectedEOF` if no data is to be expected.
         * @return `Result::OK` if no error was found (see `bytesRead` for more details).
         */
        virtual Result ReadBuffer(void* buf, size_t bufLen, size_t* bytesRead = nullptr) = 0;

        /// @see PNG::IStream::ReadBuffer()
        virtual Result ReadVector(std::vector<uint8_t>& vec, size_t* bytesRead = nullptr) { return ReadBuffer(vec.data(), vec.size(), bytesRead); }

        template<typename T>
        Result ReadNumber(T& out)
        {
            out = 0;
            uint8_t buf[1];
            for (size_t i = 0; i < sizeof(T); i++) {
                PNG_RETURN_IF_NOT_OK(ReadBuffer, buf, 1, nullptr);
                out = (out << 8) | buf[0];
            }
            return Result::OK;
        }

        virtual Result ReadU8(uint8_t& out) { return ReadBuffer(&out, 1, nullptr); }
        virtual Result ReadU32(uint32_t& out) { return ReadNumber<uint32_t>(out); }
        virtual Result ReadU64(uint64_t& out) { return ReadNumber<uint64_t>(out); }
    };

    class OStream
    {
    public:
        /**
         * @brief
         * Writes some data to the stream.
         * Calling `OStream::Flush` will tell the stream to send the data to the buffer it is attached to.
         * @param buf The buffer which holds the data to be written.
         * @param bufLen The length of `buf` in bytes.
         * @return Whether the write operation succeded or not.  
         */
        virtual Result WriteBuffer(const void* buf, size_t bufLen) = 0;
        
        /// @see PNG::OStream::WriteBuffer()
        virtual Result WriteVector(const std::vector<uint8_t>& vec) { return WriteBuffer(vec.data(), vec.size()); }

        /**
         * @brief Tells the stream that what was previously written to it can be moved from the staging buffer into the underlying buffer.
         * @return Whether the call succeded or not. 
         */
        virtual Result Flush() = 0;

        template<typename T>
        Result WriteNumber(T in)
        {
            uint8_t buf[1];
            for (size_t i = 0; i < sizeof(T); i++) {
                buf[0] = in >> (8 * (sizeof(T) - i - 1));
                PNG_RETURN_IF_NOT_OK(WriteBuffer, buf, 1);
            }
            return Result::OK;
        }

        virtual Result WriteU8(uint8_t out) { return WriteBuffer(&out, 1); }
        virtual Result WriteU32(uint32_t out) { return WriteNumber<uint32_t>(out); }
        virtual Result WriteU64(uint64_t out) { return WriteNumber<uint64_t>(out); }
    };

    class IOStream : public IStream, public OStream
    {
    public:
        /// @see PNG::IStream::ReadBuffer()
        virtual Result ReadBuffer(void* buf, size_t bufLen, size_t* bytesRead = nullptr) = 0;
        /// @see PNG::OStream::WriteBuffer()
        virtual Result WriteBuffer(const void* buf, size_t bufLen) = 0;
        /// @see PNG::OStream::Flush()
        virtual Result Flush() = 0;
    };

    class IStreamWrapper : public IStream
    {
    public:
        IStreamWrapper(std::istream& in)
            : m_Stream(in) { }

        /// @see PNG::IStream::ReadBuffer()
        virtual Result ReadBuffer(void* buf, size_t bufLen, size_t* bytesRead = nullptr) override;
    private:
        std::istream& m_Stream;

    };

    class OStreamWrapper : public OStream
    {
    public:
        OStreamWrapper(std::ostream& out)
            : m_Stream(out) { }

        /// @see PNG::OStream::WriteBuffer()
        virtual Result WriteBuffer(const void* buf, size_t bufLen) override;
        /// @see PNG::OStream::Flush()
        virtual Result Flush() override;
    private:
        std::ostream& m_Stream;

    };

    class ByteStream : public IStream
    {
    public:
        ByteStream(const void* buf, size_t bufLen)
            : m_Buffer((char*)buf), m_BufferLen(bufLen) { }

        ByteStream(const std::vector<uint8_t>& vec)
            : ByteStream(vec.data(), vec.size()) { }
        
        /// @see PNG::IStream::ReadBuffer()
        virtual Result ReadBuffer(void* buf, size_t bufLen, size_t* bytesRead = nullptr) override;

    protected:
        char* m_Buffer;
        size_t m_BufferLen;
        size_t m_ReadCursor = 0;
        std::mutex m_Mutex;
    };

    class DynamicByteStream : public IOStream
    {
    public:
        DynamicByteStream(std::chrono::milliseconds pollInterval)
            : m_IPollInterval(pollInterval) { }

        DynamicByteStream()
            : DynamicByteStream(std::chrono::milliseconds(1)) { }

        std::vector<uint8_t>& GetBuffer() { return m_IBuffer; }
        const std::vector<uint8_t>& GetBuffer() const { return m_IBuffer; }

        /// @see PNG::IStream::ReadBuffer()
        virtual Result ReadBuffer(void* buf, size_t bufLen, size_t* bytesRead = nullptr) override;
        /// @see PNG::OStream::WriteBuffer()
        virtual Result WriteBuffer(const void* buf, size_t bufLen) override;
        /// @see PNG::OStream::Flush()
        virtual Result Flush() override;

        bool IsClosed() const { return m_Closed; }
        virtual Result Close();

    protected:
        size_t GetAvailable()
        {
            std::lock_guard<std::mutex> lock(m_IMutex);
            return m_IBuffer.size() - m_ICursor;
        }

        std::vector<uint8_t> m_OBuffer;
        std::mutex m_OMutex;

        std::chrono::milliseconds m_IPollInterval;
        size_t m_ICursor = 0;

        std::vector<uint8_t> m_IBuffer;
        std::mutex m_IMutex;

        bool m_Closed = false;
    };
}

#endif // _PNG_STREAM_H
