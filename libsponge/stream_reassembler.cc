#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

#include <iostream>
#include <algorithm>

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) :
	_PR_queue(),	
	_current_waiting_index(0),
	_remain_bytes(0),
	_map_nottouched_info(),
	_output(capacity), 
	_capacity(capacity)
{
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
	if(_output.input_ended())
		return;
	
	// to prQueue;
	ST_SEGMENT stSegment;
	stSegment._index = index;
	stSegment.str = data;
	stSegment._eof = eof;
	
	_PR_queue.push(stSegment);	
		
	// deal with overlapping by using "_vec_nottouched_info"
	size_t size_end = (index+data.size());
	for (size_t begin_index = index; begin_index < size_end; ++begin_index)
	{
		_map_nottouched_info[begin_index] = true;
		
		// cerr << "size:" << _map_nottouched_info.size() << " ,data:" << data << endl;
	}
	_remain_bytes = _map_nottouched_info.size();
	// deal with overlapping end
	
	if (_PR_queue.size() == 0 || _output.input_ended())
		return;	
	
	ST_SEGMENT stSg = _PR_queue.top();
	
	//cerr << "index:" << stSg._index << " currentIndex:" << index << " _:"<< _current_waiting_index << endl;
	while (_PR_queue.size() > 0 && stSg._index <= _current_waiting_index)
	{
		// deal with dup		
		
		uint64_t offset = _current_waiting_index - stSg._index;
		
		// cerr << "current:" << stSg.str << " ,offset:" << offset;
		if (offset < stSg.str.size())
		{
			string str_write = stSg.str.substr(offset);
			size_t size_just_written = _output.write(str_write);
			
			_current_waiting_index = _output.bytes_written();
			_remain_bytes -= size_just_written;
			
			// cerr << " remain:" << _remain_bytes << " written:" << size_just_written << endl;
						
			// deal with overlapping 
			for (size_t begin_index = stSg._index; begin_index < (stSg._index+size_just_written); ++begin_index)
			{
				_map_nottouched_info.erase(begin_index);
				
				//cerr << "erased:" << begin_index << endl;
			}
			// deal with overlapping end
		}
		// deal with dup end
		
		if (stSg._eof)
			_output.end_input();		
				
		_PR_queue.pop();
		if (_remain_bytes == 0)
			break;
		
		// look next
		stSg = _PR_queue.top();
	}
	
	// cerr << "unassembled_bytes:" << unassembled_bytes() << " size:" << _map_nottouched_info.size() << endl;
}

size_t StreamReassembler::unassembled_bytes() const { 
	return {_remain_bytes}; 
}

bool StreamReassembler::empty() const { 
	return {_PR_queue.size() == 0}; 
}
