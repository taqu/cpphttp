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
//#define CPPHTTP_IMPLEMENTATION

namespace cpphttp
{
	struct Section
    {
		uint32_t size_;
		const char8_t* str_;
    };

	bool parse_url(
		Section& scheme,
		Section& host,
		Section& port,
		uint32_t size,
		const char8_t* url);
}
#endif //INC_CPPHTTP_H_

#ifdef CPPHTTP_IMPLEMENTATION
#include <cctype>
#include <iterator>

namespace cpphttp
{
namespace
{
	bool is_alpha(char8_t c)
    {
		return isalpha(c);
    }

	bool is_digit(char8_t c){
		return isdigit(c);
	}

	bool is_hexdigit(char8_t c){
		switch(c){
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
		if(is_alpha(c) || is_digit(c)){
			return true;
		}
		switch(c){
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
		switch(c){
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
		switch(c){
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

	bool is_pct_encoded(const char8_t* current, const char8_t* end){
		if(std::distance(current, end)<3){
			return false;
		}
		return u8'%' == current[0] && is_hexdigit(current[1]) && is_hexdigit(current[2]);
	}

	int32_t is_pchar(const char8_t* current, const char8_t* end){
		assert(current != end);
		if(is_unreserved(*current) || is_sub_delims(*current) || u8':'==*current || u8'@'==*current){
			return 1;
		}
		if(is_pct_encoded(current, end)){
			return 3;
		}
		return 0;
	}

	bool is_single_slash(const char8_t* current, const char8_t* end){
		if(std::distance(current, end)<1){
			return false;
		}
		return u8'/' == current[0];
	}

	bool is_double_slash(const char8_t* current, const char8_t* end){
		if(std::distance(current, end)<2){
			return false;
		}
		return u8'/' == current[0] || u8'/' == current[1];
	}

	bool is_single_colon(const char8_t* current, const char8_t* end){
		if(end<=current){
			return false;
		}
		return u8':' == current[0];
	}

	bool is_double_colon(const char8_t* current, const char8_t* end){
		if(std::distance(current, end)<2){
			return false;
		}
		return u8':' == current[0] || u8':' == current[1];
	}

	bool has_userinfo(const char8_t* current, const char8_t* end){
		while(current<end){
			if(u8'@' == *current){
				return true;
			}
			++current;
		}
		return false;
	}

	const char8_t* parse_scheme(Section& scheme, const char8_t* begin, const char8_t* end)
    {
		const char8_t* current = begin;
		while(current<end){
			if(*current == u8':'){
				scheme.size_ = static_cast<uint32_t>(std::distance(begin, current));
				scheme.str_ = begin;
				return current;
			}
			if(!is_alpha(*current)
				&& !is_digit(*current)
				&& u8'+' != *current
				&& u8'-' != *current
				&& u8'.' != *current){
				return end;
			}
			++current;
		}
		return end;
    }

	const char8_t* parse_userinfo(const char8_t* begin, const char8_t* end)
    {
		const char8_t* current = begin;
		while(current<end){
			if(u8'@' == *current){
				return current;
			}
			++current;
		}
		return current;
	}

	bool is_h16(const char8_t* begin, const char8_t* end)
    {
		if(std::distance(begin, end)<4){
			return false;
		}
		return is_hexdigit(begin[0]) && is_hexdigit(begin[1]) && is_hexdigit(begin[2]) && is_hexdigit(begin[3]);
    }

	void count_ipv6_prefix(int32_t& prefix, int32_t& suffix, const char8_t* begin, const char8_t* end)
    {
		prefix = 0;
		suffix = 0;
		const char8_t* current = begin;
		while(end != current && prefix<7){
			if(is_h16(current, end)){
				++prefix;
				current += 4;
                if(is_double_colon(current, end)) {
					switch(prefix){
                    case 0:
					case 1:
					case 2:
					case 3:
                    case 4:
						suffix = 5-prefix;
						break;
					}
					return;
                }else if(is_single_colon(current, end)){
					++current;
				}
			}else{
				break;
			}
		}
		suffix = 6;
    }

	bool is_hex_ls32(const char8_t* begin, const char8_t* end)
    {
		if(std::distance(begin, end)<9){
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

	int32_t count_octet(const char8_t* begin, const char8_t* end){
		int32_t count = 0;
		while(begin<end){
			if(u8'.'==*begin){
				break;
			}
			if(!is_digit(*begin)){
				break;
			}
			++count;
		}
		return count;
	}

	const char8_t* parse_dec_octet(const char8_t* begin, const char8_t* end){
		const char8_t* current = begin;
		int32_t count = 0;
		while(current<end && count<4){
			int32_t c = count_octet(current, end);
			switch(c){
            case 1:
				current += (count<=3)? 2 : 1;
				break;
            case 2:
				if(current[0]<0x31 || 0x39<current[0]){
					return end;
				}
				current += (count<=3)? 3 : 2;
				break;
            case 3:
				if(u8'1' == current[0]){
				}else if(u8'2' == current[0]){
                    if(u8'5' < current[1]) {
                        return end;
                    }

                    if(u8'5' == current[1]){
						if(current[2]<0x30 || 0x35<current[2]){
							return end;
						}
					}
				}else{
					return end;
				}
				current += (count<=3)? 4 : 3;
				break;
            default:
				return end;
			}
			++count;
		}
		return count==4? current:end;
	}

	int32_t parse_ipv4(const char8_t* begin, const char8_t* end){
		const char8_t* current = begin;
		int32_t count = 0;
		while(current<end && count<4){
			current = parse_dec_octet(current,end);
			if(end == current){
				return 0;
			}
			++count;
		}
		return 4==count? static_cast<uint32_t>(std::distance(begin, current)) : 0;
	}

	const char8_t* parse_ls32(const char8_t* begin, const char8_t* end)
    {
		if(is_hex_ls32(begin, end)){
			return begin + 9;
		}
		int32_t l = parse_ipv4(begin, end);
		return begin + l;
    }

	bool parse_ipv6_prefix(const char8_t*& current, int32_t num_prefix, const char8_t* end)
    {
		assert(1<=num_prefix && num_prefix<=8);
		if(!is_double_colon(current, end)){
			current += 2;
			return true;
		}
		--num_prefix;
		int32_t count = 0;
		while(current < end && count<num_prefix){
			if(!is_h16(current, end)){
				return false;
			}
			current += 4;
			if(count<(num_prefix-1)){
				if(u8':'!=*current){
					return false;
				}
				++current;
			}
			++count;
		}
		return (count==num_prefix);
    }

	bool parse_ipv6_suffix(const char8_t*& current, int32_t num_suffix, const char8_t* end)
    {
		assert(0<=num_suffix && num_suffix<=6);
		int32_t count = 0;
		while(current < end && count<num_suffix){
			if(!is_h16(current, end)){
				return false;
			}
			current += 4;
			if(end<=current){
				return false;
			}
				if(u8':'!=*current){
					return false;
				}
				++current;
			++count;
		}
		return (count==num_suffix);
    }

	const char8_t* parse_ipv6(Section& host, const char8_t* begin, const char8_t* end)
    {
		int32_t prefix;
		int32_t suffix;
		count_ipv6_prefix(prefix, suffix, begin, end);
		if(7<prefix){
			return end;
		}
		const char8_t* current = begin;
        if(!parse_ipv6_prefix(current, prefix, end)) {
            return end;
        }
		if(is_double_colon(current, end)){
			current += 2;
		}
        if(!parse_ipv6_suffix(current, suffix, end)) {
            return end;
        }

		switch(suffix){
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
				}else{
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
		if(end<=current || u8']' != *current){
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
		while(current<end){
			if(!is_hexdigit(*current)){
				break;
			}
			++current;
			++count;
		}
		if(count<=0 || end<=current || u8'.' != *current){
			return end;
		}
		++current;
		count = 0;
		while(current<end){
            if(!is_unreserved(*current) || !is_sub_delims(*current) || u8':' != *current) {
                return end;
            }
			++current;
			++count;
		}
		if(count<=0 || end<=current || u8']' != *current){
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
		if(end <= begin){
			return end;
		}
		if(u8']' == *begin){
			return begin+1;
		}else if(u8'v' == *begin){
			return parse_ip_future(host, begin, end);
		}else{
			return parse_ipv6(host, begin, end);
		}
    }

	const char8_t* parse_reg_name(Section& host, const char8_t* begin, const char8_t* end)
    {
		const char8_t* current = begin;
		while(current<end){
			if(is_unreserved(*current) || is_sub_delims(*current)){
				++current;
				continue;
			}
			if(is_pct_encoded(current, end)){
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
		if(end == begin){
			return end;
		}
		if(u8'[' == *begin){
			return parse_ip_literal(host, begin, end);
		}
		int32_t l = parse_ipv4(begin, end);
		if(0<l){
			host.size_ = static_cast<uint32_t>(l);
			host.str_ = begin;
			return begin+l;
		}else{
			return parse_reg_name(host, begin, end);
		}
    }

	const char8_t* parse_port(Section& port, const char8_t* begin, const char8_t* end)
    {
		const char8_t* current = begin;
		while(current<end){
			if(!is_digit(*current)){
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
		if(has_userinfo(begin, end)){
			begin = parse_userinfo(begin, end);
		}
		const char8_t* current = parse_host(host, begin, end);
		if(current<end && u8':'==*current){
			++current;
			return parse_port(port, current, end);
		}
		return current;
    }

	const char8_t* parse_segment(const char8_t* begin, const char8_t* end)
    {
		const char8_t* current = begin;
		while(current<end){
			int32_t l = is_pchar(current, end);
			if(l<=0){
				return current;
			}
			current += l;
		}
		return current;
    }

	const char8_t* parse_path_abempty(const char8_t* begin, const char8_t* end)
    {
		const char8_t* current = begin;
		while(current<end){
			if(u8'/' != *current){
				return current;
			}
			++current;
			current = parse_segment(current, end);
			if(end == current){
				break;
			}
		}
		return current;
    }

	const char8_t* parse_segment_nz(const char8_t* begin, const char8_t* end)
    {
		assert(end != begin);
		const char8_t* current = begin;
		if(current == end){
			return end;
		}
		do{
			int32_t l = is_pchar(current, end);
			if(l<=0){
				break;
			}
			current += l;
		}while(current != end);
		return current;
    }

	const char8_t* parse_path_absolute(const char8_t* begin, const char8_t* end)
    {
		assert(u8'/' == *begin);
		const char8_t* current = begin + 1;
		if(end == current){
			return current;
		}
		int32_t l = is_pchar(current, end);
			if(0<l){
				current = parse_segment_nz(current, end);
				if(end == current){
					return current;
				}
			}
		while(current<end){
			if(u8'/' != *current){
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
		assert(0<is_pchar(begin, end));
		const char8_t* current = parse_segment_nz(begin, end);
		while(current<end){
			if(u8'/' != *current){
				break;
			}
			current = parse_segment(current, end);
		}
		return current;
	}

	const char8_t* parse_hier_part(Section& hier_part, Section& host, Section& port, const char8_t* begin, const char8_t* end)
    {
		const char8_t* current = begin;
		while(current<end){
			if(u8'/' == *current){
				if(end != (current+1) && u8'/' == current[1]){
					current += 2;
                    current = parse_autority(host, port, current, end);
                    if(end == current) {
                        return end;
                    }
                    current = parse_path_abempty(current, end);
                }else{
					current = parse_path_absolute(current, end);
				}
				hier_part.size_ = static_cast<uint32_t>(std::distance(begin, current));
				hier_part.str_ = begin;
				return current;
			}else if(0<is_pchar(current, end)){
				current = parse_path_rootless(current, end);
				hier_part.size_ = static_cast<uint32_t>(std::distance(begin, current));
				hier_part.str_ = begin;
				return current;
			}else{
				return end;
			}
		}
		return end;
    }
}

bool parse_url(
		Section& scheme,
		Section& host,
		Section& port,
		uint32_t size,
		const char8_t* url)
{
	assert(nullptr != url);
	const char8_t* current = url;
	const char8_t* end = url+size;
	scheme = {};
	host = {};
	port = {};
	current = parse_scheme(scheme, current, end);
	if(end<=current || u8':'!= *current){
		return false;
	}
	++current;
	Section hier_part = {};
	current = parse_hier_part(hier_part, host, port, current, end);
	return 0<scheme.size_ && 0<hier_part.size_;
}
}
#endif

