#include "../cpphttp.h"
#include "catch_wrap.hpp"
#include <cstdio>

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
	url = u8"http://2-terra-tech1.cs.sega:9091/";
	Http http;
	result = http.open(url);
	Buffer buffer;
	http.get(buffer, nullptr);
}
