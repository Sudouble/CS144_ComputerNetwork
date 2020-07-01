#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

#include <iostream>

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    
	const TCPHeader &hdr = seg.header();
	
	if (!_synReceived && hdr.fin)
	{

	}
	else if ((!_synReceived && !hdr.syn))
	{
		return false;	
	}
	
	if (!_synReceived && hdr.syn)
	{
		_synReceived = true;
		_isn = hdr.seqno;
		_ackno = hdr.seqno;
	}
	
	// Segment overflowing the window on both sides is unacceptable.
	if ((hdr.seqno + seg.length_in_sequence_space() - 1 < _ackno) 
		|| (hdr.seqno >= _ackno + window_size())
		|| (hdr.seqno < _ackno && hdr.seqno + seg.length_in_sequence_space() >= _ackno + window_size())
		)
        return false;
	
	// cerr << "seqno:" << hdr.seqno 
		 // << " isn:"  << _isn 
		 // << " unwrap:" << unwrap(hdr.seqno, _isn, _reassembler.stream_out().bytes_written())-1 
		 // << " byteWritten:" << _reassembler.stream_out().bytes_written() << endl;
	
	if (hdr.seqno == _isn && _reassembler.stream_out().bytes_written() == 0)
	{
		_reassembler.push_substring(seg.payload().copy(), 
			unwrap(hdr.seqno, _isn, _reassembler.stream_out().bytes_written()), 
			hdr.fin);
	}
	else
	{
		_reassembler.push_substring(seg.payload().copy(), 
			unwrap(hdr.seqno, _isn, _reassembler.stream_out().bytes_written())-1, 
			hdr.fin);
	}
		
	if (hdr.seqno == _ackno) {
        _ackno = wrap(_reassembler.first_unassembled(), _isn) + 1lu; // 加一是因为byte stream不包括对syn和fin编号。
        if (hdr.fin)
            _ackno = _ackno + 1;
    }
	
    return true;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
	if (_synReceived)
        return _ackno;
    return optional<WrappingInt32>{};
}

size_t TCPReceiver::window_size() const { return _reassembler.stream_out().remaining_capacity(); }
