#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <lua.h>
#include <lauxlib.h>
#include <zlib.h>

#define GZIP_WBITS 15+16
#define GZIP_HEADER_BOUND 16
#define DEF_MEM_LEVEL 8
#define GROWTH_MULTIPLIER 2

static int gzip_compress(lua_State *L) {
	size_t dataLen;
	unsigned char *const data = (unsigned char *) luaL_checklstring(L, 1, &dataLen);
	const int level = luaL_optint(L, 2, Z_DEFAULT_COMPRESSION);

	const unsigned int outputLen = compressBound(dataLen) + GZIP_HEADER_BOUND;
	unsigned char *const output = malloc(outputLen);
	if (!output) {
		return luaL_error(L, "malloc failed: %d", errno);
	}

	z_stream stream;
	memset(&stream, 0, sizeof(stream));
	stream.next_in = data;
	stream.avail_in = dataLen;
	stream.next_out = output;
	stream.avail_out = outputLen;
	const int err1 = deflateInit2(&stream, level, Z_DEFLATED, GZIP_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
	if (err1 != Z_OK) {
		free(output);
		return luaL_error(L, "deflateInit2 failed: %d", err1);
	}

	gz_header header;
	memset(&header, 0, sizeof(header));
	const int err2 = deflateSetHeader(&stream, &header);
	if (err2 != Z_OK) {
		free(output);
		return luaL_error(L, "deflateSetHeader failed: %d", err2);
	}

	const int err3 = deflate(&stream, Z_FINISH);
	const int err4 = deflateEnd(&stream);
	if (err3 == Z_STREAM_END && err4 == Z_OK) {
		lua_pushlstring(L, output, stream.total_out);
		free(output);
		return 1;
	}
	free(output);

	if (err3 == Z_OK) {
		return luaL_error(L, "deflate failed: insufficient buffer.");
	} else if (err3 != Z_STREAM_END) {
		return luaL_error(L, "deflate failed: %d", err3);
	} else {
		return luaL_error(L, "deflateEnd failed: %d", err4);
	}
}

static int gzip_decompress(lua_State *L) {
	size_t dataLen;
	unsigned char *const data = (unsigned char*) luaL_checklstring(L, 1, &dataLen);

	size_t outputLen = dataLen*GROWTH_MULTIPLIER;
	unsigned char *output = malloc(outputLen);
	if (!output) {
		return luaL_error(L, "malloc failed: %d", errno);
	}

	z_stream stream;
	memset(&stream, 0, sizeof(stream));
	stream.next_in = data;
	stream.avail_in = dataLen;
	stream.next_out = output;
	stream.avail_out = outputLen;
	const int err1 = inflateInit2(&stream, GZIP_WBITS);
	if (err1 != Z_OK) {
		free(output);
		return luaL_error(L, "inflateInit2 failed: %d", err1);
	}

	for (;;) {
		const int err2 = inflate(&stream, Z_FINISH);
		if (err2 == Z_BUF_ERROR && stream.avail_in > 0) {
			outputLen *= GROWTH_MULTIPLIER;
			unsigned char *const output2 = realloc(output, outputLen);
			if (!output2) {
				free(output);
				return luaL_error(L, "realloc failed: %d", errno);
			}
			output = output2;
			stream.next_out = output + stream.total_out;
			stream.avail_out = outputLen - stream.total_out;
			continue;
		}

		const int err3 = inflateEnd(&stream);
		if (err2 == Z_STREAM_END && err3 == Z_OK) {
			lua_pushlstring(L, output, stream.total_out);
			free(output);
			return 1;
		}
		free(output);

		if (err2 != Z_STREAM_END) {
			return luaL_error(L, "inflate failed: %d", err2);
		} else {
			return luaL_error(L, "inflateEnd failed: %d", err3);
		}
	}
}

/*
gzip.compress(data) -> (data)
gzip.decompress(data) -> (data)
*/

LUA_API int luaopen_gzip(lua_State *L) {
	lua_newtable(L);

	lua_pushcfunction(L, gzip_compress);
	lua_setfield(L, -2, "compress");
	lua_pushcfunction(L, gzip_decompress);
	lua_setfield(L, -2, "decompress");

	return 1;
}
