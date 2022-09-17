#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::unclean_shutdown() {
        // set both stream to error state
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        // make sure active() return false;
        _active = false;
}
bool TCPConnection::in_listen() {
    return !_receiver.ackno().has_value();
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    //  update time since last segment received
    _time_since_last_segment_received = 0;

    if (seg.header().rst) {
        unclean_shutdown();
        return;
    }
    // give segment to receiver
    // bool _segment_recevied =  _receiver.segment_received(seg);
    _receiver.segment_received(seg);
        // syn not received, still listening
    if (in_listen()) {
        return;
    }
    // inform sender about the ackno and window size if ack is set.
    // if (_segment_recevied && seg.header().ack) {
    if (seg.header().ack) {

        _sender.ack_received(seg.header().ackno, seg.header().win);
        _sender.fill_window();
        real_send();
        // if ack > current seqno, send true ack back to the peer.
        // if (!valid) {
        //     _sender.send_empty_segment();
        //     real_send();
        // }
    }
    // Lab4 behavior: if incoming_seg.length_in_sequence_space() is not zero, send ack.
    
    // if the incoming segment occupied any sequence numbers, the TCPConnection makes sure that at least one segment is
    // sent in reply, to reflect an update in the ackno and window size.
    if (seg.length_in_sequence_space() > 0) {
        _sender.fill_window();
        bool isSend = real_send();
        if (!isSend) {
            // send ack segment
            _sender.send_empty_segment();
            TCPSegment ackSeg = _sender.segments_out().front();
            _sender.segments_out().pop();
            set_ack_win(ackSeg);
            _segments_out.push(ackSeg);
        }
    }
    // check if inbound stream ended
    if (inbound_stream().eof() && (!_sender.stream_in().input_ended())) {
        _linger_after_streams_finish = false;
    }
    // passive close
    if (!_linger_after_streams_finish && check_outbound_ended() ) {
        _active = false;
    }
}

bool TCPConnection::active() const { 
    return _active;
}

size_t TCPConnection::write(const string &data) {
    if (remaining_outbound_capacity() > 0) {
        size_t realWritten = _sender.stream_in().write(data);
        _sender.fill_window();
        real_send();
        return realWritten;

    } else {
        return 0;
    }
}
void TCPConnection::set_ack_win(TCPSegment& seg) {
    if (_receiver.ackno().has_value()) {
        // if (not seg.header().ack) {
        seg.header().ack = true;
        seg.header().ackno = _receiver.ackno().value();
        // }
    }
    size_t win_size = _receiver.window_size();
    seg.header().win = static_cast<int16_t>(win_size); 
}

// ack is included in this action
bool TCPConnection::real_send() {
    bool isSend = false;
    while (!_sender.segments_out().empty()) {
        isSend = true;
        TCPSegment seg = _sender.segments_out().front();
        set_ack_win(seg);
        _segments_out.push(seg);
        _sender.segments_out().pop();
    }
    return isSend;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _time_since_last_segment_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if (_sender.segments_out().size() > 0) {
        if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
            _sender.segments_out().front().header().rst = true;
            unclean_shutdown();
        }
        real_send();
    }
    // check if done
    if (check_inbound_ended() && check_outbound_ended() &&
        (_time_since_last_segment_received >= 10 * _cfg.rt_timeout)) {
        _active = false;
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    real_send();
}
// prereqs1 : The inbound stream has been fully assembled and has ended.
bool TCPConnection::check_inbound_ended() const {
    return _receiver.stream_out().eof();
}
// prereqs2 : The outbound stream has been ended by the local application and fully sent (including
// the fact that it ended, i.e. a segment with fin ) to the remote peer.
// prereqs3 : The outbound stream has been fully acknowledged by the remote peer.
bool TCPConnection::check_outbound_ended() const {
    return _sender.bytes_in_flight()== 0 &&
    _sender.stream_in().eof() && 
    _sender.next_seqno_absolute() == (_sender.stream_in().bytes_written() + 2);
}
void TCPConnection::connect() {
    _sender.fill_window();
    real_send();
}
void TCPConnection::send_RST() {
    _sender.send_empty_segment();
    TCPSegment RSTSeg = _sender.segments_out().front();
    _sender.segments_out().pop();
    set_ack_win(RSTSeg);
    RSTSeg.header().rst = true;
    _segments_out.push(RSTSeg);
}
TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            send_RST();
            unclean_shutdown();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
