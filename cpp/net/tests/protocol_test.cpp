#include <cassert>
#include <iostream>
#include "../protocol.h"

using namespace spatialdb::net;

static void testEncodeDecode() {
    // encode simple string
    auto ok = RespEncoder::encodeOK();
    assert(ok == "+OK\r\n" && "OK encoding failed");

    auto err = RespEncoder::encodeError("bad command");
    assert(err == "-ERR bad command\r\n" && "error encoding failed");

    auto num = RespEncoder::encodeInteger(42);
    assert(num == ":42\r\n" && "integer encoding failed");

    auto arr = RespEncoder::encodeArray({"NEARBY", "vehicles", "37.77", "-122.41", "1"});
    assert(!arr.empty() && "array encoding failed");

    // decode bulk string
    std::string raw = "$6\r\nfoobar\r\n";
    RespDecoder dec(raw);
    auto val = dec.decode();
    assert(val.has_value() && "decode returned nullopt");
    assert(val->type == RespType::BULK_STRING && "wrong type");
    assert(val->str == "foobar" && "wrong value");

    std::cout << "testEncodeDecode: PASS\n";
}

static void testArrayDecode() {
    std::string raw = "*3\r\n$6\r\nNEARBY\r\n$8\r\nvehicles\r\n$4\r\n1.23\r\n";
    RespDecoder dec(raw);
    auto val = dec.decode();
    assert(val.has_value() && "array decode failed");
    assert(val->type == RespType::ARRAY && "expected array");
    assert(val->array.size() == 3 && "expected 3 elements");
    assert(val->array[0].str == "NEARBY" && "first element mismatch");

    std::cout << "testArrayDecode: PASS\n";
}

static void testNullBulk() {
    auto null_str = RespEncoder::encodeNull();
    assert(null_str == "$-1\r\n" && "null encoding failed");

    RespDecoder dec(null_str);
    auto val = dec.decode();
    assert(val.has_value() && "null decode failed");
    assert(val->type == RespType::NULL_BULK && "expected null type");

    std::cout << "testNullBulk: PASS\n";
}

int main() {
    testEncodeDecode();
    testArrayDecode();
    testNullBulk();
    std::cout << "All protocol tests passed.\n";
    return 0;
}
