require 'lunit'
local GZIP = require 'gzip'

module('testcase', lunit.testcase, package.seeall)

math.randomseed(os.time())

function generateBinaryString(length)
	local result = {}
	for i = 1, length do
		table.insert(result, string.char(math.random(0, 255)))
	end
	return table.concat(result)
end

function string:multiply(count)
	local result = {}
	for i = 1, count do
		table.insert(result, self)
	end
	return table.concat(result)
end

function testCompressDecompress()
	local data = generateBinaryString(23):multiply(11)
	lunit.assert_equal(23*11, #data)

	local compressed = GZIP.compress(data)
	lunit.assert_string(compressed)
	lunit.assert_true(#compressed > 23 and #compressed < #data)

	local decompressed = GZIP.decompress(compressed)
	lunit.assert_equal(data, decompressed)
end

function testCompressDecompressRandom()
	local data = generateBinaryString(100)

	local compressed = GZIP.compress(data)
	lunit.assert_string(compressed)
	lunit.assert_true(#compressed > #data)

	local decompressed = GZIP.decompress(compressed)
	lunit.assert_equal(data, decompressed)
end

function testCompressDecompressNothing()
	local data = ''

	local compressed = GZIP.compress(data)
	lunit.assert_string(compressed)
	lunit.assert_true(#compressed > 0)

	local decompressed = GZIP.decompress(compressed)
	lunit.assert_equal(data, decompressed)
end

function testDecompressNothing()
	work = function()
		local decompressed = GZIP.decompress('')
	end
	local status, err = pcall(work)
	lunit.assert_false(status)
end

function testDecompressInvalid()
	work = function()
		local data = generateBinaryString(11)
		local decompressed = GZIP.decompress(data)
	end
	local status, err = pcall(work)
	lunit.assert_false(status)
end

lunit.main()
