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
    size_t seg_abs_seqno = unwrap(seqno, _isn, _stream_index);
    if (out_of_window(seg.length_in_sequence_space(), seg_abs_seqno)) {
        return false;
    }
    size_t index = seg_abs_seqno;
    if (index > 0) {
        index--;
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

bool TCPReceiver::out_of_window(size_t len, size_t seg_abs_seqno) {
    return (seg_abs_seqno >= (_abs_seqno + window_size())) ||
    (seg_abs_seqno + len <= _abs_seqno);
    // return (index >= _first_unaccep) || ((index + data.length() ) <= _first_unassem);
    // return _reassembler.isNotacc(index, data);
}

size_t TCPReceiver::window_size() const { 
    return _capacity - _reassembler.stream_out().buffer_size();
    }
