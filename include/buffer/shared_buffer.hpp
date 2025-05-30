/** @mainpage Reference Counted Byte Buffer Classes, Const and Mutable Versions
 *
 * ## Overview
 *
 * The @c shared_buffer classes provide byte buffers with internal reference counting. 
 * These classes can be used within asynchronous IO / networking applications where the 
 * lifetime of the buffer must be kept alive until a requested IO operation completes.
 *
 * For example, a network write is started asynchronously, which means a write request
 * is started, and the write completion will happen at a later time. The byte buffer
 * must be kept alive until the completion notification. Meanwhile multiple other
 * write requests can happen simultaneously (even on the same thread). For reads, a
 * read request can be started, and the byte buffer used for incoming data will be
 * kept alive until a read request completes - i.e. data arrives.
 *
 * In networking applications, this allows multiple sockets to be performing reads 
 * and writes under one thread (or multiple threads, such as in a thread pool).
 * In particular, it eliminates the (sometimes inefficient) design of "one thread
 * per socket".
 *
 * The Connective C++ Chops Net IP library is an asynchronous networking library (using
 * Asio underneath) and it uses @c shared_buffer classes for buffer lifetime management. 
 *
 * There are two concrete classes, @c mutable_shared_buffer and @c const_shared_buffer.
 * @c mutable_shared_buffer is a reference counted modifiable buffer class with 
 * convenience methods for appending data. @c const_shared_buffer is a reference counted 
 * non-modifiable buffer class. Once the object is constructed, it cannot be modified. 
 *
 * A @c const_shared_buffer can be efficiently constructed (no buffer copies, only
 * pointer assignments through move construction) from a @c mutable shared_buffer. This 
 * allows the use case of serializing data into a @c mutable_shared_buffer then 
 * constructing a @c const_shared_buffer for writing to the network.
 *
 * Besides the data buffer lifetime management, these utility classes eliminate data 
 * buffer copies and (obviously) can be utilized in use cases other than networking
 * (for example reading and writing disk files).
 *
 * ### Additional Details
 *
 * Internally all data is stored in a @c std::vector of @c std::byte. There are 
 * convenience templated constructors so that the @c shared_buffer objects can
 * be constructed from traditional byte buffers, such as @c char @c *.
 *
 * There are ordering methods so that shared buffer objects can be stored in 
 * sequential or associative containers.
 *
 * Efficient moving of data (versus copying) is enabled in multiple ways, including
 * allowing a @c const_shared_buffer to be move constructed from a 
 * @c mutable_shared_buffer, and allowing a @c std::vector of @c std::byte to be moved 
 * into either @c shared_buffer type.
 *
 * The implementation is adapted from Chris Kohlhoff's reference counted buffer 
 * examples in the Asio library. It has been significantly modified by adding a 
 * @c mutable_shared_buffer class as well as adding convenience methods to the 
 * @c const_shared_buffer class.
 *
 * @note Everything is declared @c noexcept except for the methods that allocate
 * memory and might throw a memory exception. This is tighter than the @c noexcept
 * declarations on the underlying @c std::vector methods, since @c std::byte 
 * operations will never throw.
 *
 * @authors Cliff Green, Chris Kohlhoff
 *
 * @copyright (c) 2017-2025 by Cliff Green
 *
 * Distributed under the Boost Software License, Version 1.0. 
 * (See accompanying file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#ifndef SHARED_BUFFER_HPP_INCLUDED
#define SHARED_BUFFER_HPP_INCLUDED

#include <cstddef> // std::byte
#include <vector>
#include <memory> // std::shared_ptr
#include <compare> // spaceship operator
#include <span>
#include <bit> // std::bit_cast

#include <utility> // std::move, std::swap
#include <algorithm> // std::copy

// TODO - add concepts and / or requires
//
// modify the templated constructor that takes a buffer of any valid 
// byte type, add constraints which makes the casting safer

namespace chops {

class const_shared_buffer;

/**
 * @brief A mutable (modifiable) byte buffer class with convenience methods, internally 
 * reference-counted for efficient copying and lifetime management.
 *
 * This class provides ownership, copying, and lifetime management for byte oriented 
 * buffers. In particular, it is designed to be used in conjunction with the 
 * @c const_shared_buffer class for efficient transfer and correct lifetime management 
 * of buffers in asynchronous libraries (such as Asio). In particular, 
 * a reference counted buffer can be passed among multiple layers of software without 
 * any one layer "owning" the buffer.
 *
 * A @c std::byte pointer returned by the @c data method may be invalidated if the 
 * @c mutable_shared_buffer is modified in any way (this follows the usual constraints
 * on @c std::vector iterator invalidation).
 *
 * This class is similar to @c const_shared_buffer, but with mutable characteristics.
 *
 * @invariant There will always be an internal buffer of data, even if the size is zero.
 *
 * @note Modifying the underlying buffer of data (for example by writing bytes using the 
 * @c data method, or appending data) will show up in any other @c mutable_shared_buffer 
 * objects that have been copied to or from the original object.
 *
 */

class mutable_shared_buffer {
public:
  using byte_vec = std::vector<std::byte>;
  using size_type = typename byte_vec::size_type;

private:
  std::shared_ptr<byte_vec> m_data;

private:

  friend class const_shared_buffer;

  friend bool operator==(const mutable_shared_buffer&, const const_shared_buffer&) noexcept;
  friend bool operator==(const const_shared_buffer&, const mutable_shared_buffer&) noexcept;

public:

  // default copy and move construction, copy and move assignment
  mutable_shared_buffer(const mutable_shared_buffer&) = default;
  mutable_shared_buffer(mutable_shared_buffer&&) = default;
  mutable_shared_buffer& operator=(const mutable_shared_buffer&) = default;
  mutable_shared_buffer& operator=(mutable_shared_buffer&&) = default;

/**
 * @brief Default construct the @c mutable_shared_buffer.
 *
 */
  mutable_shared_buffer() noexcept : 
      m_data{std::make_shared<byte_vec>(size_type(0))} { }

/**
 * @brief Construct by copying from a @c std::span of @c std::byte.
 *
 * @param sp @c std::byte span pointing to buffer of data. The data is
 * copied into the internal buffer of the @c mutable_shared_buffer. 
 *
 */
  template <std::size_t Ext>
  explicit mutable_shared_buffer(std::span<const std::byte, Ext> sp) : 
      m_data{std::make_shared<byte_vec>(sp.data(), sp.data()+sp.size())} { }

/**
 * @brief Construct by copying from a @c std::byte array.
 *
 * A @c std::span is first created, then the constructor taking
 * a @c std::span is called.
 *
 * @pre Size cannot be greater than the source buffer.
 *
 * @param buf Non-null pointer to a @c std::byte buffer of data. The 
 * data is copied into the internal buffer of the @c mutable_shared_buffer.
 *
 * @param sz Size of buffer.
 *
 */
  mutable_shared_buffer(const std::byte* buf, std::size_t sz) : 
      mutable_shared_buffer(std::as_bytes(std::span<const std::byte>{buf, sz})) { }

/**
 * @brief Move construct from a @c std::vector of @c std::bytes.
 *
 * Efficiently construct from a @c std::vector of @c std::bytes by moving
 * into a @c mutable_shared_buffer.
 *
 * @note The @c std::byte @c std::vector passed in will be left in a
 * "moved from" state (as it typical with move operations).
 *
 */
  explicit mutable_shared_buffer(byte_vec&& bv) noexcept : 
      m_data{std::make_shared<byte_vec>(size_type(0))} {
    *m_data = std::move(bv);
  }

/**
 * @brief Construct a @c mutable_shared_buffer with an initial size, contents
 * of each byte set to zero.
 *
 * Allocate zero initialized space which can be overwritten with data as needed.
 * The @c data method is called to get access to the underlying @c std::byte 
 * buffer.
 *
 * @param sz Size for internal @c std::byte buffer.
 */
  explicit mutable_shared_buffer(size_type sz) : 
      m_data{std::make_shared<byte_vec>(sz)} { }


/**
 * @brief Construct by copying bytes from a @c std::span.
 *
 * The type of the span must be convertible to or be layout compatible with
 * @c std::byte.
 *
 * @param sp @c std::span pointing to buffer of data. The @c std::span 
 * pointer is cast into a @c std::byte pointer and bytes are then copied. 
 *
 */
  template <typename T, std::size_t Ext>
  mutable_shared_buffer(std::span<const T, Ext> sp) : 
      mutable_shared_buffer(std::as_bytes(sp)) { }

/**
 * @brief Construct by copying bytes from an arbitrary pointer.
 *
 * The pointer passed into this constructor is cast into a @c std::byte pointer and bytes 
 * are then copied. In particular, this method can be used for @c char pointers, 
 * @c unsigned @c char pointers, @c std::uint8_t pointers, etc. Non character types that
 * are trivially copyable are also allowed, although the usual care must be taken
 * (padding bytes, alignment, etc).
 *
 * @pre Size cannot be greater than the source buffer.
 *
 * @param buf Non-null pointer to a contiguous array of data. 
 *
 * @param num Number of elements in the array.
 *
 * @note For @c void pointers, see specific constructor taking a @c void pointer.
 */
  template <typename T>
  mutable_shared_buffer(const T* buf, size_type num) : 
      mutable_shared_buffer(std::as_bytes(std::span<const T>{buf, num})) { }

/**
 * @brief Construct by copying bytes from a void pointer.
 *
 * The pointer passed into this constructor is cast into a @c std::byte pointer and bytes 
 * are then copied.
 *
 * @pre Size cannot be greater than the source buffer.
 *
 * @param buf Non-null @c void pointer to a buffer of data. 
 *
 * @param sz Size of buffer, in bytes.
 */
  mutable_shared_buffer(const void* buf, size_type sz) : 
      mutable_shared_buffer(std::as_bytes(
          std::span<const std::byte>{std::bit_cast<const std::byte*>(buf), sz})) { }

/**
 * @brief Construct from input iterators.
 *
 * @pre Valid iterator range, where each element is convertible to a @c std::byte.
 *
 * @param beg Beginning input iterator of range.
 * @param end Ending input iterator of range.
 *
 */
  template <typename InIt>
  mutable_shared_buffer(InIt beg, InIt end) : 
      m_data(std::make_shared<byte_vec>(beg, end)) { }

/**
 * @brief Return @c std::byte pointer to beginning of buffer.
 *
 * This method provides pointer access to the beginning of the buffer. If the
 * buffer is empty the pointer cannot be dereferenced or undefined behavior will 
 * occur.
 *
 * Accessing past the end of the internal buffer (as defined by the @c size() 
 * method) results in undefined behavior.
 *
 * @return @c std::byte pointer to buffer.
 */
  std::byte* data() noexcept { return m_data->data(); }

/**
 * @brief Return @c const @c std::byte pointer to beginning of buffer.
 *
 * @c const method providing pointer access to the beginning of the buffer.
 *
 * @return @c const @c std::byte pointer to buffer.
 */
  const std::byte* data() const noexcept { return m_data->data(); }

/**
 * @brief Return size (number of bytes) of buffer.
 *
 * @return Size of buffer, which may be zero.
 */
  size_type size() const noexcept { return m_data->size(); }

/**
 * @brief Return access to underlying @c std::vector.
 *
 * This can be used to instantiate a @c dynamic_buffer as defined in the Networking TS
 * or Asio API. Changing the @c std::vector from outside this class works because no 
 * state data is stored within this object that needs to be consistent with the 
 * @c std::vector contents.
 *
 * @return Reference to @c std::vector<std::byte>.
 */
  byte_vec& get_byte_vec() noexcept { return *m_data; }

/**
 * @brief Query to see if size is zero.
 *
 * @return @c true if empty (size equals zero).
 */
  bool empty() const noexcept { return m_data->empty(); }

/**
 * @brief Clear the internal contents back to an empty state.
 *
 * This method is handy after a @c mutable_shared_buffer has been moved into 
 * another object (e.g. a @c const_shared_buffer). At that point the contents
 * are in a consistent but unknown state. Calling @c clear puts the internal
 * buffer into a known and empty state.
 *
 */
  void clear() noexcept { m_data->clear(); }

/**
 * @brief Resize internal buffer.
 *
 * @param sz New size for buffer. If the buffer is expanded, new bytes are added,
 * each zero initialized. The size can also be contracted. @c resize does not 
 * destroy old data in the internal buffer, so @c clear may need to be called first.
 *
 * Resizing to zero results in an empty buffer, although calling @c clear is 
 * preferred.
 */
  void resize(size_type sz) { m_data->resize(sz); }

/**
 * @brief Swap with the contents of another @c mutable_shared_buffer object.
 */
  void swap(mutable_shared_buffer& rhs) noexcept {
    using std::swap; // swap idiom
    swap(m_data, rhs.m_data);
  }

/**
 * @brief Append a @c std::byte buffer to the end of the internal buffer.
 *
 * @param buf Non-null pointer to @c std::byte buffer of data.
 *
 * @param sz Size of buffer.
 *
 * @return Reference to @c this (to allow method chaining).
 */
  mutable_shared_buffer& append(const std::byte* buf, std::size_t sz) {
    size_type old_sz = size();
    resize(old_sz + sz); // set up buffer space
    std::copy(buf, buf+sz, data()+old_sz);
    return *this;
  }

/**
 * @brief Append a @c std::span of @c std::bytes to the end of the internal buffer.
 *
 * @param sp @c std::span of @c std::byte data.
 *
 * @return Reference to @c this (to allow method chaining).
 */
  template <std::size_t Ext>
  mutable_shared_buffer& append(std::span<const std::byte, Ext> sp) {
    return append(sp.data(), sp.size());
  }

/**
 * @brief Append by copying bytes from an arbitrary pointer.
 *
 * The pointer passed into this method is cast into a @c std::byte pointer and bytes 
 * are then copied. In particular, this method can be used for @c char pointers, 
 * @c void pointers, @c unsigned @c char pointers, @c std::uint8_t pointers, etc.
 * Non character types that are layout compatible with @c std::byte are allowed.
 *
 * @param buf Non-null pointer to an array of data.
 *
 * @param num Number of elements in the array.
 */
  template <typename T>
  mutable_shared_buffer& append(const T* buf, std::size_t num) {
    return append(std::as_bytes(std::span<const T>{buf, num}));
  }

/**
 * @brief Append by copying bytes from a void pointer.
 *
 * The pointer passed into this constructor is cast into a @c std::byte pointer and bytes 
 * are then appended.
 *
 * @pre Size cannot be greater than the source buffer.
 *
 * @param buf Non-null @c void pointer to a buffer of data. 
 *
 * @param sz Size of buffer, in bytes.
 */
  mutable_shared_buffer& append(const void* buf, size_type sz) {
    return append(std::as_bytes(
          std::span<const std::byte>{std::bit_cast<const std::byte*>(buf), sz}));
  }

/**
 * @brief Append a @c std::span that is a non @c std::byte buffer.
 *
 * The @c std::span passed into this method is performs a cast on the
 * data. In particular, this method can be used for @c char pointers, 
 * @c void pointers, @ unsigned @c char pointers, etc.
 *
 * The type of the span must be convertible to or be layout compatible with
 * @c std::byte.
 *
 * @param sp @c std::span of arbitrary bytes.
 *
 */
  template <typename T, std::size_t Ext>
  mutable_shared_buffer& append(std::span<const T, Ext> sp) {
    return append(std::as_bytes(sp));
  }

/**
 * @brief Append the contents of another @c mutable_shared_buffer to the end.
 *
 * @param rhs @c mutable_shared_buffer to append from.
 *
 * @return Reference to @c this (to allow method chaining).
 */
  mutable_shared_buffer& append(const mutable_shared_buffer& rhs) {
    return append(rhs.data(), rhs.size());
  }

/**
 * @brief Append the contents of another @c mutable_shared_buffer to the end.
 *
 * See @c append method for details.
 */
  mutable_shared_buffer& operator+=(const mutable_shared_buffer& rhs) {
    return append(rhs);
  }

/**
 * @brief Append a single @c std::byte to the end.
 *
 * @param b Byte to append.
 *
 * @return Reference to @c this (to allow method chaining).
 */
  mutable_shared_buffer& append(std::byte b) {
    return append(&b, 1);
  }

/**
 * @brief Append a single @c std::byte to the end.
 *
 * See @c append method (single @c std::byte) for details.
 */
  mutable_shared_buffer& operator+=(std::byte b) {
    return append(b);
  }

/**
 * @brief Compare two @c mutable_shared_buffer objects for internal buffer 
 * byte-by-byte equality.
 *
 * Internally this invokes the @c std::vector @c operator== on @c std::byte 
 * elements.
 *
 * @return @c true if @c size() same for each, and each byte compares @c true.
 */
  bool operator== (const mutable_shared_buffer& rhs) const noexcept { 
    return *m_data == *rhs.m_data;
  }  

/**
 * @brief Compare two @c mutable_shared_buffer objects for internal buffer 
 * byte-by-byte spaceship operator ordering.
 *
 * Internally this invokes the @c std::vector @c <=> on @c std::byte 
 * elements.
 *
 * @return Spaceship operator comparison result.
 *
 */
  auto operator<=>(const mutable_shared_buffer& rhs) const noexcept {
    return *m_data <=> *rhs.m_data;
  }

}; // end mutable_shared_buffer class

// non-member functions
/**
 *  @brief Swap two @c mutable_shared_buffer objects.
 *
 */

inline void swap(mutable_shared_buffer& lhs, mutable_shared_buffer& rhs) noexcept {
  lhs.swap(rhs);
}


/**
 * @brief A reference counted non-modifiable buffer class with various convenience methods, 
 * providing efficient copying and convenient buffer lifetime management. 
 *
 * The primary difference between this class and the @c mutable_shared_buffer class is that
 * once a @c const_shared_buffer object is constructed, nothing inside it can be modified. This 
 * allows it to be used with asynchronous IO functions which depend on the buffer staying the 
 * same (i.e. the internal pointer to the data and the size) for the full lifetime of the 
 * asynchronous operations.
 *
 * @invariant There will always be an internal buffer of data, even if the size is zero.
 *
 */

class const_shared_buffer {
public:
  using byte_vec = std::vector<std::byte>;
  using size_type = typename byte_vec::size_type;

private:
  std::shared_ptr<byte_vec> m_data;

private:

  friend bool operator==(const mutable_shared_buffer&, const const_shared_buffer&) noexcept;
  friend bool operator==(const const_shared_buffer&, const mutable_shared_buffer&) noexcept;

public:

  const_shared_buffer() = delete;

  // default copy and move construction, should do the right thing
  const_shared_buffer(const const_shared_buffer&) = default;
  const_shared_buffer(const_shared_buffer&&) = default;
  // copy and move assignment disabled
  const_shared_buffer& operator=(const const_shared_buffer&) = delete;
  const_shared_buffer& operator=(const_shared_buffer&&) = delete;

/**
 * @brief Construct by copying from a @c std::span of @c std::byte.
 *
 * @param sp @c std::byte span pointing to buffer of data. The data is
 * copied into the internal buffer of the @c const_shared_buffer. 
 *
 */
  template <std::size_t Ext>
  explicit const_shared_buffer(std::span<const std::byte, Ext> sp) : 
      m_data(std::make_shared<byte_vec>(sp.data(), sp.data()+sp.size())) { }
/**
 * @brief Construct by copying from a @c std::byte array.
 *
 * @pre Size cannot be greater than the source buffer.
 *
 * @param buf Non-null pointer to @c std::byte buffer of data. The data is
 * copied into the internal buffer of the @c const_shared_buffer.
 *
 * @param sz Size of buffer.
 */
  const_shared_buffer(const std::byte* buf, std::size_t sz) : 
      const_shared_buffer(std::as_bytes(std::span<const std::byte>{buf, sz})) { }

/**
 * @brief Construct by copying from a @c std::span.
 *
 * The type of the span must be convertible to or be layout compatible with
 * @c std::byte.
 *
 * @param sp @c std::span pointing to buffer of data. The @c std::span 
 * pointer is cast into a @c std::byte pointer and bytes are then copied. 
 *
 */
  template <typename T, std::size_t Ext>
  const_shared_buffer(std::span<const T, Ext> sp) : 
      const_shared_buffer(std::as_bytes(sp)) { }
/**
 * @brief Construct by copying bytes from an arbitrary pointer.
 *
 * The pointer passed into this constructor is cast into a @c std::byte pointer and bytes 
 * are then copied. In particular, this method can be used for @c char pointers, 
 * @c unsigned @c char pointers, @c std::uint8_t pointers, etc. Non character types that
 * are trivially copyable are also allowed, although the usual care must be taken
 * (padding bytes, alignment, etc).
 *
 * The type of the span must be convertible to or be layout compatible with
 * @c std::byte.
 *
 * @pre Size cannot be greater than the source buffer.
 *
 * @param buf Non-null pointer to an array of data. 
 *
 * @param num Number of elements in the array.
 */
  template <typename T>
  const_shared_buffer(const T* buf, std::size_t num) : 
      const_shared_buffer(std::as_bytes(std::span<const T>{buf, num})) { }

/**
 * @brief Construct by copying bytes from a void pointer.
 *
 * The pointer passed into this constructor is cast into a @c std::byte pointer and bytes 
 * are then copied.
 *
 * @pre Size cannot be greater than the source buffer.
 *
 * @param buf Non-null @c void pointer to a buffer of data. 
 *
 * @param sz Size of buffer, in bytes.
 */
  const_shared_buffer(const void* buf, size_type sz) : 
      const_shared_buffer(std::as_bytes(
          std::span<const std::byte>{std::bit_cast<const std::byte*>(buf), sz})) { }

/**
 * @brief Construct by copying from a @c mutable_shared_buffer object.
 *
 * This constructor will copy from a @c mutable_shared_buffer. There is an alternative
 * constructor that is more efficient which moves from a @c mutable_shared_buffer 
 * instead of copying.
 *  
 * @param rhs @c mutable_shared_buffer containing bytes to be copied.
 */
  explicit const_shared_buffer(const mutable_shared_buffer& rhs) : 
      const_shared_buffer(rhs.data(), rhs.size()) { }

/**
 * @brief Construct by moving from a @c mutable_shared_buffer object.
 *
 * This constructor will move from a @c mutable_shared_buffer into a @c const_shared_buffer. 
 * This allows efficient API boundaries, where application code can construct and fill in a
 * @c mutable_shared_buffer, then use this constructor which will @c std::move it into a 
 * @c const_shared_buffer for use with asynchronous functions.
 *  
 * @param rhs @c mutable_shared_buffer to be moved from; after moving the 
 * @c mutable_shared_buffer will be empty.
 */
  explicit const_shared_buffer(mutable_shared_buffer&& rhs) noexcept : 
      m_data(std::move(rhs.m_data)) {
    rhs.m_data = std::make_shared<byte_vec>(0); // set rhs back to invariant
  }

/**
 * @brief Move construct from a @c std::vector of @c std::bytes.
 *
 * Efficiently construct from a @c std::vector of @c std::bytes by moving
 * into a @c const_shared_buffer.
 *
 */
  explicit const_shared_buffer(byte_vec&& bv) noexcept :
      m_data(std::make_shared<byte_vec>()) { 
    *m_data = std::move(bv);
  }

/**
 * @brief Construct from input iterators.
 *
 * @pre Valid iterator range, where each element is convertible to a @c std::byte.
 *
 * @param beg Beginning input iterator of range.
 * @param end Ending input iterator of range.
 *
 */
  template <typename InIt>
  const_shared_buffer(InIt beg, InIt end) : m_data(std::make_shared<byte_vec>(beg, end)) { }

/**
 * @brief Return @c const @c std::byte pointer to beginning of buffer.
 *
 * This method provides pointer access to the beginning of the buffer. If the
 * buffer is empty the pointer cannot be dereferenced or undefined behavior will 
 * occur.
 *
 * Accessing past the end of the internal buffer (as defined by the @c size() 
 * method) results in undefined behavior.
 *
 * @return @c const @c std::byte pointer to buffer.
 */
  const std::byte* data() const noexcept { return m_data->data(); }

/**
 * @brief Return size (number of bytes) of buffer.
 *
 * @return Size of buffer, which may be zero.
 */
  size_type size() const noexcept { return m_data->size(); }

/**
 * @brief Query to see if size is zero.
 *
 * @return @c true if empty (size equals zero).
 */
  bool empty() const noexcept { return (*m_data).empty(); }

/**
 * @brief Compare two @c const_shared_buffer objects for internal buffer 
 * byte-by-byte equality.
 *
 * Internally this invokes the @c std::vector @c operator== on @c std::byte 
 * elements.
 *
 * @return @c true if @c size() same for each, and each byte compares @c true.
 *
 */
  bool operator== (const const_shared_buffer& rhs) const noexcept { 
    return *m_data == *rhs.m_data;
  } 
/**
 * @brief Compare two @c const_shared_buffer objects for internal buffer 
 * byte-by-byte spaceship operator ordering.
 *
 * Internally this invokes the @c std::vector @c <=> on @c std::byte 
 * elements.
 *
 * @return Spaceship operator comparison result.
 *
 */
  auto operator<=> (const const_shared_buffer& rhs) const noexcept {
    return *m_data <=> *rhs.m_data;
  }

}; // end const_shared_buffer class

// non-member functions

/**
 * @brief Compare a @c const_shared_buffer object with a @c mutable_shared_buffer for 
 * internal buffer byte-by-byte equality.
 *
 * @return @c true if @c size() same for each, and each byte compares @c true.
 */
inline bool operator== (const const_shared_buffer& lhs, const mutable_shared_buffer& rhs) noexcept { 
  return *lhs.m_data == *rhs.m_data;
}  

/**
 * @brief Compare a @c mutable_shared_buffer object with a @c const_shared_buffer for 
 * internal buffer byte-by-byte equality.
 *
 * @return @c true if @c size() same for each, and each byte compares @c true.
 */
inline bool operator== (const mutable_shared_buffer& lhs, const const_shared_buffer& rhs) noexcept { 
  return *lhs.m_data == *rhs.m_data;
}  

} // end namespace

#endif

