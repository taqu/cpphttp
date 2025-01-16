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
	Section post;
	result = parse_url(scheme, host, post, strlen((const char*)url), url);
    {
		char buffer[128];
		::memcpy(buffer, scheme.str_, scheme.size_);
		buffer[scheme.size_] = '\0';
		printf("%s\n", buffer);

		::memcpy(buffer, host.str_, host.size_);
		buffer[host.size_] = '\0';
		printf("%s\n", buffer);

		::memcpy(buffer, post.str_, post.size_);
		buffer[post.size_] = '\0';
		printf(buffer);
    }
}

