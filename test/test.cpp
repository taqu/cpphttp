#include "../cpphttp.h"
#include "catch_wrap.hpp"
#include <cstdio>
#include <string>

TEST_CASE("TestURL::Buffer")
{
	using namespace cpphttp;

	Buffer buffer;
	buffer.push_back(strlen("abcd"), (const uint8_t*)"abcd");
	buffer.push_back(strlen("efgh"), (const uint8_t*)"efgh");
	buffer.push_back(1,(const uint8_t*)"\0");
	printf("%s\n", (const char*)&buffer[0]);
}

TEST_CASE("TestURL::Parse")
{
	using namespace cpphttp;
	bool result;
	const char8_t* url;
	url = u8"https://tex2e.github.io/rfc-translater/html/rfc3986.html";
	Section scheme;
	Section host;
	Section port;
	Section path;
	result = parse_url(scheme, host, port, path, strlen((const char*)url), url);
    {
		char buffer[128];
		::memcpy(buffer, scheme.str_, scheme.size_);
		buffer[scheme.size_] = '\0';
		printf("%s\n", buffer);

		::memcpy(buffer, host.str_, host.size_);
		buffer[host.size_] = '\0';
		printf("%s\n", buffer);

		::memcpy(buffer, port.str_, port.size_);
		buffer[port.size_] = '\0';
		printf(buffer);

		::memcpy(buffer, path.str_, path.size_);
		buffer[path.size_] = '\0';
		printf(buffer);
    }
}

TEST_CASE("TestURL::Get")
{
	using namespace cpphttp;
	bool result;
	const char8_t* url;
	url = u8"http://192.168.128.178:9090/";
	Http http;
	result = http.open(url);
	Buffer buffer;
	http.get(buffer, nullptr);
	if(0<buffer.size()){
		buffer.push_back(1, (const uint8_t*)"\0");
		const char* s = (const char*)buffer.begin();
		printf("%s", s);
	}

	http.get(buffer, nullptr);
	if(0<buffer.size()){
		buffer.push_back(1, (const uint8_t*)"\0");
		const char* s = (const char*)buffer.begin();
		printf("%s", s);
	}
}
