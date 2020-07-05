#include "tcp_connection.hh"

#include <random>
#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { 
	return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const { 
	return _sender.bytes_in_flight(); 
}

size_t TCPConnection::unassembled_bytes() const {
	return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const {
	return _tick_time - _last_seg_recv_time;
}

void TCPConnection::segment_received(const TCPSegment &seg) { 
	_last_seg_recv_time = _tick_time;
	
	const TCPHeader &header = seg.header();
	// deal with reset flag
	if (header.rst)
	{
		_sender.stream_in().set_error();
		_receiver.stream_out().set_error();
	}
	else
	{
		bool recv_result = _receiver.segment_received(seg);
		bool ack_result = false;
		bool has_ack_result = false;
		if (recv_result == true)
		{			
			if (header.ack == true)
			{
				// deal ack
				ack_result = _sender.ack_received(header.ackno, header.win);				
				has_ack_result = true;
			}
			else
			{
				WrappingInt32 isn = _cfg.fixed_isn.value_or(WrappingInt32{random_device()()});
				// send ack
				TCPSegment segAck;
				segAck.header().ack = true;
				segAck.header().ackno = wrap(_sender.next_seqno_absolute(), isn);
				
				_segments_out.push(segAck);
			}
		}
		
		if (not recv_result || (not ack_result && has_ack_result))
		{
			TCPSegment segRev;
			segRev.header().ackno = header.ackno;
			segRev.header().win = header.win;
			
			_segments_out.push(segRev);
		}
	}
	
	test_end();
}

bool TCPConnection::active() const {
	// check byteStream of sender and reciever
	if (_sender.stream_in().error() || _receiver.stream_out().error())
		return false;
	
	if (_sender.stream_in().input_ended() 
		&& _receiver.stream_out().input_ended()
		&& _sender.bytes_in_flight() == 0) // all acked
		{
			return false;
		}
	
	// linger connection
	return _linger_after_streams_finish;
}

size_t TCPConnection::write(const string &data) {
	
	if (_sender.consecutive_retransmissions() >= TCPConfig::MAX_RETX_ATTEMPTS)
	{
		// send reset, but will the recv deal this?
		TCPSegment seg;
		
		WrappingInt32 isn = _cfg.fixed_isn.value_or(WrappingInt32{random_device()()});
		seg.header().seqno = wrap(_sender.next_seqno_absolute(), isn);
		seg.header().rst = true;
		_segments_out.push(seg);
		
		return seg.length_in_sequence_space();
	}
	
    size_t n_size_write = _sender.stream_in().write(data);
	_sender.fill_window();
	
	size_t n_size_count = 0;
	while(!_sender.segments_out().empty())
	{
		TCPSegment segment = _sender.segments_out().front();
		n_size_count += segment.length_in_sequence_space();
		
		_segments_out.push(segment);
		_sender.segments_out().pop();
	}
	
	test_end();
	
    return n_size_write;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
	_tick_time += ms_since_last_tick;
	
	_sender.tick(ms_since_last_tick);
	
	while(!_sender.segments_out().empty())
	{
		TCPSegment segment = _sender.segments_out().front();
		
		_segments_out.push(segment);
		_sender.segments_out().pop();
	}
}

void TCPConnection::end_input_stream() {
	_sender.stream_in().end_input();
	_sender.fill_window();
	
	while(!_sender.segments_out().empty())
	{
		TCPSegment segment = _sender.segments_out().front();
		
		_segments_out.push(segment);
		_sender.segments_out().pop();
	}
	
	// _linger_after_streams_finish
	if (_receiver.stream_out().input_ended() && _sender.stream_in().eof())
	{
		_linger_after_streams_finish = false;
	}
}

void TCPConnection::connect() {
	if (not _sync_sent)
	{
		_sync_sent = true;
		
		_sender.fill_window();
		while(!_sender.segments_out().empty())
		{
			TCPSegment segment = _sender.segments_out().front();
			
			_segments_out.push(segment);
			_sender.segments_out().pop();
		}
	} else {
		// say connect already exists.
	}
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
			WrappingInt32 isn = _cfg.fixed_isn.value_or(WrappingInt32{random_device()()});
			
			TCPSegment seg;
			seg.header().seqno = wrap(_sender.next_seqno_absolute(), isn);
			seg.header().rst = true;
			_segments_out.push(seg);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::test_end()
{
	if (_linger_after_streams_finish 
		&& _sender.stream_in().input_ended() 
		&& _receiver.stream_out().input_ended()
		&& _sender.bytes_in_flight() == 0
		&& time_since_last_segment_received() > 10*_cfg.rt_timeout)
	{
		_linger_after_streams_finish = false;
	}
}