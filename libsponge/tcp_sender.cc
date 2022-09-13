#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , RTO(retx_timeout) { }

uint64_t TCPSender::bytes_in_flight() const {
    return _bytes_in_flight; 
    }

void TCPSender::fill_window() {
    if (!syn_sent) {
        // send syn
        bool syn = true;
        bool fin = false;
        bool ack = false;
        TCPSegment synSegment = make_segment(fin, syn, ack, std::nullopt);
        send_segment(synSegment);
        syn_sent = true;
        return;
    } 
    // if syn not acked, do nothing.
    if ((!_segments_out.empty()) && (_segments_out.front().header().syn == true)) {
        return;
    }
    // If _stream is empty but input has not ended, do nothing.
    if (_stream.buffer_empty() && !_stream.eof())
        // Lab4 behavior: if incoming_seg.length_in_sequence_space() is not zero, send ack.
        return;
    
    if (fin_sent) {
        return;
    }
    if (_rev_window_size > 0) {
        while (_free_space > 0) {

        // 1. reading from the ByteStream
        // 2. creating new TCP segments()
        // when you find _next_seqno == rwindow, you should check the value of windowsize.
        size_t payload_size = min({TCPConfig::MAX_PAYLOAD_SIZE, static_cast<uint64_t>(_free_space),  stream_in().buffer_size()});
        std::string data = stream_in().read(payload_size);
        bool fin = false;
        if (_stream.eof()) {
           fin = true; 
        } 
        TCPSegment seg = make_segment(fin, false, false, data);
        send_segment(seg);
        if (_stream.buffer_empty()) {
            break;
        }
        }
    } else if (_free_space == 0) {
        if (_stream.eof()) {
            TCPSegment seg;
            seg.header().fin = true;
            send_segment(seg);
            
        } else {
            TCPSegment seg;
            seg.payload() = std::move(_stream.read(1));
            send_segment(seg);
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t old_abs_ackno = _abs_ackno;
    uint64_t tmp_abs_ackno = unwrap(ackno, _isn, old_abs_ackno);
    if (tmp_abs_ackno > _next_seqno) {
        return false;
    } else if (tmp_abs_ackno < _abs_ackno) {
        return true;
    }
    _abs_ackno = tmp_abs_ackno; 
    _rev_window_size = window_size;
    rwindow = _abs_ackno + _rev_window_size;
    _free_space = rwindow - _next_seqno;
    //  do with outstanding segments
    while(!_outstanding_segments.empty()) {
        TCPSegment tmp = _outstanding_segments.front();
        uint64_t tmp_abs_seqno = unwrap(tmp.header().seqno, _isn, old_abs_ackno);
        if ((tmp.length_in_sequence_space() + tmp_abs_seqno) <= _abs_ackno) {
            _bytes_in_flight -= tmp.length_in_sequence_space();
            _outstanding_segments.pop();
        } else {
            break;
        }
    }
    if (!_outstanding_segments.empty()) {
        timer.start();
    }
    RTO = _initial_retransmission_timeout;
    _consecutive_retransmission = 0;
    return true;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    timer.update(ms_since_last_tick);
    if (timer.isTimeOut(RTO)){
        if (_rev_window_size != 0) {
        // double RTO
        RTO *= 2;
        _consecutive_retransmission += 1;
        }
        // retransmit oldest-sent outstanding segment if time out
        TCPSegment oldest_seg = _outstanding_segments.front();
        segments_out().push(oldest_seg);
        if (!timer.isRunning()) {
            timer.start();
        }
    }
    }

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmission; }

void TCPSender::send_empty_segment() {
    TCPSegment empty_seg;
    empty_seg.header().seqno = next_seqno();
    segments_out().push(empty_seg);
}

void TCPSender::send_segment(TCPSegment& seg) {
    segments_out().push(seg);
    _outstanding_segments.push(seg);
    _next_seqno += seg.length_in_sequence_space();
    _bytes_in_flight += seg.length_in_sequence_space();
    _free_space -= seg.length_in_sequence_space();
    if (seg.header().fin) {
        fin_sent = true;
    }
    if (!timer.isRunning()) {
        timer.start();
    }
} 

TCPSegment TCPSender::make_segment(bool fin, bool syn, bool ack, std::optional<std::string> data) {
    TCPSegment seg;
    seg.header().syn = syn;
    seg.header().fin = fin;
    seg.header().ack = ack;
    seg.header().seqno = next_seqno();
    if (data.has_value()) {
        seg.payload() = std::move(data.value());
    }
    return seg;
}

