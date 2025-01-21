#ifndef INC_CPPHTTP_H_
#define INC_CPPHTTP_H_
/*
﻿# License
This software is distributed under two licenses, choose whichever you like.

## MIT License
Copyright (c) 2025 Takuro Sakai

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Public Domain
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/
/**
@file cpphttp.h

USAGE:
  put '#define CPPHTTP_IMPLEMENTATION' before including this file to create the implementation.
*/
#include <cassert>
#include <cstddef>
#include <cstdint>

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX
#    include <windows.h>
#    include <winsock2.h>
#    include <ws2tcpip.h>
#endif

#if defined(__unix__) || defined(__linux__)
#    include <sys/socket.h>
#    include <sys/types.h>
#endif

namespace cpphttp
{
/**
 * @brief A part of url
 */
struct Section
{
    uint32_t size_;
    const char8_t* str_;
};

bool parse_url(
    Section& scheme,
    Section& host,
    Section& port,
    Section& path,
    uint32_t size,
    const char8_t* url);

/**
 * @brief A dynamic byte buffer
 */
class Buffer
{
public:
    Buffer();
    ~Buffer();

    uint32_t capacity() const;
    uint32_t size() const;
    void clear();
    void reserve(uint32_t x);
    void resize(uint32_t x);
    void pop_front(uint32_t x);
    void push_back(uint32_t s, const uint8_t* items);
    void push_str(const uint8_t* str);

    const uint8_t& operator[](uint32_t x) const;
    uint8_t& operator[](uint32_t x);

    const uint8_t* begin() const;
    uint8_t* begin();
    const uint8_t* end() const;
    uint8_t* end();

private:
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    void expand(uint32_t x);
    uint32_t capacity_;
    uint32_t size_;
    uint8_t* items_;
};

#ifdef _WIN32
/**
 * @brief A socket wrapper
 */
class Socket
{
public:
    enum class SD
    {
        Receive = 0,
        Send = 1,
        Both = 2,
    };
    Socket();
    ~Socket();
    bool open(const char* node, const char* port);
    bool connect();
    void shutdown(SD sd = SD::Both);
    void close();
    int32_t send(int32_t size, const uint8_t* data);
    int32_t recieve(Buffer& buff);

private:
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    int32_t get_error();
    SOCKET s_;
    addrinfo address_;
};
#endif

class Http
{
public:
    inline static constexpr uint32_t MaxHostSize = 64;
    inline static constexpr uint32_t MaxPortSize = 16;
    inline static constexpr uint32_t MaxPathSize = 128;
    enum class Encoding
    {
        None,
        GZip,
        Compress,
        Deflate,
        Br,
    };

    Http();
    ~Http();
    bool open(const char8_t* url);
    void close();

    bool get(Buffer& result, const char8_t* content_type = nullptr);
    bool post(Buffer& result, uint32_t size, const char8_t* data, const char8_t* content_type = nullptr);

private:
    Http(const Http&) = delete;
    Http& operator=(const Http&) = delete;
    void set_header(Buffer& header, bool get, uint32_t size, const char8_t* content_type);
    const char8_t* parse(int32_t& status, int32_t& length, Encoding& encoding, const char8_t* begin, const char8_t* end);
    Socket socket_;
    char host_[MaxHostSize];
    char port_[MaxPortSize];
    char path_[MaxPathSize];
};
} // namespace cpphttp
#endif // INC_CPPHTTP_H_

#ifdef CPPHTTP_IMPLEMENTATION
#include <cctype>
#include <charconv>
#include <cstring>
#include <fcntl.h>
#include <iterator>
#include <zlib.h>

#ifndef CPPHTTP_MALLOC
#    define CPPHTTP_MALLOC(size) ::malloc(size)
#endif

#ifndef CPPHTTP_FREE
#    define CPPHTTP_FREE(ptr) ::free(ptr)
#endif

namespace cpphttp
{
namespace
{
    bool is_alpha(char8_t c)
    {
        return isalpha(c);
    }

    bool is_digit(char8_t c)
    {
        return isdigit(c);
    }

    bool is_hexdigit(char8_t c)
    {
        switch(c) {
        case u8'0':
        case u8'1':
        case u8'2':
        case u8'3':
        case u8'4':
        case u8'5':
        case u8'6':
        case u8'7':
        case u8'8':
        case u8'9':
        case u8'A':
        case u8'B':
        case u8'C':
        case u8'D':
        case u8'E':
        case u8'F':
            return true;
        }
        return false;
    }

    bool is_unreserved(char8_t c)
    {
        if(is_alpha(c) || is_digit(c)) {
            return true;
        }
        switch(c) {
        case u8'-':
        case u8'.':
        case u8'_':
        case u8'~':
            return true;
        }
        return false;
    }

    bool is_gen_delims(char8_t c)
    {
        switch(c) {
        case u8':':
        case u8'/':
        case u8'?':
        case u8'#':
        case u8'[':
        case u8']':
        case u8'@':
            return true;
        }
        return false;
    }

    bool is_sub_delims(char8_t c)
    {
        switch(c) {
        case u8'!':
        case u8'$':
        case u8'&':
        case u8'\'':
        case u8'(':
        case u8')':
        case u8'*':
        case u8'+':
        case u8',':
        case u8';':
        case u8'=':
            return true;
        }
        return false;
    }

    bool is_reserved(char8_t c)
    {
        return is_gen_delims(c) || is_sub_delims(c);
    }

    bool is_pct_encoded(const char8_t* current, const char8_t* end)
    {
        if(std::distance(current, end) < 3) {
            return false;
        }
        return u8'%' == current[0] && is_hexdigit(current[1]) && is_hexdigit(current[2]);
    }

    int32_t is_pchar(const char8_t* current, const char8_t* end)
    {
        assert(current != end);
        if(is_unreserved(*current) || is_sub_delims(*current) || u8':' == *current || u8'@' == *current) {
            return 1;
        }
        if(is_pct_encoded(current, end)) {
            return 3;
        }
        return 0;
    }

    bool is_single_slash(const char8_t* current, const char8_t* end)
    {
        if(std::distance(current, end) < 1) {
            return false;
        }
        return u8'/' == current[0];
    }

    bool is_double_slash(const char8_t* current, const char8_t* end)
    {
        if(std::distance(current, end) < 2) {
            return false;
        }
        return u8'/' == current[0] || u8'/' == current[1];
    }

    bool is_single_colon(const char8_t* current, const char8_t* end)
    {
        if(end <= current) {
            return false;
        }
        return u8':' == current[0];
    }

    bool is_double_colon(const char8_t* current, const char8_t* end)
    {
        if(std::distance(current, end) < 2) {
            return false;
        }
        return u8':' == current[0] || u8':' == current[1];
    }

    bool has_userinfo(const char8_t* current, const char8_t* end)
    {
        while(current < end) {
            if(u8'@' == *current) {
                return true;
            }
            ++current;
        }
        return false;
    }

    const char8_t* parse_scheme(Section& scheme, const char8_t* begin, const char8_t* end)
    {
        const char8_t* current = begin;
        while(current < end) {
            if(*current == u8':') {
                scheme.size_ = static_cast<uint32_t>(std::distance(begin, current));
                scheme.str_ = begin;
                return current;
            }
            if(!is_alpha(*current)
               && !is_digit(*current)
               && u8'+' != *current
               && u8'-' != *current
               && u8'.' != *current) {
                return end;
            }
            ++current;
        }
        return end;
    }

    const char8_t* parse_userinfo(const char8_t* begin, const char8_t* end)
    {
        const char8_t* current = begin;
        while(current < end) {
            if(u8'@' == *current) {
                return current;
            }
            ++current;
        }
        return current;
    }

    bool is_h16(const char8_t* begin, const char8_t* end)
    {
        if(std::distance(begin, end) < 4) {
            return false;
        }
        return is_hexdigit(begin[0]) && is_hexdigit(begin[1]) && is_hexdigit(begin[2]) && is_hexdigit(begin[3]);
    }

    void count_ipv6_prefix(int32_t& prefix, int32_t& suffix, const char8_t* begin, const char8_t* end)
    {
        prefix = 0;
        suffix = 0;
        const char8_t* current = begin;
        while(end != current && prefix < 7) {
            if(is_h16(current, end)) {
                ++prefix;
                current += 4;
                if(is_double_colon(current, end)) {
                    switch(prefix) {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                        suffix = 5 - prefix;
                        break;
                    }
                    return;
                } else if(is_single_colon(current, end)) {
                    ++current;
                }
            } else {
                break;
            }
        }
        suffix = 6;
    }

    bool is_hex_ls32(const char8_t* begin, const char8_t* end)
    {
        if(std::distance(begin, end) < 9) {
            return false;
        }
        return is_hexdigit(begin[0])
               && is_hexdigit(begin[1])
               && is_hexdigit(begin[2])
               && is_hexdigit(begin[3])
               && u8':' == begin[4]
               && is_hexdigit(begin[5])
               && is_hexdigit(begin[6])
               && is_hexdigit(begin[7])
               && is_hexdigit(begin[8]);
    }

    int32_t count_octet(const char8_t* begin, const char8_t* end)
    {
        int32_t count = 0;
        while(begin < end) {
            if(u8'.' == *begin) {
                break;
            }
            if(!is_digit(*begin)) {
                break;
            }
            ++count;
            ++begin;
        }
        return count;
    }

    const char8_t* parse_dec_octet(const char8_t* begin, const char8_t* end)
    {
        const char8_t* current = begin;
        int32_t count = 0;
        while(current < end && count < 4) {
            int32_t c = count_octet(current, end);
            switch(c) {
            case 1:
                current += (count <= 3) ? 2 : 1;
                break;
            case 2:
                if(current[0] < 0x31 || 0x39 < current[0]) {
                    return end;
                }
                current += (count <= 3) ? 3 : 2;
                break;
            case 3:
                if(u8'1' == current[0]) {
                } else if(u8'2' == current[0]) {
                    if(u8'5' < current[1]) {
                        return end;
                    }

                    if(u8'5' == current[1]) {
                        if(current[2] < 0x30 || 0x35 < current[2]) {
                            return end;
                        }
                    }
                } else {
                    return end;
                }
                current += (count <= 3) ? 4 : 3;
                break;
            default:
                return end;
            }
            ++count;
        }
        return count == 4 ? current : end;
    }

    int32_t parse_ipv4(const char8_t* begin, const char8_t* end)
    {
        const char8_t* current = begin;
        int32_t count = 0;
        while(current < end && count < 4) {
            current = parse_dec_octet(current, end);
            if(end == current) {
                return 0;
            }
            ++count;
        }
        return 4 == count ? static_cast<uint32_t>(std::distance(begin, current)) : 0;
    }

    const char8_t* parse_ls32(const char8_t* begin, const char8_t* end)
    {
        if(is_hex_ls32(begin, end)) {
            return begin + 9;
        }
        int32_t l = parse_ipv4(begin, end);
        return begin + l;
    }

    bool parse_ipv6_prefix(const char8_t*& current, int32_t num_prefix, const char8_t* end)
    {
        assert(1 <= num_prefix && num_prefix <= 8);
        if(!is_double_colon(current, end)) {
            current += 2;
            return true;
        }
        --num_prefix;
        int32_t count = 0;
        while(current < end && count < num_prefix) {
            if(!is_h16(current, end)) {
                return false;
            }
            current += 4;
            if(count < (num_prefix - 1)) {
                if(u8':' != *current) {
                    return false;
                }
                ++current;
            }
            ++count;
        }
        return (count == num_prefix);
    }

    bool parse_ipv6_suffix(const char8_t*& current, int32_t num_suffix, const char8_t* end)
    {
        assert(0 <= num_suffix && num_suffix <= 6);
        int32_t count = 0;
        while(current < end && count < num_suffix) {
            if(!is_h16(current, end)) {
                return false;
            }
            current += 4;
            if(end <= current) {
                return false;
            }
            if(u8':' != *current) {
                return false;
            }
            ++current;
            ++count;
        }
        return (count == num_suffix);
    }

    const char8_t* parse_ipv6(Section& host, const char8_t* begin, const char8_t* end)
    {
        int32_t prefix;
        int32_t suffix;
        count_ipv6_prefix(prefix, suffix, begin, end);
        if(7 < prefix) {
            return end;
        }
        const char8_t* current = begin;
        if(!parse_ipv6_prefix(current, prefix, end)) {
            return end;
        }
        if(is_double_colon(current, end)) {
            current += 2;
        }
        if(!parse_ipv6_suffix(current, suffix, end)) {
            return end;
        }

        switch(suffix) {
        case 0:
            switch(prefix) {
            case 5:
                current = parse_ls32(current, end);
                host.size_ = static_cast<uint32_t>(std::distance(begin, current));
                host.str_ = begin;
                break;
            case 6:
                if(is_h16(current, end)) {
                    current += 4;
                    host.size_ = static_cast<uint32_t>(std::distance(begin, current));
                    host.str_ = begin;
                } else {
                    return end;
                }
            case 7:
                host.size_ = static_cast<uint32_t>(std::distance(begin, current));
                host.str_ = begin;
                break;
            default:
                return end;
            }
        default:
            current = parse_ls32(current, end);
            host.size_ = static_cast<uint32_t>(std::distance(begin, current));
            host.str_ = begin;
            break;
        }
        if(end <= current || u8']' != *current) {
            host = {};
            return end;
        }
        ++current;
        return current;
    }

    const char8_t* parse_ip_future(Section& host, const char8_t* begin, const char8_t* end)
    {
        assert(u8'v' == *begin);
        ++begin;
        const char8_t* current = begin;
        int32_t count = 0;
        while(current < end) {
            if(!is_hexdigit(*current)) {
                break;
            }
            ++current;
            ++count;
        }
        if(count <= 0 || end <= current || u8'.' != *current) {
            return end;
        }
        ++current;
        count = 0;
        while(current < end) {
            if(!is_unreserved(*current) || !is_sub_delims(*current) || u8':' != *current) {
                return end;
            }
            ++current;
            ++count;
        }
        if(count <= 0 || end <= current || u8']' != *current) {
            return end;
        }
        host.size_ = static_cast<uint32_t>(std::distance(begin, current));
        host.str_ = begin;
        ++current;
        return current;
    }

    const char8_t* parse_ip_literal(Section& host, const char8_t* begin, const char8_t* end)
    {
        assert(u8'[' == *begin);
        ++begin;
        if(end <= begin) {
            return end;
        }
        if(u8']' == *begin) {
            return begin + 1;
        } else if(u8'v' == *begin) {
            return parse_ip_future(host, begin, end);
        } else {
            return parse_ipv6(host, begin, end);
        }
    }

    const char8_t* parse_reg_name(Section& host, const char8_t* begin, const char8_t* end)
    {
        const char8_t* current = begin;
        while(current < end) {
            if(is_unreserved(*current) || is_sub_delims(*current)) {
                ++current;
                continue;
            }
            if(is_pct_encoded(current, end)) {
                current += 3;
                continue;
            }
            break;
        }
        host.size_ = static_cast<uint32_t>(std::distance(begin, current));
        host.str_ = begin;
        return current;
    }

    const char8_t* parse_host(Section& host, const char8_t* begin, const char8_t* end)
    {
        if(end == begin) {
            return end;
        }
        if(u8'[' == *begin) {
            return parse_ip_literal(host, begin, end);
        }
        int32_t l = parse_ipv4(begin, end);
        if(0 < l) {
            host.size_ = static_cast<uint32_t>(l);
            host.str_ = begin;
            return begin + l;
        } else {
            return parse_reg_name(host, begin, end);
        }
    }

    const char8_t* parse_port(Section& port, const char8_t* begin, const char8_t* end)
    {
        const char8_t* current = begin;
        while(current < end) {
            if(!is_digit(*current)) {
                break;
            }
            ++current;
        }
        port.size_ = static_cast<uint32_t>(std::distance(begin, current));
        port.str_ = begin;
        return current;
    }

    const char8_t* parse_autority(Section& host, Section& port, const char8_t* begin, const char8_t* end)
    {
        if(has_userinfo(begin, end)) {
            begin = parse_userinfo(begin, end);
        }
        const char8_t* current = parse_host(host, begin, end);
        if(current < end && u8':' == *current) {
            ++current;
            return parse_port(port, current, end);
        }
        return current;
    }

    const char8_t* parse_segment(const char8_t* begin, const char8_t* end)
    {
        const char8_t* current = begin;
        while(current < end) {
            int32_t l = is_pchar(current, end);
            if(l <= 0) {
                return current;
            }
            current += l;
        }
        return current;
    }

    const char8_t* parse_path_abempty(const char8_t* begin, const char8_t* end)
    {
        const char8_t* current = begin;
        while(current < end) {
            if(u8'/' != *current) {
                return current;
            }
            ++current;
            current = parse_segment(current, end);
            if(end == current) {
                break;
            }
        }
        return current;
    }

    const char8_t* parse_segment_nz(const char8_t* begin, const char8_t* end)
    {
        assert(end != begin);
        const char8_t* current = begin;
        if(current == end) {
            return end;
        }
        do {
            int32_t l = is_pchar(current, end);
            if(l <= 0) {
                break;
            }
            current += l;
        } while(current != end);
        return current;
    }

    const char8_t* parse_path_absolute(const char8_t* begin, const char8_t* end)
    {
        assert(u8'/' == *begin);
        const char8_t* current = begin + 1;
        if(end == current) {
            return current;
        }
        int32_t l = is_pchar(current, end);
        if(0 < l) {
            current = parse_segment_nz(current, end);
            if(end == current) {
                return current;
            }
        }
        while(current < end) {
            if(u8'/' != *current) {
                return current;
            }
            ++current;
            current = parse_segment(current, end);
        }
        return current;
    }

    const char8_t* parse_path_rootless(const char8_t* begin, const char8_t* end)
    {
        assert(end != begin);
        assert(0 < is_pchar(begin, end));
        const char8_t* current = parse_segment_nz(begin, end);
        while(current < end) {
            if(u8'/' != *current) {
                break;
            }
            current = parse_segment(current, end);
        }
        return current;
    }

    const char8_t* parse_hier_part(Section& hier_part, Section& host, Section& port, const char8_t* begin, const char8_t* end)
    {
        const char8_t* current = begin;
        while(current < end) {
            if(u8'/' == *current) {
                const char8_t* path_root;
                if(end != (current + 1) && u8'/' == current[1]) {
                    current += 2;
                    current = parse_autority(host, port, current, end);
                    if(end == current) {
                        return end;
                    }
                    path_root = current;
                    current = parse_path_abempty(current, end);
                } else {
                    path_root = current;
                    current = parse_path_absolute(current, end);
                }
                if(u8'/' == *path_root) {
                    ++path_root;
                }
                hier_part.size_ = static_cast<uint32_t>(std::distance(path_root, current));
                hier_part.str_ = path_root;
                return current;
            } else if(0 < is_pchar(current, end)) {
                current = parse_path_rootless(current, end);
                hier_part.size_ = static_cast<uint32_t>(std::distance(begin, current));
                hier_part.str_ = begin;
                return current;
            } else {
                return end;
            }
        }
        return end;
    }
} // namespace

bool parse_url(
    Section& scheme,
    Section& host,
    Section& port,
    Section& path,
    uint32_t size,
    const char8_t* url)
{
    assert(nullptr != url);
    const char8_t* current = url;
    const char8_t* end = url + size;
    scheme = {};
    host = {};
    port = {};
    current = parse_scheme(scheme, current, end);
    if(end <= current || u8':' != *current) {
        return false;
    }
    ++current;
    path = {};
    current = parse_hier_part(path, host, port, current, end);
    return 0 < scheme.size_;
}

Buffer::Buffer()
    : capacity_(0)
    , size_(0)
    , items_(nullptr)
{
}

Buffer::~Buffer()
{
    CPPHTTP_FREE(items_);
    items_ = nullptr;
    size_ = 0;
    capacity_ = 0;
}

uint32_t Buffer::capacity() const
{
    return capacity_;
}

uint32_t Buffer::size() const
{
    return size_;
}

void Buffer::clear()
{
    size_ = 0;
}

void Buffer::reserve(uint32_t x)
{
    if(capacity_ < x) {
        expand(x);
    }
}

void Buffer::resize(uint32_t x)
{
    if(capacity_ < x) {
        expand(x);
    }
    size_ = x;
}

void Buffer::pop_front(uint32_t x)
{
    assert(x <= size_);
    size_ -= x;
    for(uint32_t i = 0; i < size_; ++i) {
        items_[i] = items_[i + x];
    }
}

void Buffer::push_back(uint32_t s, const uint8_t* items)
{
    assert(nullptr != items);
    if(capacity_ < (size_ + s)) {
        expand(size_ + s);
    }
    ::memcpy(items_ + size_, items, s);
    size_ += s;
}

void Buffer::push_str(const uint8_t* str)
{
    uint32_t s = ::strlen((const char*)str);
    push_back(s, str);
}

const uint8_t& Buffer::operator[](uint32_t x) const
{
    return items_[x];
}

uint8_t& Buffer::operator[](uint32_t x)
{
    return items_[x];
}

const uint8_t* Buffer::begin() const
{
    return items_;
}

uint8_t* Buffer::begin()
{
    return items_;
}

const uint8_t* Buffer::end() const
{
    return items_ + size_;
}

uint8_t* Buffer::end()
{
    return items_ + size_;
}

void Buffer::expand(uint32_t x)
{
    uint32_t new_capacity = capacity_;
    while(new_capacity < x) {
        new_capacity += 64;
    }
    uint8_t* items = (uint8_t*)CPPHTTP_MALLOC(new_capacity);
    ::memcpy(items, items_, capacity_);
    CPPHTTP_FREE(items_);
    capacity_ = new_capacity;
    items_ = items;
}

#ifdef _WIN32
namespace
{
    struct WSA
    {
    public:
        WSA()
        {
            WORD wVersionRequested = wVersionRequested = MAKEWORD(2, 2);
            WSADATA wsaData;
            WSAStartup(wVersionRequested, &wsaData);
        }
        ~WSA()
        {
            WSACleanup();
        }
    };
    WSA wsa;
} // namespace

Socket::Socket()
    : s_(INVALID_SOCKET)
{
}

Socket::~Socket()
{
    close();
}

bool Socket::open(const char* node, const char* port)
{
    assert(INVALID_SOCKET == s_);
    addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    addrinfo* result = nullptr;
    int32_t r = getaddrinfo(node, port, &hints, &result);
    if(0 != r) {
        return false;
    }
    s_ = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if(INVALID_SOCKET == s_) {
        freeaddrinfo(result);
        return false;
    }
    u_long nonblocking = 1;
#    ifdef _WIN32
    r = ioctlsocket(s_, FIONBIO, &nonblocking);
#    else
    int32_t flags = fcntl(s_, F_GETFL, 0);
    r = fcntl(s_, F_SETFL, flags | O_NONBLOCK);
#    endif
    if(r < 0) {
        freeaddrinfo(result);
#    ifdef _WIN32
        closesocket(s_);
#    else
        close(s_);
#    endif
        s_ = INVALID_SOCKET;
        return false;
    }

    ::memcpy(&address_, result, sizeof(addrinfo));
    freeaddrinfo(result);
    return INVALID_SOCKET != s_;
}

bool Socket::connect()
{
    assert(INVALID_SOCKET != s_);
    int32_t r = ::connect(s_, address_.ai_addr, (int32_t)address_.ai_addrlen);
    if(r == SOCKET_ERROR) {
#    ifdef _WIN32
        if(WSAEWOULDBLOCK != WSAGetLastError() && WSAEINPROGRESS != WSAGetLastError()) {
            return false;
        }
#    else
        if(EWOULDBLOCK != errno && EINPROGRESS != errno && EAGAIN != errno) {
            return false;
        }
#    endif
    }
    return true;
}

void Socket::shutdown(SD sd)
{
    ::shutdown(s_, (int32_t)sd);
}

void Socket::close()
{
    if(INVALID_SOCKET != s_) {
        closesocket(s_);
        s_ = INVALID_SOCKET;
    }
}

int32_t Socket::send(int32_t size, const uint8_t* data)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(s_, &fds);
    struct timeval tv = {};
    tv.tv_sec = 0;
    tv.tv_usec = 10;

    for(int32_t i = 0; i < 3; ++i) {
        int32_t r = ::select((int)(s_ + 1), nullptr, &fds, nullptr, &tv);
        if(0 < r) {
            r = ::send(s_, (const char*)data, size, 0);
            return r == SOCKET_ERROR ? -1 : r;
        } else if(r < 0) {
            return -1;
        }
    }
    return -1;
}

int32_t Socket::recieve(Buffer& buff)
{
    static constexpr int32_t BufferSize = 1024;
    char buffer[BufferSize];
    struct timeval tv = {};
    tv.tv_sec = 0;
    tv.tv_usec = 100000;

    fd_set origin;
    FD_ZERO(&origin);
    FD_SET(s_, &origin);

    int32_t total = 0;
    for(int32_t i = 0; i < 10; ++i) {
        fd_set fds = origin;
        int32_t n = ::select((int)(s_ + 1), &fds, nullptr, nullptr, &tv);
        if(0 == n) {
            int32_t err = get_error();
            if(0 != err && err != ETIMEDOUT) {
                return 0;
            }
            tv.tv_usec += 10000;
            continue;
        } else if(n < 0) {
            return 0;
        }
        {
            int32_t r = recv(s_, buffer, BufferSize, 0);
            if(0 < r) {
                buff.push_back(static_cast<uint32_t>(r), (const uint8_t*)buffer);
                total += r;
                i = 0;
                tv.tv_usec = 10000;
            } else if(0 < r) {
                return 0;
            }
        }
    }
    return total;
}

int32_t Socket::get_error()
{
    int opt = 0;
    socklen_t len = sizeof(opt);
    if(getsockopt(s_, SOL_SOCKET, SO_ERROR, (char*)(&opt), &len) < 0) {
        return -1;
    }
    return opt;
}
#endif

namespace
{
    bool decode(Buffer& result)
    {
        z_stream stream;
        stream.zalloc = NULL;
        stream.zfree = NULL;
        stream.opaque = NULL;
        int32_t ret = inflateInit2(&stream, 32);
        if(Z_OK != ret) {
            return false;
        }
        uint32_t src_size = result.size();
        uint8_t* encoded = (uint8_t*)CPPHTTP_MALLOC(src_size);
        ::memcpy(encoded, result.begin(), src_size);
        result.clear();

        static constexpr uint32_t Chunk = 4096;
        uint8_t out[Chunk];
        uint32_t count = 0;
        int32_t outCount = 0;
        do {
            if(src_size <= count) {
                inflateEnd(&stream);
                CPPHTTP_FREE(encoded);
                return true;
            }
            uint32_t size = src_size - count;
            stream.avail_in = size;
            stream.next_in = encoded + count;
            count += size;

            do {
                stream.avail_out = Chunk;
                stream.next_out = out;
                ret = inflate(&stream, Z_NO_FLUSH);
                switch(ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR;
                    break;
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    inflateEnd(&stream);
                    CPPHTTP_FREE(encoded);
                    return false;
                }
                uint32_t s = Chunk - stream.avail_out;
                result.push_back(s, out);
                outCount += s;
            } while(stream.avail_out == 0);
            assert(stream.avail_in <= 0);
        } while(ret != Z_STREAM_END);
        inflateEnd(&stream);
        CPPHTTP_FREE(encoded);
        return ret == Z_STREAM_END ? true : false;
    }
} // namespace

Http::Http()
{
    host_[0] = '\0';
    port_[0] = '\0';
    path_[0] = '\0';
}

Http::~Http()
{
    close();
}

bool Http::open(const char8_t* url)
{
    assert(nullptr != url);
    Section scheme;
    Section host;
    Section port;
    Section path;
    if(!parse_url(scheme, host, port, path, ::strlen((const char*)url), url)) {
        return false;
    }
    if(scheme.size_ <= 0 || host.size_ <= 0 || MaxHostSize <= host.size_ || MaxPathSize <= path.size_) {
        return false;
    }
    if(0 != ::strncmp((const char*)scheme.str_, "http", ::strlen("http"))) {
        return false;
    }
    ::memcpy(host_, host.str_, host.size_);
    host_[host.size_] = '\0';

    if(0 < port.size_ && port.size_ < MaxPortSize) {
        ::memcpy(port_, port.str_, port.size_);
        port_[port.size_] = '\0';
    } else {
        ::strcat(port_, "80");
    }
    ::memcpy(path_, path.str_, path.size_);
    path_[path.size_] = '\0';
    if(!socket_.open(host_, port_)) {
        return false;
    }
    return true;
}

void Http::close()
{
    socket_.close();
    host_[0] = '\0';
    port_[0] = '\0';
    path_[0] = '\0';
}

bool Http::get(Buffer& result, const char8_t* content_type)
{
    if(!socket_.connect()) {
        return false;
    }
    {
        result.clear();
        set_header(result, true, 0, content_type);
        // char buf[256];
        //::snprintf(buf, result.size(), "%s", result.begin());
        int32_t r = socket_.send(result.size(), &result[0]);
        result.clear();
        if(r < 0) {
            socket_.shutdown();
            return false;
        }
    }

    {
        int32_t r = socket_.recieve(result);
        if(r <= 0 || result.size() <= 0) {
            socket_.shutdown();
            return false;
        }
        const char8_t* begin = (const char8_t*)result.begin();
        const char8_t* end = (const char8_t*)result.end();
        int32_t status;
        int32_t length;
        Encoding encoding;
        begin = parse(status, length, encoding, begin, end);
        size_t size = std::distance(begin, end);
        if(200 != status
           || (0 < length && static_cast<uint32_t>(length) != size)
           || (Encoding::None != encoding && Encoding::GZip != encoding)) {
            socket_.shutdown();
            return false;
        }
        result.pop_front(std::distance((const char8_t*)&result[0], begin));
        if(Encoding::GZip == encoding) {
            if(!decode(result)) {
                socket_.shutdown();
                return false;
            }
        }
    }
    socket_.shutdown();
    return true;
}

bool Http::post(Buffer& result, uint32_t size, const char8_t* data, const char8_t* content_type)
{
    {
        result.clear();
        set_header(result, false, size, content_type);
        result.push_back(size, (const uint8_t*)data);

        int32_t r = socket_.send(result.size(), &result[0]);
        result.clear();
        if(r < 0) {
            return false;
        }
    }

    {
        int32_t r = socket_.recieve(result);
        if(r <= 0 || result.size() <= 0) {
            return false;
        }
        const char8_t* begin = (const char8_t*)&result[0];
        const char8_t* end = (const char8_t*)&result[result.size() - 1];
        int32_t status;
        int32_t length;
        Encoding encoding;
        begin = parse(status, length, encoding, begin, end);
        size_t size = std::distance(begin, end);
        if(200 != status
           || (0 < length && static_cast<uint32_t>(length) != size)
           || (Encoding::None != encoding && Encoding::GZip != encoding)) {
            return false;
        }
        result.pop_front(std::distance((const char8_t*)&result[0], begin));
        if(Encoding::GZip == encoding) {
            if(!decode(result)) {
                return false;
            }
        }
    }
    return true;
}

void Http::set_header(Buffer& header, bool get, uint32_t size, const char8_t* content_type)
{
    if(get) {
        header.push_str((const uint8_t*)"GET /");
    } else {
        header.push_str((const uint8_t*)"POST /");
    }
    header.push_str((const uint8_t*)path_);
    header.push_str((const uint8_t*)" HTTP/1.1\r\nHOST: ");
    header.push_str((const uint8_t*)host_);
    header.push_str((const uint8_t*)"\r\n");
    if(nullptr != content_type) {
        header.push_str((const uint8_t*)"Content-Type: ");
        header.push_str((const uint8_t*)content_type);
        header.push_str((const uint8_t*)"\r\n");
    }
    if(0 < size) {
        char length[16];
        snprintf(length, 16, "%d", size);
        header.push_str((const uint8_t*)"Content-Length: ");
        header.push_str((const uint8_t*)length);
        header.push_str((const uint8_t*)"\r\n");
    }
    header.push_str((const uint8_t*)"Accept-Encoding: gzip\r\n");
    header.push_str((const uint8_t*)"\r\n");
}

namespace
{
    const char8_t* skip_line(const char8_t* current, const char8_t* end)
    {
        while(current < end) {
            if(u8'\r' == current[0]) {
                if((current + 1) < end && u8'\n' == current[1]) {
                    return current + 2;
                }
                return end;
            }
            ++current;
        }
        return current;
    }

    int32_t is_header_end(const char8_t* current, const char8_t* end)
    {
        if(std::distance(current, end) < 2) {
            return -1;
        }
        return u8'\r' == current[0] && u8'\n' == current[1] ? 1 : 0;
    }

    const char8_t* parse_status(int32_t& status, const char8_t* current, const char8_t* end)
    {
        if(std::distance(current, end) < 8) {
            return end;
        }
        if(0 != ::strncmp((const char*)current, "HTTP/1.1", 8)) {
            return end;
        }
        current += 8;
        while(current < end) {
            if(!isspace(*current)) {
                break;
            }
            ++current;
        }
        if(end <= current || !is_digit(*current)) {
            return end;
        }
        const char8_t* number = current;
        while(current < end) {
            if(!is_digit(*current)) {
                break;
            }
            ++current;
        }
        auto [ptr, ec] = std::from_chars((const char*)number, (const char*)current, status);
        if(ec != std::errc{}) {
            status = 0;
            return end;
        }
        return skip_line(current, end);
    }

    int32_t parse_number(const char8_t* current, const char8_t* end)
    {
        while(current < end) {
            if(!isspace(*current)) {
                break;
            }
            ++current;
        }
        if(end <= current || !is_digit(*current)) {
            return 0;
        }
        const char8_t* number = current;
        while(current < end) {
            if(!is_digit(*current)) {
                break;
            }
            ++current;
        }
        int32_t result = 0;
        auto [ptr, ec] = std::from_chars((const char*)number, (const char*)current, result);
        if(ec != std::errc{}) {
            result = 0;
        }
        return result;
    }

    Http::Encoding parse_encoding(const char8_t* current, const char8_t* end)
    {
        while(current < end) {
            if(!isspace(*current)) {
                break;
            }
            ++current;
        }
        if(end <= current
           || (u8'g' != *current
               && u8'c' != *current
               && u8'd' != *current
               && u8'b' != *current)) {
            return Http::Encoding::None;
        }
        const char8_t* encoding = current;
        while(current < end) {
            if(!is_alpha(*current)) {
                break;
            }
            ++current;
        }
        uint32_t len = std::distance(encoding, current);
        if(0 == strncmp((const char*)encoding, "gzip", len)) {
            return Http::Encoding::GZip;
        } else if(0 == strncmp((const char*)encoding, "compress", len)) {
            return Http::Encoding::Compress;
        } else if(0 == strncmp((const char*)encoding, "deflate", len)) {
            return Http::Encoding::Deflate;
        } else if(0 == strncmp((const char*)encoding, "br", len)) {
            return Http::Encoding::Br;
        }
        return Http::Encoding::None;
    }

    const char8_t* parse_header(int32_t& length, Http::Encoding& encoding, const char8_t* current, const char8_t* end)
    {
        while(current < end) {
            int32_t r = is_header_end(current, end);
            if(r < 0) {
                return end;
            }
            if(1 == r) {
                return current;
            }
            if(0 == ::strncmp((const char*)current, "Content-Length:", 15)) {
                length = parse_number(current + 15, end);
                current = skip_line(current, end);
            } else if(0 == ::strncmp((const char*)current, "Content-Encoding:", 17)) {
                encoding = parse_encoding(current + 17, end);
                current = skip_line(current, end);
            } else {
                current = skip_line(current, end);
            }
        }
        return current;
    }

    const char8_t* skip_to_content(const char8_t* current, const char8_t* end)
    {
        while(current < end) {
            int32_t r = is_header_end(current, end);
            if(r < 0) {
                return end;
            }
            if(1 == r) {
                return current + 2;
            }
            current = skip_line(current, end);
        }
        return current;
    }
} // namespace

const char8_t* Http::parse(int32_t& status, int32_t& length, Encoding& encoding, const char8_t* current, const char8_t* end)
{
    char buffer[1024];
    snprintf(buffer, 1024, "%s", (const char*)current);
    status = 0;
    length = 0;
    encoding = Encoding::None;
    current = parse_status(status, current, end);
    if(200 != status) {
        return end;
    }
    current = parse_header(length, encoding, current, end);
    return skip_to_content(current, end);
}

} // namespace cpphttp
#endif
