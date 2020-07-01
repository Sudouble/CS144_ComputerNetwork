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

ByteStream::ByteStream(const size_t capacity) :
    m_capacity(capacity),
	m_byte_written(0),
	m_byte_read(0)
{ 
}

size_t ByteStream::write(const string &data) {
	if (input_ended())
		return 0;
	
    size_t len = data.length();

    size_t can_write = (remaining_capacity() >= len)? len : remaining_capacity();
    
	str_buffer += data.substr(0, can_write);
	m_capacity -= can_write;
	
    m_byte_written += can_write;

    return can_write;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    return str_buffer.substr(0, len);;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 

    size_t can_read = (buffer_size() >= len) ? len : buffer_size();

    str_buffer = str_buffer.substr(can_read);

	m_capacity += can_read;
    m_byte_read += can_read;
	
	if (buffer_empty() && input_ended())
        _eof = true;
}

void ByteStream::end_input() {
	b_cannot_write = true;
	
	if (buffer_empty())
		_eof = true;
}

bool ByteStream::input_ended() const { 
    return b_cannot_write; 
}

size_t ByteStream::buffer_size() const {
    return str_buffer.size();
}

bool ByteStream::buffer_empty() const { 
    return str_buffer.size() == 0;
}

bool ByteStream::eof() const { return _eof; }

size_t ByteStream::bytes_written() const { return {m_byte_written}; }

size_t ByteStream::bytes_read() const { return {m_byte_read}; }

size_t ByteStream::remaining_capacity() const { return m_capacity; }
