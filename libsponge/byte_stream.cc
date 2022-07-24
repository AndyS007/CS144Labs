#include "byte_stream.hh"

#include <algorithm>
#include <iterator>
#include <stdexcept>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _capacity(capacity){}

size_t ByteStream::write(const string &data) {
    // _input_ended_flag = false;
    size_t canWrite = _capacity - buffer_size();
    size_t realWrite = min(canWrite, data.length());
    for (size_t i = 0; i < realWrite; i++) {
        _buffer.push_back(data[i]);
    }
    _write_count += realWrite;
    return  realWrite;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t stringLen = len > buffer_size() ? buffer_size() : len;
    return string().assign(_buffer.begin(), _buffer.begin() + stringLen);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    size_t actualLen = len > buffer_size() ? buffer_size() : len;
    for (size_t i = 0; i < actualLen; i++) {
        _buffer.pop_front();
        _read_count += 1;
    }
 }

void ByteStream::end_input() { 
    _input_ended_flag = true;
}

bool ByteStream::input_ended() const { 
    return _input_ended_flag;
}

size_t ByteStream::buffer_size() const { return _buffer.size(); }

bool ByteStream::buffer_empty() const { return _buffer.empty(); }

bool ByteStream::eof() const { return buffer_empty() && input_ended(); }

size_t ByteStream::bytes_written() const { return _write_count; }

size_t ByteStream::bytes_read() const { return _read_count; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer_size(); }
