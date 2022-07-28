#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"
#include <algorithm>
#include <cstdint>
#include <string>
using std::string;

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.
    size_t _first_unaccep;
    size_t _first_unassem;
    size_t _unass_bytes_size;
    bool _eof_flag;
    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes
    std::deque<char> _unassembled_buffer;
    std::deque<bool> _unassembled_valid;

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receives a substring and writes any newly contiguous bytes into the stream.
    //!
    //! If accepting all the data would overflow the `capacity` of this
    //! `StreamReassembler`, then onlysize_t first_unass the part of the data that fits will be
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
    bool isFull() const;
    bool isNotacc(size_t index, const string& data);
    void writeToUnAssemBuffer(size_t index, const string& data);
    std::pair<size_t, std::string> cutString(const string&data, const size_t index);
    void write_contiguous();
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
