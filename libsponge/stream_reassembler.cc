#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

StreamReassembler::StreamReassembler(const size_t capacity)
    : _first_unaccep(capacity)
    , _first_unassem(0)
    , _first_unread(0)
    , _unass_bytes_size(0)
    , _eof_flag(false)
    , _output(capacity)
    , _capacity(capacity)
    , _unassembled_buffer(capacity, '\0')
    , _unassembled_valid(capacity, false) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {

    if (eof) {
        _eof_flag = true;
    }
    // if no more bytes can be received, just drop the substring.
    if (isFull() || isNotacc(index, data)) {
        if (_eof_flag && empty()) {
            _output.end_input();
        }
        return;
    }
    // Renew the base of sliding window
    updateWindowBound();
    // process duplicate and overlap string
    auto [newIndex, newData] = cutString(data, index);
    //write to buffer
    size_t bytesWritten = writeToUnAssemBuffer(newIndex, newData);
    //update unassembled bytes
    _unass_bytes_size += bytesWritten;
    write_contiguous();
    if (_eof_flag && empty()) {
        _output.end_input();
    }
}
void StreamReassembler::write_contiguous() {
    string s;
    while (_unassembled_valid.front() != false) {
        s.push_back(_unassembled_buffer.front());
        _unassembled_buffer.pop_front();
        _unassembled_valid.pop_front();
        _unassembled_buffer.push_back('\0');
        _unassembled_valid.push_back(false);
        _unass_bytes_size--;
        _first_unassem++;
    }
    _output.write(s);
}

void StreamReassembler::updateWindowBound() {
    _first_unread += _output.bytes_read();
    // _first_unaccep = _first_unread + _capacity;
    _first_unaccep += _output.bytes_read();
}
bool StreamReassembler::isNotacc(size_t index, const string& data){
    return (index > _first_unaccep) || ((index + data.length() ) <= _first_unassem);
}
bool StreamReassembler::isFull() const {
    return (_output.buffer_size() +  unassembled_bytes() == _capacity);
}

size_t StreamReassembler::unassembled_bytes() const {
    return _unass_bytes_size;
}
size_t StreamReassembler::writeToUnAssemBuffer(size_t index, const string& s) {
    // index must bigger or equal than first_unassem
    size_t offset = index - _first_unassem;
    // size_t len = s.length();
    size_t bytesWritten = 0;
    for (const char& c : s) {
        // false means empty
        if (_unassembled_valid[offset] == false) {
        // every time buffer is written should set flag to true
        // while flag set to false when buffer is read
        _unassembled_buffer[offset] = c;
        _unassembled_valid[offset] = true;       
        bytesWritten += 1;
        }
        offset += 1;
    }
    return bytesWritten;
    
}

std::pair<size_t, std::string> StreamReassembler::cutString(const string &data, const size_t index) {
    int offset = 0;
    size_t newIndex = index;
    string newString = data;
    if (index < _first_unassem) {
        offset = _first_unassem - index;
        newIndex += offset;
        newString = string().assign(data.begin() + offset, data.end());
    }
    return {newIndex, newString};

}


bool StreamReassembler::empty() const { return unassembled_bytes() == 0; }
