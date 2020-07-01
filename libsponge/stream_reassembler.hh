#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <string>
#include <queue>
#include <map>

typedef struct ST_TCP_SEGMENT{
	uint64_t _index = 0;
	std::string str = "";
	bool _eof = false;
	
	bool operator>(const ST_TCP_SEGMENT &right) const
	{
		if (this->_index > right._index)
			return true;
		return false;
	}
}ST_SEGMENT;



//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.	
	std::priority_queue<ST_SEGMENT, std::vector<ST_SEGMENT>, std::greater<ST_SEGMENT> > _PR_queue;
	uint64_t _current_waiting_index;
	uint64_t _remain_bytes;
	std::map<uint64_t, bool> _map_nottouched_info; // true is represented as not reassembled.
	

    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receives a substring and writes any newly contiguous bytes into the stream.
    //!
    //! If accepting all the data would overflow the `capacity` of this
    //! `StreamReassembler`, then only the part of the data that fits will be
    //! accepted. If the substring is only partially accepted, then the `eof`
    //! will be disregarded.
    //!
    //! \param data the string being added
    //! \param index the index of the first byte in `data`
    //! \param eof whether or not this segment ends with the end of the stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been submitted twice, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
	
	size_t first_unassembled() { return _current_waiting_index; }
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
