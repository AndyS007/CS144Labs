#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    if (!syn_received) {
        if (seg.header().syn) {
            _isn = seg.header().seqno;
            syn_received = true;
        } else {
            return false;
        }
    } else {
        if (seg.header().syn){
            return false;
        }
    }
    if (!fin_received) {
        if (seg.header().fin) {
            fin_received = true;
        }
    } else {
        if (seg.header().fin) {
            return false;
        }
    }
    
    WrappingInt32 seqno = seg.header().seqno;
    string data = seg.payload().copy();
    size_t index = unwrap(seqno, _isn, _stream_index);
    if (index != 0) {
        index -= 1;
    }
    if (out_of_window((seg.header().syn || seg.header().fin), index, data)) {
        return false;
    }
    _reassembler.push_substring(data, index, fin_received);
    _stream_index = _reassembler.expect_seqno();
    _abs_seqno = _stream_index + (syn_received ? 1 : 0) + 
    (fin_received && (unassembled_bytes() == 0) ? 1 : 0);
    return true;
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (!syn_received) {
        return {};
    } else {
        return wrap(_abs_seqno, _isn);
    }
}

size_t TCPReceiver::window_size() const { 
    return _capacity - _reassembler.stream_out().buffer_size();
    }
