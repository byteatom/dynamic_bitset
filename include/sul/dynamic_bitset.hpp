//
// Copyright (c) 2019 Maxime Pinard
//
// Distributed under the MIT license
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
//
#ifndef SUL_DYNAMIC_BITSET_HPP
#define SUL_DYNAMIC_BITSET_HPP

namespace std {
#if (__cplusplus < 201402L)
	template< class T = void >
	struct bit_not {
		constexpr T operator()(const T& arg) const
		{
			return ~arg;
		}
	};
#endif
}

/**
 * @brief      @ref sul::dynamic_bitset version major.
 */
#define SUL_DYNAMIC_BITSET_VERSION_MAJOR 1

/**
 * @brief      @ref sul::dynamic_bitset version minor.
 */
#define SUL_DYNAMIC_BITSET_VERSION_MINOR 2

/**
 * @brief      @ref sul::dynamic_bitset version patch.
 */
#define SUL_DYNAMIC_BITSET_VERSION_PATCH 0

/** @file
 * @brief      @ref sul::dynamic_bitset declaration and implementation.
 *
 * @details    Standalone file, does not depend on other implementation files or dependencies other
 *             than the standard library, can be taken and used directly.
 *
 *             Can optionally include and use libpopcnt if @a DYNAMIC_BITSET_NO_LIBPOPCNT is not
 *             defined and @a __has_include(\<libpopcnt.h\>) is @a true.
 *
 * @remark     Include multiple standard library headers and optionally @a libpopcnt.h.
 */

#include <memory>
#include <vector>
#include <algorithm>
#include <string>
#if (__cplusplus >= 201703L)
#include <string_view>
#endif
#include <functional>
#include <type_traits>
#include <limits>
#include <cmath>
#include <cassert>

#ifndef DYNAMIC_BITSET_NO_LIBPOPCNT
#	if 1//__has_include(<libpopcnt.h>)
#		include <libpopcnt.h>
#		define DYNAMIC_BITSET_USE_LIBPOPCNT
#	endif
#endif

#if defined(__clang__)
#	define DYNAMIC_BITSET_CLANG
#	ifdef __has_builtin
#		if __has_builtin(__builtin_popcount) && __has_builtin(__builtin_popcountl) \
		  && __has_builtin(__builtin_popcountll)
#			define DYNAMIC_BITSET_CLANG_builtin_popcount
#		endif
#		if __has_builtin(__builtin_ctz) && __has_builtin(__builtin_ctzl) \
		  && __has_builtin(__builtin_ctzll)
#			define DYNAMIC_BITSET_CLANG_builtin_ctz
#		endif
#	endif
#elif defined(__GNUC__)
#	define DYNAMIC_BITSET_GCC
#elif defined(_MSC_VER)
#	define DYNAMIC_BITSET_MSVC
#	include <intrin.h>
#	pragma intrinsic(_BitScanForward)
#	if defined(_M_X64) || defined(_M_ARM64)
#		define DYNAMIC_BITSET_MSVC_64
#		pragma intrinsic(_BitScanForward64)
#	else
#	endif
#endif

#ifndef DYNAMIC_BITSET_NO_NAMESPACE
/**
 * @brief      Simple Useful Libraries.
 */
namespace sul
{
#endif

/**
 * @brief      Dynamic bitset.
 *
 * @details    Data structure used to store a vector of bits and apply binary operations to it. The
 *             bits are stored in an optimized way in an underling block type. It is highly inspired
 *             by std\::bitset but with a run-time changeable size.
 *
 *             Preconditions are checked with @a assert but no exception will be thrown if one is
 *             violated (as with std\::bitset).
 *
 * @remark     It is not a Container as it does not provide iterators because of the reference proxy
 *             class used to access the bits.
 *
 * @tparam     Block      Block type to use for storing the bits, must be an unsigned integral type
 * @tparam     Allocator  Allocator type to use for memory management, must meet the standard
 *                        requirements of @a Allocator
 */
template<typename Block = unsigned long long, typename Allocator = std::allocator<Block>>
class dynamic_bitset
{
	static_assert(std::is_unsigned<Block>::value, "Block is not an unsigned integral type");

public:
	/**
	 * @brief      Type used to represent the size of a @ref sul::dynamic_bitset.
	 */
	typedef size_t size_type;

	/**
	 * @brief      Same type as @p Block.
	 */
	typedef Block block_type;

	/**
	 * @brief      Same type as @p Allocator.
	 */
	typedef Allocator allocator_type;

	/**
	 * @brief      Number of bits that can be stored in a block.
	 */
	static constexpr size_type bits_per_block = std::numeric_limits<block_type>::digits;

	/**
	 * @brief      Maximum value of @ref size_type, returned for invalid positions.
	 */
	static constexpr size_type npos = std::numeric_limits<size_type>::max();

	/**
	 * @brief      Reference to a @ref sul::dynamic_bitset bit.
	 *
	 * @details    As the bits in the @ref sul::dynamic_bitset class are stored in an optimized way
	 *             in blocks, it is not possible for the subscript operators to return a reference
	 *             to a boolean. Hence this class is used as a proxy to enable subscript operator of
	 *             the @ref sul::dynamic_bitset class to be used as if it was an array of bools.
	 */
	class reference
	{
	public:
		/**
		 * @brief      Constructs a @ref reference to a bit from a @ref sul::dynamic_bitset and a
		 *             bit position.
		 *
		 * @param      bitset   @ref sul::dynamic_bitset containing the bit
		 * @param[in]  bit_pos  Position of the bit to reference in the @ref sul::dynamic_bitset
		 *
		 * @complexity Constant.
		 */
		constexpr reference(dynamic_bitset<Block, Allocator>& bitset, size_type bit_pos);

		/**
		 * @brief      Copy constructor.
		 */
		constexpr reference(const reference&) noexcept = default;

		/**
		 * @brief      Move constructor.
		 */
		constexpr reference(reference&&) noexcept = default;

		/**
		 * @brief      Destructor.
		 */
		~reference() noexcept = default;

		/**
		 * @brief      Assign a value to the referenced bit.
		 *
		 * @param[in]  v     Value to assign to the referenced bit
		 *
		 * @return     The @ref reference
		 *
		 * @complexity Constant.
		 */
		constexpr reference& operator=(bool v);

		/**
		 * @brief      Assign a value to the referenced bit from another @ref reference.
		 *
		 * @param[in]  rhs   @ref reference to the bit to assign value from
		 *
		 * @return     The @ref reference
		 *
		 * @complexity Constant.
		 */
		constexpr reference& operator=(const reference& rhs);

		/**
		 * @brief      Assign a value to the referenced bit from another @ref reference.
		 *
		 * @param[in]  rhs   @ref reference to the bit to assign value from
		 *
		 * @return     The @ref reference
		 *
		 * @complexity Constant.
		 */
		constexpr reference& operator=(reference&& rhs) noexcept;

		/**
		 * @brief      Apply binary operator AND to the referenced bit and a value, and assign the
		 *             result to the referenced bit.
		 *
		 * @param[in]  v     Value to apply binary operator AND with
		 *
		 * @return     The @ref reference
		 *
		 * @complexity Constant.
		 */
		constexpr reference& operator&=(bool v);

		/**
		 * @brief      Apply binary operator OR to the referenced bit and a value, and assign the
		 *             result to the referenced bit.
		 *
		 * @param[in]  v     Value to apply binary operator OR with
		 *
		 * @return     The @ref reference
		 *
		 * @complexity Constant.
		 */
		constexpr reference& operator|=(bool v);

		/**
		 * @brief      Apply binary operator XOR to the referenced bit and a value, and assign the
		 *             result to the referenced bit.
		 *
		 * @param[in]  v     Value to apply binary operator XOR with
		 *
		 * @return     The @ref reference
		 *
		 * @complexity Constant.
		 */
		constexpr reference& operator^=(bool v);

		/**
		 * @brief      Apply binary difference to the referenced bit and a value, and assign the
		 *             result to the referenced bit.
		 *
		 * @details    Equivalent to:
		 *             @code
		 *             this &= !v;
		 *             @endcode
		 *
		 * @param[in]  v     Value to apply binary difference with
		 *
		 * @return     The @ref reference
		 *
		 * @complexity Constant.
		 */
		constexpr reference& operator-=(bool v);

		/**
		 * @brief      Return the result of applying unary NOT operator.
		 *
		 * @return     The opposite of the referenced bit
		 *
		 * @complexity Constant.
		 */
	#if (__cplusplus >= 201703L)
		[[nodiscard]]
	#endif
		constexpr bool operator~() const;

		/**
		 * @brief      bool conversion operator.
		 *
		 * @complexity Constant.
		 */
	#if (__cplusplus >= 201703L)
		[[nodiscard]]
	#endif
		constexpr operator bool() const;

		/**
		 * @brief      Deleted to avoid taking the address of a temporary proxy object.
		 */
		constexpr void operator&() = delete;

		/**
		 * @brief      Set the referenced bit to @a true.
		 *
		 * @return     The @ref reference
		 *
		 * @complexity Constant.
		 */
		constexpr reference& set();

		/**
		 * @brief      Reset the referenced bit to @a false.
		 *
		 * @return     The @ref reference
		 *
		 * @complexity Constant.
		 */
		constexpr reference& reset();

		/**
		 * @brief      Flip the referenced bit.
		 *
		 * @return     The @ref reference
		 *
		 * @complexity Constant.
		 */
		constexpr reference& flip();

		/**
		 * @brief      Assign the value @p v to the referenced bit.
		 *
		 * @param[in]  v     Value to assign to the bit
		 *
		 * @return     The @ref reference
		 *
		 * @complexity Constant.
		 */
		constexpr reference& assign(bool v);

	private:
		block_type& m_block;
		block_type m_mask;
	};

	/**
	 * @brief      Const reference to a @ref sul::dynamic_bitset bit, type bool.
	 */
	typedef bool const_reference;

	/**
	 * @brief      Copy contructor.
	 */
	constexpr dynamic_bitset(const dynamic_bitset<Block, Allocator>& other) = default;

	/**
	 * @brief      Move contructor.
	 */
	constexpr dynamic_bitset(dynamic_bitset<Block, Allocator>&& other) noexcept = default;

	/**
	 * @brief      Copy assignment operator.
	 */
#if (__cplusplus >= 201703L)
	constexpr
#endif
		dynamic_bitset<Block, Allocator>& operator=(
	  const dynamic_bitset<Block, Allocator>& other) = default;

	/**
	 * @brief      Move assignment operator.
	 */
#if (__cplusplus >= 201703L)
	constexpr
#endif
		dynamic_bitset<Block, Allocator>& operator=(
	  dynamic_bitset<Block, Allocator>&& other) /*noexcept*/ = default;

	/**
	 * @brief      Constructs an empty @ref sul::dynamic_bitset.
	 *
	 * @details    A copy of @p allocator will be used for memory management.
	 *
	 * @param[in]  allocator  Allocator to use for memory management
	 *
	 * @complexity Constant.
	 */
	constexpr explicit dynamic_bitset(const allocator_type& allocator = allocator_type());

	/**
	 * @brief      Constructs a @ref sul::dynamic_bitset of @p nbits bits from an initial value.
	 *
	 * @details    The first bits are initialized with the bits from @p init_val, if @p nbits \>
	 *             std\::numeric_limits\<unsigned long long\>\::digits , all other bits are
	 *             initialized to @a false. A copy of @p allocator will be used for memory
	 *             management.
	 *
	 * @param[in]  nbits      Number of bits of the @ref sul::dynamic_bitset
	 * @param[in]  init_val   Value to initialize the @ref sul::dynamic_bitset with
	 * @param[in]  allocator  Allocator to use for memory management
	 *
	 * @complexity Linear in @p nbits / @ref bits_per_block.
	 */
#if (__cplusplus >= 201703L)
	constexpr
#endif 
		explicit dynamic_bitset(size_type nbits,
	                                  unsigned long long init_val = 0,
	                                  const allocator_type& allocator = allocator_type());

	/**
	 * @brief      Constructs a @ref sul::dynamic_bitset using @p init_vals to initialize the first
	 *             blocks.
	 *
	 * @details    The size of the newly created @ref sul::dynamic_bitset is @p init_vals.size() *
	 *             @ref bits_per_block. A copy of @p allocator will be used for memory management.
	 *
	 * @param[in]  init_vals  Value of the @ref sul::dynamic_bitset first blocks
	 * @param[in]  allocator  Allocator to use for memory management
	 *
	 * @complexity Linear in @p init_vals.size().
	 */
#if (__cplusplus >= 201703L)
	constexpr
#endif 
		dynamic_bitset(std::initializer_list<block_type> init_vals,
	                         const allocator_type& allocator = allocator_type());

	/**
	 * @brief      Constructs a @ref sul::dynamic_bitset from a string or a part of a string.
	 *
	 * @details    Construct the @ref sul::dynamic_bitset using the characters from @p str in the
	 *             range \[@p pos, std\::min(@p pos + @p n, @p str.size())\[.
	 *
	 * @param[in]  str        String containing the part to use
	 * @param[in]  pos        Starting position of the string part to use in @p str
	 * @param[in]  n          Number of characters of @p str to use from the starting position
	 * @param[in]  zero       Character used to represent @a false bits in @p str
	 * @param[in]  one        Character used to represent @a true bits in @p str
	 * @param[in]  allocator  Allocator to use for memory management
	 *
	 * @tparam     _CharT     Character type of the string
	 * @tparam     _Traits    Traits class specifying the operations on the character type of the
	 *                        string
	 *
	 * @pre        @code
	 *             pos < str.size()
	 *             @endcode
	 *
	 * @complexity Linear in std\::min(@p n, @p str.size() - @p pos).
	 */
#if (__cplusplus >= 201703L)
	template<typename _CharT, typename _Traits>
	constexpr explicit dynamic_bitset(
	  std::basic_string_view<_CharT, _Traits> str,
	  typename std::basic_string_view<_CharT, _Traits>::size_type pos = 0,
	  typename std::basic_string_view<_CharT, _Traits>::size_type n =
	    std::basic_string_view<_CharT, _Traits>::npos,
	  _CharT zero = _CharT('0'),
	  _CharT one = _CharT('1'),
	  const allocator_type& allocator = allocator_type());

	/**
	 * @brief      Constructs a @ref sul::dynamic_bitset from a string or a part of a string.
	 *
	 * @details    Construct the @ref sul::dynamic_bitset using the characters from @p str in the
	 *             range \[@p pos, std\::min(@p pos + @p n, @p str.size())\[.
	 *
	 * @param[in]  str        String containing the part to use
	 * @param[in]  pos        Starting position of the string part to use in @p str
	 * @param[in]  n          Number of characters of @p str to use from the starting position
	 * @param[in]  zero       Character used to represent @a false bits in @p str
	 * @param[in]  one        Character used to represent @a true bits in @p str
	 * @param[in]  allocator  Allocator to use for memory management
	 *
	 * @tparam     _CharT     Character type of the string
	 * @tparam     _Traits    Traits class specifying the operations on the character type of the
	 *                        string
	 * @tparam     _Alloc     Allocator type used to allocate internal storage of the string
	 *
	 * @pre        @code
	 *             pos < str.size()
	 *             @endcode
	 *
	 * @complexity Linear in std\::min(@p n, @p str.size() - @p pos).
	 */
	template<typename _CharT, typename _Traits, typename _Alloc>
	constexpr explicit dynamic_bitset(
	  const std::basic_string<_CharT, _Traits, _Alloc>& str,
	  typename std::basic_string<_CharT, _Traits, _Alloc>::size_type pos = 0,
	  typename std::basic_string<_CharT, _Traits, _Alloc>::size_type n =
	    std::basic_string<_CharT, _Traits, _Alloc>::npos,
	  _CharT zero = _CharT('0'),
	  _CharT one = _CharT('1'),
	  const allocator_type& allocator = allocator_type());

	/**
	 * @brief      Constructs a @ref sul::dynamic_bitset from a string or a part of a string.
	 *
	 * @details    Construct the @ref sul::dynamic_bitset using the characters from @p str in the
	 *             range \[@p pos, std\::min(@p pos + @p n, @p _Traits\::length(@p str))\[.
	 *
	 * @param[in]  str        String containing the part to use
	 * @param[in]  pos        Starting position of the string part to use
	 * @param[in]  n          Number of characters to use from the starting position
	 * @param[in]  zero       Character used to represent @a false bits in the string
	 * @param[in]  one        Character used to represent 1 @a true bitsn the string
	 * @param[in]  allocator  Allocator to use for memory management
	 *
	 * @tparam     _CharT     Character type of the string
	 * @tparam     _Traits    Traits class specifying the operations on the character type of the
	 *                        string
	 *
	 * @pre        @code
	 *             pos < _Traits::length(str)
	 *             @endcode
	 *
	 * @complexity Linear in std\::min(@p n, @p _Traits\::length(@p str) - @p pos).
	 */
	template<typename _CharT, typename _Traits = std::char_traits<_CharT>>
	constexpr explicit dynamic_bitset(
	  const _CharT* str,
	  typename std::basic_string<_CharT>::size_type pos = 0,
	  typename std::basic_string<_CharT>::size_type n = std::basic_string<_CharT>::npos,
	  _CharT zero = _CharT('0'),
	  _CharT one = _CharT('1'),
	  const allocator_type& allocator = allocator_type());
#endif

	/**
	 * @brief      Destructor.
	 */
	~dynamic_bitset() noexcept = default;

	/**
	 * @brief      Resize the @ref sul::dynamic_bitset to contain @p nbits bits.
	 *
	 * @details    Bits keep the value they had before the resize and, if @p nbits is greater than
	 *             the current size, new bit are initialized to @p value.
	 *
	 * @param[in]  nbits  New size of the @ref sul::dynamic_bitset
	 * @param[in]  value  Value of the new bits
	 *
	 * @complexity Linear in the difference between the current size and @p nbits.
	 *             Additional complexity possible due to reallocation if capacity is less than @p
	 *             nbits.
	 */
#if (__cplusplus >= 201703L)
	constexpr
#endif
		void resize(size_type nbits, bool value = false);

	/**
	 * @brief      Clears the @ref sul::dynamic_bitset, resize it to 0.
	 *
	 * @details    Equivalent to:
	 *             @code
	 *             this.resize(0);
	 *             @endcode
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
	constexpr void clear();

	/**
	 * @brief      Add a new bit with the value @p value at the end of the @ref sul::dynamic_bitset.
	 *
	 * @details    Increase the size of the bitset by one, the added bit becomes the
	 *             most-significant bit.
	 *
	 * @param[in]  value  Value of the bit to add
	 *
	 * @complexity Amortized constant.
	 */
#if (__cplusplus >= 201703L)
	constexpr
#endif
		void push_back(bool value);

	/**
	 * @brief      Remove the last bit of the @ref sul::dynamic_bitset.
	 *
	 * @details    Decrease the size of the @ref sul::dynamic_bitset by one, does nothing if the
	 *             @ref dynamic_bitset is empty.
	 *
	 * @complexity Constant.
	 */
#if (__cplusplus >= 201703L)
	constexpr
#endif
		void pop_back();

	/**
	 * @brief      Append a block of bits @p block at the end of the @ref sul::dynamic_bitset.
	 *
	 * @details    Increase the size of the @ref sul::dynamic_bitset by @ref bits_per_block.
	 *
	 * @param[in]  block  Block of bits to add
	 *
	 * @complexity Amortized constant.
	 */
	constexpr void append(block_type block);

	/**
	 * @brief      Append blocks of bits from @p blocks at the end of the @ref sul::dynamic_bitset.
	 *
	 * @param[in]  blocks  Blocks of bits to add
	 *
	 * @complexity Linear in the size of @p blocks. Additional complexity possible due
	 *             to reallocation if capacity is less than @ref size() + @p blocks.size() * @ref
	 *             bits_per_block.
	 */
	constexpr void append(std::initializer_list<block_type> blocks);

	/**
	 * @brief      Append blocks of bits from the range \[@p first, @p last\[ at the end of the @ref
	 *             dynamic_bitset.
	 *
	 * @param[in]  first               First iterator of the range
	 * @param[in]  last                Last iterator of the range (after the last element to add)
	 *
	 * @tparam     BlockInputIterator  Type of the range iterators
	 *
	 * @complexity Linear in the size of the range. Additional complexity possible due
	 *             to reallocation if capacity is less than @ref size() + std\::distance(@p first,
	 *             @p last) * @ref bits_per_block.
	 */
	template<typename BlockInputIterator>
	constexpr void append(BlockInputIterator first, BlockInputIterator last);

	/**
	 * @brief      Sets the bits to the result of binary AND on corresponding pairs of bits of *this
	 *             and @p rhs.
	 *
	 * @param[in]  rhs   Right hand side @ref sul::dynamic_bitset of the operator
	 *
	 * @return     A reference to the @ref sul::dynamic_bitset *this
	 *
	 * @pre       @code
	 *             size() == rhs.size()
	 *             @endcode
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
	constexpr dynamic_bitset<Block, Allocator>& operator&=(
	  const dynamic_bitset<Block, Allocator>& rhs);

	/**
	 * @brief      Sets the bits to the result of binary OR on corresponding pairs of bits of *this
	 *             and @p rhs.
	 *
	 * @param[in]  rhs   Right hand side @ref sul::dynamic_bitset of the operator
	 *
	 * @return     A reference to the @ref sul::dynamic_bitset *this
	 *
	 * @pre        @code
	 *             size() == rhs.size()
	 *             @endcode
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
	constexpr dynamic_bitset<Block, Allocator>& operator|=(
	  const dynamic_bitset<Block, Allocator>& rhs);

	/**
	 * @brief      Sets the bits to the result of binary XOR on corresponding pairs of bits of *this
	 *             and @p rhs.
	 *
	 * @param[in]  rhs   Right hand side @ref sul::dynamic_bitset of the operator
	 *
	 * @return     A reference to the @ref sul::dynamic_bitset *this
	 *
	 * @pre        @code
	 *             size() == rhs.size()
	 *             @endcode
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
	constexpr dynamic_bitset<Block, Allocator>& operator^=(
	  const dynamic_bitset<Block, Allocator>& rhs);

	/**
	 * @brief      Sets the bits to the result of the binary difference between the bits of *this
	 *             and @p rhs.
	 *
	 * @details    Less efficient but equivalent way to get this result:
	 *             @code
	 *             this &= ~rhs;
	 *             @endcode
	 *
	 * @param[in]  rhs   Right hand side @ref sul::dynamic_bitset of the operator
	 *
	 * @return     A reference to the @ref sul::dynamic_bitset *this
	 *
	 * @pre        @code
	 *             size() == rhs.size()
	 *             @endcode
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
	constexpr dynamic_bitset<Block, Allocator>& operator-=(
	  const dynamic_bitset<Block, Allocator>& rhs);

	/**
	 * @brief      Performs binary shift left of @p shift bits.
	 *
	 * @details    Zeroes are shifted in, does nothing if @p shift == 0.
	 *
	 * @param[in]  shift  Number of positions to shift the bits
	 *
	 * @return     A reference to the @ref sul::dynamic_bitset *this
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
	constexpr dynamic_bitset<Block, Allocator>& operator<<=(size_type shift);

	/**
	 * @brief      Performs binary shift right of @p shift bits.
	 *
	 * @details    Zeroes are shifted in, does nothing if @p shift == 0.
	 *
	 * @param[in]  shift  Number of positions to shift the bits
	 *
	 * @return     A reference to the @ref sul::dynamic_bitset *this
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
	constexpr dynamic_bitset<Block, Allocator>& operator>>=(size_type shift);

	/**
	 * @brief      Performs binary shift right of @p shift bits.
	 *
	 * @details    Zeroes are shifted in. Does nothing if @p shift == 0.\n
	 *             Equivalent to:
	 *             @code
	 *             dynamic_bitset<Block, Allocator> bitset(*this);
	 *             bitset <<= shift;
	 *             @endcode
	 *
	 * @param[in]  shift  Number of positions to shift the bits
	 *
	 * @return     A new @ref sul::dynamic_bitset containing the shifted bits
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr dynamic_bitset<Block, Allocator> operator<<(size_type shift) const;

	/**
	 * @brief      Performs binary shift left of @p shift bits.
	 *
	 * @details    Zeroes are shifted in. Does nothing if @p shift == 0.\n
	 *             Equivalent to:
	 *             @code
	 *             dynamic_bitset<Block, Allocator> bitset(*this);
	 *             bitset >>= shift;
	 *             @endcode
	 *
	 * @param[in]  shift  Number of positions to shift the bits
	 *
	 * @return     A new @ref sul::dynamic_bitset containing the shifted bits
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr dynamic_bitset<Block, Allocator> operator>>(size_type shift) const;

	/**
	 * @brief      Performs a unary NOT on all bits.
	 *
	 * @details    Equivalent to:
	 *             @code
	 *             dynamic_bitset<Block, Allocator> bitset(*this);
	 *             bitset.flip();
	 *             @endcode
	 *
	 * @return     A copy of *this with all bits flipped
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr dynamic_bitset<Block, Allocator> operator~() const;

	/**
	 * @brief      Set the bits of the range \[@p pos, @p pos + @p len\[ to value @p value.
	 *
	 * @details    Does nothing if @p len == 0.
	 *
	 * @param[in]  pos    Position of the first bit of the range
	 * @param[in]  len    Length of the range
	 * @param[in]  value  Value to set the bits to
	 *
	 * @return     A reference to the @ref sul::dynamic_bitset *this
	 *
	 * @pre        @code
	 *             (pos < size()) && ((len == 0) || (pos + len - 1 < size()))
	 *             @endcode
	 *
	 * @complexity Linear in @p len.
	 */
#if (__cplusplus >= 201703L)
	constexpr
#endif
		dynamic_bitset<Block, Allocator>& set(size_type pos, size_type len, bool value);

	/**
	 * @brief      Set the bit at the position @p pos to @a true or value @p value.
	 *
	 * @param[in]  pos    Position of the bit to set
	 * @param[in]  value  Value to set the bit to
	 *
	 * @return     A reference to the @ref sul::dynamic_bitset *this
	 *
	 * @pre        @code
	 *             pos < size()
	 *             @endcode
	 *
	 * @complexity Constant.
	 */
#if (__cplusplus >= 201703L)
	constexpr
#endif
		dynamic_bitset<Block, Allocator>& set(size_type pos, bool value = true);

	/**
	 * @brief      Set all the bits of the @ref sul::dynamic_bitset to @a true.
	 *
	 * @return     A reference to the @ref sul::dynamic_bitset *this
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
	constexpr dynamic_bitset<Block, Allocator>& set();

	/**
	 * @brief      Reset the bits of the range \[@p pos, @p pos + @p len\[ to @a false.
	 *
	 * @param[in]  pos   Position of the first bit of the range
	 * @param[in]  len   Length of the range
	 *
	 * @return     A reference to the @ref sul::dynamic_bitset *this
	 *
	 * @pre        @code
	 *             (pos < size()) && ((len == 0) || (pos + len - 1 < size()))
	 *             @endcode
	 *
	 * @complexity Linear in @p len.
	 */
	constexpr dynamic_bitset<Block, Allocator>& reset(size_type pos, size_type len);

	/**
	 * @brief      Reset the bit at the position @p pos to @a false.
	 *
	 * @param[in]  pos    Position of the bit to reset
	 *
	 * @return     A reference to the @ref sul::dynamic_bitset *this
	 *
	 * @pre        @code
	 *             pos < size()
	 *             @endcode
	 *
	 * @complexity Constant.
	 */
	constexpr dynamic_bitset<Block, Allocator>& reset(size_type pos);

	/**
	 * @brief      Reset all the bits of the @ref sul::dynamic_bitset to @a false.
	 *
	 * @return     A reference to the @ref sul::dynamic_bitset *this
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
	constexpr dynamic_bitset<Block, Allocator>& reset();

	/**
	 * @brief      Flip the bits of the range \[@p pos, @p pos + @p len\[.
	 *
	 * @param[in]  pos   Position of the first bit of the range
	 * @param[in]  len   Length of the range
	 *
	 * @return     A reference to the @ref sul::dynamic_bitset *this
	 *
	 * @pre        @code
	 *             (pos < size()) && ((len == 0) || (pos + len - 1 < size()))
	 *             @endcode
	 *
	 * @complexity Linear in @p len.
	 */
	constexpr dynamic_bitset<Block, Allocator>& flip(size_type pos, size_type len);

	/**
	 * @brief      Flip the bit at the position @p pos.
	 *
	 * @param[in]  pos    Position of the bit to reset
	 *
	 * @return     A reference to the @ref sul::dynamic_bitset *this
	 *
	 * @pre        @code
	 *             pos < size()
	 *             @endcode
	 *
	 * @complexity Constant.
	 */
	constexpr dynamic_bitset<Block, Allocator>& flip(size_type pos);

	/**
	 * @brief      Flip all the bits of the @ref sul::dynamic_bitset.
	 *
	 * @return     A reference to the @ref sul::dynamic_bitset *this
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
	constexpr dynamic_bitset<Block, Allocator>& flip();

	/**
	 * @brief      Test the value of the bit at position @p pos.
	 *
	 * @param[in]  pos   Position of the bit to test
	 *
	 * @return     The tested bit value
	 *
	 * @pre        @code
	 *             pos < size()
	 *             @endcode
	 *
	 * @complexity Constant.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr bool test(size_type pos) const;

	/**
	 * @brief      Test the value of the bit at position @p pos and set it to @a true or value @p
	 *             value.
	 *
	 * @param[in]  pos    Position of the bit to test and set
	 * @param[in]  value  Value to set the bit to
	 *
	 * @return     The tested bit value
	 *
	 * @pre        @code
	 *             pos < size()
	 *             @endcode
	 *
	 * @complexity Constant.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr bool test_set(size_type pos, bool value = true);

	/**
	 * @brief      Checks if all bits are set to @a true.
	 *
	 * @return     @a true if all bits are set to @a true, otherwise @a false
	 *
	 * @remark     Return @a true if the @ref sul::dynamic_bitset is empty, the logic is that you
	 *             are checking if all bits are set to @a true, meaning none of them is set to @a
	 *             false, and in an empty @ref sul::dynamic_bitset no bits are set to @a false.
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr bool all() const;

	/**
	 * @brief      Checks if any bits are set to @a true.
	 *
	 * @return     @a true if any of the bits is set to @a true, otherwise @a false
	 *
	 * @remark     Return @a false if the @ref sul::dynamic_bitset is empty, the logic is you are
	 *             checking if there is at least one bit set to @a true and in an empty @ref
	 *             dynamic_bitset there is no bit set to @a true.
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr bool any() const;

	/**
	 * @brief      Checks if none of the bits are set to @a true.
	 *
	 * @return     @a true if none of the bits is set to @a true, otherwise @a false
	 *
	 * @remark     Return @a true if the @ref sul::dynamic_bitset is empty, the logic is that you
	 *             are checking if there is no bit set to @a true and in an empty @ref
	 *             sul::dynamic_bitset there is no bit that can be set to @a true.
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr bool none() const;

	/**
	 * @brief      Count the number of bits set to @a true.
	 *
	 * @details    Return 0 if the @ref sul::dynamic_bitset is empty.
	 *
	 * @return     The number of bits that are set to @a true
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr size_type count() const noexcept;

	/**
	 * @brief      Accesses the bit at position @p pos.
	 *
	 * @param[in]  pos   Position of the bit to access
	 *
	 * @return     A @ref reference object which allows writing to the requested bit
	 *
	 * @pre        @code
	 *             pos < size()
	 *             @endcode
	 *
	 * @complexity Constant.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]] constexpr
#endif
	reference operator[](size_type pos);

	/**
	 * @brief      Accesses the bit at position @p pos.
	 *
	 * @param[in]  pos   Position of the bit to access
	 *
	 * @return     The value of the requested bit
	 *
	 * @pre        @code
	 *             pos < size()
	 *             @endcode
	 *
	 * @complexity Constant.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr const_reference operator[](size_type pos) const;

	/**
	 * @brief      Give the number of bits of the @ref sul::dynamic_bitset.
	 *
	 * @return     The number of bits of the @ref sul::dynamic_bitset
	 *
	 * @complexity Constant.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr size_type size() const noexcept;

	/**
	 * @brief      Give the number of blocks used by the @ref sul::dynamic_bitset.
	 *
	 * @return     The number of blocks used by the @ref sul::dynamic_bitset
	 *
	 * @complexity Constant.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr size_type num_blocks() const noexcept;

	/**
	 * @brief      Checks if the @ref sul::dynamic_bitset is empty.
	 *
	 * @details    Equivalent to:
	 *             @code
	 *             size() == 0;
	 *             @endcode
	 *
	 * @return     @a true if the @ref sul::dynamic_bitset is empty, @a false otherwise
	 *
	 * @complexity Constant.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr bool empty() const noexcept;

	/**
	 * @brief      Give the number of bits that the @ref sul::dynamic_bitset has currently allocated
	 *             space for.
	 *
	 * @return     Capacity of the currently allocated storage.
	 *
	 * @complexity Constant.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr size_type capacity() const noexcept;

	/**
	 * @brief      Increase the capacity of the @ref sul::dynamic_bitset to a value that's greater
	 *             or equal to @p num_bits.
	 *
	 * @details    If @p num_bits is greater than the current capacity, new storage is allocated and
	 *             all @ref reference on bits of the @ref sul::dynamic_bitset are invalidated,
	 *             otherwise the method does nothing.
	 *
	 * @param[in]  num_bits  New capacity of the @ref sul::dynamic_bitset
	 *
	 * @complexity At most linear in the size of the @ref sul::dynamic_bitset.
	 */
	constexpr void reserve(size_type num_bits);

	/**
	 * @brief      Requests the removal of unused capacity.
	 *
	 * @details    It is a non-binding request to reduce the capacity to the size. It depends on the
	 *             implementation of std\::vector whether the request is fulfilled.\n If
	 *             reallocation occurs, all @ref reference on bits of the @ref sul::dynamic_bitset
	 *             are invalidated.
	 *
	 * @complexity At most linear in the size of the @ref sul::dynamic_bitset.
	 */
	constexpr void shrink_to_fit();

	/**
	 * @brief      Determines if this @ref sul::dynamic_bitset is a subset of @p bitset.
	 *
	 * @details    This @ref sul::dynamic_bitset is a subset of @p bitset if, for every bit that is
	 *             set in this @ref sul::dynamic_bitset, the corresponding bit in @p bitset a is
	 *             also set.\n\n Less efficient but equivalent way to get this result:
	 *             @code
	 *             res = (this & ~bitset).none();
	 *             @endcode
	 *
	 * @param[in]  bitset  The @ref sul::dynamic_bitset for which to check if this @ref
	 *                     sul::dynamic_bitset is a subset
	 *
	 * @return     @a true if this @ref sul::dynamic_bitset is a subset of @p bitset, @a false
	 *             otherwise
	 *
	 * @remark     The relation "is a subset of" is not symmetric (A being a subset of B doesn't
	 *             imply that B is a subset of A) but is antisymmetric (if A is a subset of B and B
	 *             is a subset of A, then A == B).
	 *
	 * @pre        @code
	 *             size() == bitset.size()
	 *             @endcode
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr bool is_subset_of(const dynamic_bitset<Block, Allocator>& bitset) const;

	/**
	 * @brief      Determines if this @ref sul::dynamic_bitset is a proper subset of @p bitset.
	 *
	 * @details    This @ref sul::dynamic_bitset is a proper subset of @p bitset if, for every bit
	 *             that is set in this @ref sul::dynamic_bitset, the corresponding bit in @p bitset
	 *             a is also set and if this @ref sul::dynamic_bitset is different from @p
	 *             bitset.\n\n Less efficient but equivalent way to get this result:
	 *             @code
	 *             res = ((this != bitset) && (this & ~bitset).none());
	 *             @endcode
	 *
	 * @param[in]  bitset  The @ref sul::dynamic_bitset for which to check if this @ref
	 *                     sul::dynamic_bitset is a proper subset
	 *
	 * @return     @a true if this @ref sul::dynamic_bitset is a proper subset of @p bitset, @a
	 *             false otherwise
	 *
	 * @remark     The relation "is a proper subset of" is asymmetric (A being a proper subset of B
	 *             imply that B is not a proper subset of A).
	 *
	 * @pre        @code
	 *             size() == bitset.size()
	 *             @endcode
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr bool is_proper_subset_of(
	  const dynamic_bitset<Block, Allocator>& bitset) const;

	/**
	 * @brief      Determines if this @ref sul::dynamic_bitset and @p bitset intersect.
	 *
	 * @details    This @ref sul::dynamic_bitset intersects with @p bitset if for at least one bit
	 *             set in this @ref sul::dynamic_bitset, the corresponding bit in @p bitset a is
	 *             also set. In other words two bitsets intersect if they have at least one bit set
	 *             in common.\n\n Less efficient but equivalent way to get this result:
	 *             @code
	 *             res = (this & bitset).any();
	 *             @endcode
	 *
	 * @param[in]  bitset  The @ref sul::dynamic_bitset for which to check if this @ref
	 *                     sul::dynamic_bitset intersects
	 *
	 * @return     @a true if this @ref sul::dynamic_bitset intersects with @p bitset, @a false
	 *             otherwise
	 *
	 * @pre        @code
	 *             size() == bitset.size()
	 *             @endcode
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr bool intersects(const dynamic_bitset<Block, Allocator>& bitset) const;

	/**
	 * @brief      Find the position of the first bit set in the @ref sul::dynamic_bitset starting
	 *             from the least-significant bit.
	 *
	 * @details    Give the lowest index of the @ref sul::dynamic_bitset with a bit set, or @ref
	 *             npos if no bits are set.
	 *
	 * @return     The position of the first bit set, or @ref npos if no bits are set
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr size_type find_first() const;

	/**
	 * @brief      Find the position of the first bit set in the range \[@p prev + 1, @ref size()\[
	 *             of the @ref sul::dynamic_bitset starting from the position @p prev + 1.
	 *
	 * @details    Give the lowest index superior to @p prev of the @ref sul::dynamic_bitset with a
	 *             bit set, or @ref npos if no bits are set after the index @p prev.\n If @p prev +
	 *             1 \>= @ref size(), return @ref npos.
	 *
	 * @param[in]  prev  Position of the bit preceding the search range
	 *
	 * @return     The position of the first bit set after @p prev, or @ref npos if no bits are set
	 *             after @p prev
	 *
	 * @complexity Linear in @ref size() - @p prev.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr size_type find_next(size_type prev) const;

	/**
	 * @brief      Exchanges the bits of this @ref sul::dynamic_bitset with those of @p other.
	 *
	 * @details    All @ref reference on bits of the @ref sul::dynamic_bitset are invalidated.
	 *
	 * @param      other  @ref sul::dynamic_bitset to exchange bits with
	 *
	 * @complexity Constant.
	 */
	constexpr void swap(dynamic_bitset<Block, Allocator>& other);

	/**
	 * @brief      Gets the associated allocator.
	 *
	 * @return     The associated allocator.
	 *
	 * @complexity Constant.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr allocator_type get_allocator() const;

	/**
	 * @brief      Generate a string representation of the @ref sul::dynamic_bitset.
	 *
	 * @details    Uses @p zero to represent bits with value of @a false and @p one to represent
	 *             bits with value of @a true. The resulting string contains @ref size() characters
	 *             with the first character corresponds to the last (@ref size() - 1th) bit and the
	 *             last character corresponding to the first bit.
	 *
	 * @param[in]  zero     Character to use to represent @a false
	 * @param[in]  one      Character to use to represent @a true
	 *
	 * @tparam     _CharT   Character type of the string
	 * @tparam     _Traits  Traits class specifying the operations on the character type of the
	 *                      string
	 * @tparam     _Alloc   Allocator type used to allocate internal storage of the string
	 *
	 * @return     The string representing the @ref sul::dynamic_bitset content
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
	template<typename _CharT = char,
	         typename _Traits = std::char_traits<_CharT>,
	         typename _Alloc = std::allocator<_CharT>>
#if (__cplusplus >= 201703L)
		[[nodiscard]]
#endif
	constexpr std::basic_string<_CharT, _Traits, _Alloc> to_string(
	  _CharT zero = _CharT('0'),
	  _CharT one = _CharT('1')) const;

	/**
	 * @brief      Iterate on the @ref sul::dynamic_bitset and call @p function with the position of
	 *             the bits on.
	 *
	 * @details    For each set bit, @p function is called as follow:
	 *             @code
	 *             std::invoke(std::forward<Function>(function), bit_pos, std::forward<Parameters>(parameters)...))
	 *             @endcode
	 *             where @p bit_pos is the position of the current bit on. Thus @p function
	 *             should take a size_t for the current set bit position as first argument, also @p
	 *             parameters can be used to pass additional arguments to @p function when it is
	 *             called by this method.\n\n @p function can return nothing or a bool, if it return
	 *             a bool, the return value indicate if the iteration should continue, @a true to
	 *             continue the iteration, @a false to stop, this make it easy to do an early exit.
	 *
	 * @param      function    Function to call on all bits on, take the current bit position as
	 *                         first argument and @p parameters as next arguments
	 * @param      parameters  Extra parameters for @p function
	 *
	 * @tparam     Function    Type of @p function, must take a size_t as first argument and @p
	 *                         Parameters as next arguments
	 * @tparam     Parameters  Type of @p parameters
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
	template<typename Function, typename... Parameters>
	constexpr void iterate_bits_on(Function&& function, Parameters&&... parameters) const;

	/**
	 * @brief      Return a pointer to the underlying array serving as blocks storage.
	 *
	 * @details    The pointer is such that range [@ref data(); @ref data() + @ref num_blocks()) is
	 *             always a valid range, even if the container is empty (@ref data() is not
	 *             dereferenceable in that case).
	 *
	 *
	 * @post       The bits past the end of the @ref sul::dynamic_bitset in the last block are
	 *             guaranteed to be 0s, example:
	 *             @code
	 *             // random bitset of size 11
	 *             std::minstd_rand rand(std::random_device{}());
	 *             std::bernoulli_distribution dist;
	 *             sul::dynamic_bitset<uint8_t> bitset;
	 *             for(size_t i = 0; i < 11; ++i)
	 *             {
	 *                 bitset.push_back(dist(rand));
	 *             }
	 *
	 *             // the bitset use 2 blocks of 8 bits
	 *             // check that unused bits are set to 0
	 *             assert(*(bitset.data() + 1) >> 3 == 0);
	 *             @endcode
	 *
	 * @remark     If the @ref sul::dynamic_bitset is empty, this function may or may not return a
	 *             null pointer.
	 *
	 * @return     A pointer to the underlying array serving as blocks storage
	 *
	 * @complexity Constant.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]] constexpr
#endif
	block_type* data() noexcept;

	/**
	 * @brief      Return a pointer to the underlying array serving as blocks storage.
	 *
	 * @details    The pointer is such that range [@ref data(); @ref data() + @ref num_blocks()) is
	 *             always a valid range, even if the container is empty (@ref data() is not
	 *             dereferenceable in that case).
	 *
	 *
	 * @post       The bits past the end of the @ref sul::dynamic_bitset in the last block are
	 *             guaranteed to be 0s, example:
	 *             @code
	 *             // random bitset of size 11
	 *             std::minstd_rand rand(std::random_device{}());
	 *             std::bernoulli_distribution dist;
	 *             sul::dynamic_bitset<uint8_t> bitset;
	 *             for(size_t i = 0; i < 11; ++i)
	 *             {
	 *                 bitset.push_back(dist(rand));
	 *             }
	 *
	 *             // the bitset use 2 blocks of 8 bits
	 *             // check that unused bits are set to 0
	 *             assert(*(bitset.data() + 1) >> 3 == 0);
	 *             @endcode
	 *
	 * @remark     If the @ref sul::dynamic_bitset is empty, this function may or may not return a
	 *             null pointer.
	 *
	 * @return     A pointer to the underlying array serving as blocks storage
	 *
	 * @complexity Constant.
	 */
#if (__cplusplus >= 201703L)
	[[nodiscard]]
#endif
	constexpr const block_type* data() const noexcept;

	/**
	 * @brief      Test if two @ref sul::dynamic_bitset have the same content.
	 *
	 * @param[in]  lhs         The left hand side @ref sul::dynamic_bitset of the operator
	 * @param[in]  rhs         The right hand side @ref sul::dynamic_bitset of the operator
	 *
	 * @tparam     Block_      Block type used by @p lhs and @p rhs for storing the bits
	 * @tparam     Allocator_  Allocator type used by @p lhs and @p rhs for memory management
	 *
	 * @return     @a true if they contain the same bits, @a false otherwise
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
	template<typename Block_, typename Allocator_>
	friend constexpr bool operator==(const dynamic_bitset<Block_, Allocator_>& lhs,
	                                 const dynamic_bitset<Block_, Allocator_>& rhs);

	/**
	 * @brief      Test if @p lhs is "less than" @p rhs. The comparison of the two @ref
	 *             dynamic_bitset is first on numbers their content represent and then on their
	 *             size.
	 *
	 * @details    The size comparison is necessary for the comparison operators to keep their
	 *             properties. For example without the size comparison the "<=" operator (defined
	 *             for "A <= B" by "!(B < A)") would no longer be antisymmetric (if A \<= B and B
	 *             \<= A, then A == B) because @ref operator==() compare the @ref
	 *             sul::dynamic_bitset as a container and not a number. For example with bitsets
	 *             A(0011) and B(011), without the size comparison B \< A would be @a false, A \<= B
	 *             would be @a true, B \<= A would be @a true, but A == B would be @a false,
	 *             breaking the antisymmetric property of the operator. Thus, to respect the
	 *             properties of the operators, the size is used as a secondary criteria for the
	 *             comparison of @ref sul::dynamic_bitset which content represent the same number.
	 *             Therefore, for the previous example with bitsets A(0011) and B(011), B \< A is @a
	 *             true, A \<= B is @a false, B \<= A is @a true and A == B is @a false.\n\n If
	 *             comparing bitsets @a A and @a B with the content of @a A representing the number
	 *             @a a, and the content of @a B representing the number @a b, this operator would
	 *             work as follow:
	 *             @code
	 *             if(a == b)
	 *             {
	 *                 return A.size() < B.size();
	 *             }
	 *             else
	 *             {
	 *                 return a < b;
	 *             }
	 *             @endcode
	 *
	 * @remark     The empty @ref sul::dynamic_bitset is the "lowest" of all bitset and for 0-only
	 *             bitsets comparison, the shortest is the lowest.
	 *
	 * @param[in]  lhs         The left hand side @ref sul::dynamic_bitset of the operator
	 * @param[in]  rhs         The right hand side @ref sul::dynamic_bitset of the operator
	 *
	 * @tparam     Block_      Block type used by @p lhs and @p rhs for storing the bits
	 * @tparam     Allocator_  Allocator type used by @p lhs and @p rhs for memory management
	 *
	 * @return     @a true if @p lhs is "less than" @p rhs
	 *
	 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
	 */
	template<typename Block_, typename Allocator_>
	friend constexpr bool operator<(const dynamic_bitset<Block_, Allocator_>& lhs,
	                                const dynamic_bitset<Block_, Allocator_>& rhs);

private:
	template<typename T>
	struct dependent_false : public std::false_type
	{
	};

	std::vector<Block, Allocator> m_blocks;
	size_type m_bits_number;

	static constexpr block_type zero_block = block_type(0);
	static constexpr block_type one_block = block_type(~zero_block);
	static constexpr size_type block_last_bit_index = bits_per_block - 1;

	static constexpr size_type blocks_required(size_type nbits) noexcept;

	static constexpr size_type block_index(size_type pos) noexcept;
	static constexpr size_type bit_index(size_type pos) noexcept;

	static constexpr block_type bit_mask(size_type pos) noexcept;
	static constexpr block_type bit_mask(size_type first, size_type last) noexcept;

	static constexpr void set_block_bits(block_type& block,
	                                     size_type first,
	                                     size_type last,
	                                     bool val = true) noexcept;
	static constexpr void flip_block_bits(block_type& block,
	                                      size_type first,
	                                      size_type last) noexcept;

	static constexpr size_type block_count(const block_type& block) noexcept;
	static constexpr size_type block_count(const block_type& block, size_type nbits) noexcept;

	static constexpr size_type first_on(const block_type& block) noexcept;

#if (__cplusplus >= 201703L)
	template<typename _CharT, typename _Traits>
	constexpr void init_from_string(std::basic_string_view<_CharT, _Traits> str,
	                                typename std::basic_string_view<_CharT, _Traits>::size_type pos,
	                                typename std::basic_string_view<_CharT, _Traits>::size_type n,
	                                _CharT zero,
	                                _CharT one);
#endif
#if (__cplusplus >= 201703L)
	constexpr
#endif
		block_type& get_block(size_type pos);
	constexpr const block_type& get_block(size_type pos) const;
#if (__cplusplus >= 201703L)
	constexpr
#endif
		block_type& last_block();
	constexpr block_type last_block() const;

	// used bits in the last block
	constexpr size_type extra_bits_number() const noexcept;
	// unused bits in the last block
	constexpr size_type unused_bits_number() const noexcept;

	template<typename BinaryOperation>
	constexpr void apply(const dynamic_bitset<Block, Allocator>& other, BinaryOperation binary_op);
	template<typename UnaryOperation>
	constexpr void apply(UnaryOperation unary_op);
	constexpr void apply_left_shift(size_type shift);
	constexpr void apply_right_shift(size_type shift);

	// reset unused bits to 0
#if (__cplusplus >= 201703L)
	constexpr
#endif
		void sanitize();

	// check functions used in asserts
	constexpr bool check_unused_bits() const noexcept;
	constexpr bool check_size() const noexcept;
	constexpr bool check_consistency() const noexcept;
};

// Deduction guideline for expressions like "dynamic_bitset a(32);" with an integral type as parameter
// to use the constructor with the initial size instead of the constructor with the allocator.
#if (__cplusplus >= 201703L)
template<typename integral_type, typename = std::enable_if_t<std::is_integral_v<integral_type>>>
dynamic_bitset(integral_type) -> dynamic_bitset<>;
#endif

//=================================================================================================
// dynamic_bitset external functions declarations
//=================================================================================================

/**
 * @brief      Test if two @ref sul::dynamic_bitset content are different.
 *
 * @details    Defined as:
 *             @code
 *             return !(lhs == rhs);
 *             @endcode
 *             see @ref sul::dynamic_bitset::operator==() for more informations.
 *
 * @param[in]  lhs        The left hand side @ref sul::dynamic_bitset of the operator
 * @param[in]  rhs        The right hand side @ref sul::dynamic_bitset of the operator
 *
 * @tparam     Block      Block type used by @p lhs and @p rhs for storing the bits
 * @tparam     Allocator  Allocator type used by @p lhs and @p rhs for memory management
 *
 * @return     @a true if they does not contain the same bits, @a false otherwise
 *
 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
 *
 * @relatesalso dynamic_bitset
 */
template<typename Block, typename Allocator>
constexpr bool operator!=(const dynamic_bitset<Block, Allocator>& lhs,
                          const dynamic_bitset<Block, Allocator>& rhs);

/**
 * @brief      Test if @p lhs is "less than or equal to" @p rhs. The comparison of the two @ref
 *             sul::dynamic_bitset is first on numbers their content represent and then on their size.
 *
 * @details    Defined as:
 *             @code
 *             return !(rhs < lhs);
 *             @endcode
 *             see @ref sul::dynamic_bitset::operator<() for more informations.
 *
 * @param[in]  lhs        The left hand side @ref sul::dynamic_bitset of the operator
 * @param[in]  rhs        The right hand side @ref sul::dynamic_bitset of the operator
 *
 * @tparam     Block      Block type used by @p lhs and @p rhs for storing the bits
 * @tparam     Allocator  Allocator type used by @p lhs and @p rhs for memory management
 *
 * @return     @a true if @p lhs is "less than or equal to" @p rhs
 *
 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
 *
 * @relatesalso dynamic_bitset
 */
template<typename Block, typename Allocator>
constexpr bool operator<=(const dynamic_bitset<Block, Allocator>& lhs,
                          const dynamic_bitset<Block, Allocator>& rhs);

/**
 * @brief      Test if @p lhs is "greater than" @p rhs. The comparison of the two @ref
 *             sul::dynamic_bitset is first on numbers their content represent and then on their
 *             size.
 *
 * @details    Defined as:
 *             @code
 *             return rhs < lhs;
 *             @endcode
 *             see @ref sul::dynamic_bitset::operator<() for more informations.
 *
 * @param[in]  lhs        The left hand side @ref sul::dynamic_bitset of the operator
 * @param[in]  rhs        The right hand side @ref sul::dynamic_bitset of the operator
 *
 * @tparam     Block      Block type used by @p lhs and @p rhs for storing the bits
 * @tparam     Allocator  Allocator type used by @p lhs and @p rhs for memory management
 *
 * @return     @a true if @p lhs is "greater than" @p rhs
 *
 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
 *
 * @relatesalso dynamic_bitset
 */
template<typename Block, typename Allocator>
constexpr bool operator>(const dynamic_bitset<Block, Allocator>& lhs,
                         const dynamic_bitset<Block, Allocator>& rhs);

/**
 * @brief      Test if @p lhs is "greater than or equal to" @p rhs. The comparison of the two @ref
 *             sul::dynamic_bitset is first on numbers their content represent and then on their
 *             size.
 *
 * @details    Defined as:
 *             @code
 *             return !(lhs < rhs);
 *             @endcode
 *             see @ref sul::dynamic_bitset::operator<() for more informations.
 *
 * @param[in]  lhs        The left hand side @ref sul::dynamic_bitset of the operator
 * @param[in]  rhs        The right hand side @ref sul::dynamic_bitset of the operator
 *
 * @tparam     Block      Block type used by @p lhs and @p rhs for storing the bits
 * @tparam     Allocator  Allocator type used by @p lhs and @p rhs for memory management
 *
 * @return     @a true if @p lhs is "greater than or equal to" @p rhs
 *
 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
 *
 * @relatesalso dynamic_bitset
 */
template<typename Block, typename Allocator>
constexpr bool operator>=(const dynamic_bitset<Block, Allocator>& lhs,
                          const dynamic_bitset<Block, Allocator>& rhs);

/**
 * @brief      Performs binary AND on corresponding pairs of bits of @p lhs and @p rhs.
 *
 * @details    Defined as:
 *             @code
 *             dynamic_bitset<Block, Allocator> result(lhs);
 *             return result &= rhs;
 *             @endcode
 *             see @ref sul::dynamic_bitset::operator&=() for more informations.
 *
 * @param[in]  lhs        The left hand side @ref sul::dynamic_bitset of the operator
 * @param[in]  rhs        The right hand side @ref sul::dynamic_bitset of the operator
 *
 * @tparam     Block      Block type used by @p lhs and @p rhs for storing the bits
 * @tparam     Allocator  Allocator type used by @p lhs and @p rhs for memory management
 *
 * @return     A @ref sul::dynamic_bitset with each bit being the result of a binary AND between the
 *             corresponding pair of bits of @p lhs and @p rhs
 *
 * @pre        @code
 *             lhs.size() == rhs.size()
 *             @endcode
 *
 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
 *
 * @relatesalso dynamic_bitset
 */
template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator> operator&(const dynamic_bitset<Block, Allocator>& lhs,
                                                     const dynamic_bitset<Block, Allocator>& rhs);

/**
 * @brief      Performs binary OR on corresponding pairs of bits of @p lhs and @p rhs.
 *
 * @details    Defined as:
 *             @code
 *             dynamic_bitset<Block, Allocator> result(lhs);
 *             return result |= rhs;
 *             @endcode
 *             see @ref sul::dynamic_bitset::operator|=() for more informations.
 *
 * @param[in]  lhs        The left hand side @ref sul::dynamic_bitset of the operator
 * @param[in]  rhs        The right hand side @ref sul::dynamic_bitset of the operator
 *
 * @tparam     Block      Block type used by @p lhs and @p rhs for storing the bits
 * @tparam     Allocator  Allocator type used by @p lhs and @p rhs for memory management
 *
 * @return     A @ref sul::dynamic_bitset with each bit being the result of a binary OR between the
 *             corresponding pair of bits of @p lhs and @p rhs
 *
 * @pre        @code
 *             lhs.size() == rhs.size()
 *             @endcode
 *
 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
 *
 * @relatesalso dynamic_bitset
 */
template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator> operator|(const dynamic_bitset<Block, Allocator>& lhs,
                                                     const dynamic_bitset<Block, Allocator>& rhs);

/**
 * @brief      Performs binary XOR on corresponding pairs of bits of @p lhs and @p rhs.
 *
 * @details    Defined as:
 *             @code
 *             dynamic_bitset<Block, Allocator> result(lhs);
 *             return result ^= rhs;
 *             @endcode
 *             see @ref sul::dynamic_bitset::operator^=() for more informations.
 *
 * @param[in]  lhs        The left hand side @ref sul::dynamic_bitset of the operator
 * @param[in]  rhs        The right hand side @ref sul::dynamic_bitset of the operator
 *
 * @tparam     Block      Block type used by @p lhs and @p rhs for storing the bits
 * @tparam     Allocator  Allocator type used by @p lhs and @p rhs for memory management
 *
 * @return     A @ref sul::dynamic_bitset with each bit being the result of a binary XOR between the
 *             corresponding pair of bits of @p lhs and @p rhs
 *
 * @pre        @code
 *             lhs.size() == rhs.size()
 *             @endcode
 *
 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
 *
 * @relatesalso dynamic_bitset
 */
template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator> operator^(const dynamic_bitset<Block, Allocator>& lhs,
                                                     const dynamic_bitset<Block, Allocator>& rhs);

/**
 * @brief      Performs binary difference between bits of @p lhs and @p rhs.
 *
 * @details    Defined as:
 *             @code
 *             dynamic_bitset<Block, Allocator> result(lhs);
 *             return result -= rhs;
 *             @endcode
 *             see @ref sul::dynamic_bitset::operator-=() for more informations.
 *
 * @param[in]  lhs        The left hand side @ref sul::dynamic_bitset of the operator
 * @param[in]  rhs        The right hand side @ref sul::dynamic_bitset of the operator
 *
 * @tparam     Block      Block type used by @p lhs and @p rhs for storing the bits
 * @tparam     Allocator  Allocator type used by @p lhs and @p rhs for memory management
 *
 * @return     A @ref sul::dynamic_bitset with each bit being the result of the binary difference
 *             between the corresponding bits of @p lhs and @p rhs
 *
 * @pre        @code
 *             lhs.size() == rhs.size()
 *             @endcode
 *
 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
 *
 * @relatesalso dynamic_bitset
 */
template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator> operator-(const dynamic_bitset<Block, Allocator>& lhs,
                                                     const dynamic_bitset<Block, Allocator>& rhs);

/**
 * @brief      Insert a string representation of this @ref sul::dynamic_bitset to a character
 *             stream.
 *
 * @details    The string representation written is the same as if generated with @ref
 *             sul::dynamic_bitset::to_string() with default parameter, using '1' for @a true bits
 *             and '0' for @a false bits.
 *
 * @param      os         Character stream to write to
 * @param[in]  bitset     @ref sul::dynamic_bitset to write
 *
 * @tparam     _CharT     Character type of the character stream
 * @tparam     _Traits    Traits class specifying the operations on the character type of the
 *                        character stream
 * @tparam     Block      Block type used by @p bitset for storing the bits
 * @tparam     Allocator  Allocator type used by @p bitset for memory management
 *
 * @return     @p os
 *
 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
 *
 * @relatesalso dynamic_bitset
 */
template<typename _CharT, typename _Traits, typename Block, typename Allocator>
constexpr std::basic_ostream<_CharT, _Traits>& operator<<(
  std::basic_ostream<_CharT, _Traits>& os,
  const dynamic_bitset<Block, Allocator>& bitset);

/**
 * @brief      Extract a @ref sul::dynamic_bitset from a character stream using its string
 *             representation.
 *
 * @details    The string representation expected is the same as if generated with @ref
 *             sul::dynamic_bitset::to_string() with default parameter, using '1' for @a true bits
 *             and '0' for @a false bits. On success the content of @p bitset is cleared before
 *             writing to it. The extraction starts by skipping leading whitespace then take the
 *             characters one by one and stop if @p is.good() return @a false or the next character
 *             is neither _CharT('0') nor _CharT('1').
 *
 * @param      is         Character stream to read from
 * @param      bitset     @ref sul::dynamic_bitset to write to
 *
 * @tparam     _CharT     Character type of the character stream
 * @tparam     _Traits    Traits class specifying the operations on the character type of the
 *                        character stream
 * @tparam     Block      Block type used by @p bitset for storing the bits
 * @tparam     Allocator  Allocator type used by @p bitset for memory management
 *
 * @return     @p is
 *
 * @complexity Linear in the size of the @ref sul::dynamic_bitset.
 *
 * @relatesalso dynamic_bitset
 */
template<typename _CharT, typename _Traits, typename Block, typename Allocator>
constexpr std::basic_istream<_CharT, _Traits>& operator>>(std::basic_istream<_CharT, _Traits>& is,
                                                          dynamic_bitset<Block, Allocator>& bitset);

/**
 * @brief      Exchange the content of @p bitset1 and @p bitset2.
 *
 * @details    Defined as:
 *             @code
 *             bitset1.swap(bitset2);
 *             @endcode
 *             see @ref sul::dynamic_bitset::swap() for more informations.
 *
 * @param      bitset1    @ref sul::dynamic_bitset to be swapped
 * @param      bitset2    @ref sul::dynamic_bitset to be swapped
 *
 * @tparam     Block      Block type used by @p bitset for storing the bits
 * @tparam     Allocator  Allocator type used by @p bitset for memory management
 *
 * @relatesalso dynamic_bitset
 */
template<typename Block, typename Allocator>
constexpr void swap(dynamic_bitset<Block, Allocator>& bitset1,
                    dynamic_bitset<Block, Allocator>& bitset2);

//=================================================================================================
// dynamic_bitset::reference functions implementations
//=================================================================================================

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator>::reference::reference(
  dynamic_bitset<Block, Allocator>& bitset,
  size_type bit_pos)
  : m_block(bitset.get_block(bit_pos)), m_mask(dynamic_bitset<Block, Allocator>::bit_mask(bit_pos))
{
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::reference& dynamic_bitset<Block, Allocator>::
  reference::operator=(bool v)
{
	assign(v);
	return *this;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::reference& dynamic_bitset<Block, Allocator>::
  reference::operator=(const dynamic_bitset<Block, Allocator>::reference& rhs)
{
	assign(rhs);
	return *this;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::reference& dynamic_bitset<Block, Allocator>::
  reference::operator=(dynamic_bitset::reference&& rhs) noexcept
{
	assign(rhs);
	return *this;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::reference& dynamic_bitset<Block, Allocator>::
  reference::operator&=(bool v)
{
	if(!v)
	{
		reset();
	}
	return *this;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::reference& dynamic_bitset<Block, Allocator>::
  reference::operator|=(bool v)
{
	if(v)
	{
		set();
	}
	return *this;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::reference& dynamic_bitset<Block, Allocator>::
  reference::operator^=(bool v)
{
	if(v)
	{
		flip();
	}
	return *this;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::reference& dynamic_bitset<Block, Allocator>::
  reference::operator-=(bool v)
{
	if(v)
	{
		reset();
	}
	return *this;
}

template<typename Block, typename Allocator>
constexpr bool dynamic_bitset<Block, Allocator>::reference::operator~() const
{
	return (m_block & m_mask) == zero_block;
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator>::reference::operator bool() const
{
	return (m_block & m_mask) != zero_block;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::reference& dynamic_bitset<Block, Allocator>::
  reference::set()
{
	m_block |= m_mask;
	return *this;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::reference& dynamic_bitset<Block, Allocator>::
  reference::reset()
{
	m_block &= ~m_mask;
	return *this;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::reference& dynamic_bitset<Block, Allocator>::
  reference::flip()
{
	m_block ^= m_mask;
	return *this;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::reference& dynamic_bitset<Block, Allocator>::
  reference::assign(bool v)
{
	if(v)
	{
		set();
	}
	else
	{
		reset();
	}
	return *this;
}

//=================================================================================================
// dynamic_bitset public functions implementations
//=================================================================================================

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator>::dynamic_bitset(const allocator_type& allocator)
  : m_blocks(allocator), m_bits_number(0)
{
}

template<typename Block, typename Allocator>
#if (__cplusplus >= 201703L)
constexpr
#endif 
dynamic_bitset<Block, Allocator>::dynamic_bitset(size_type nbits,
                                                           unsigned long long init_val,
                                                           const allocator_type& allocator)
  : m_blocks(blocks_required(nbits), allocator), m_bits_number(nbits)
{
	if(nbits == 0 || init_val == 0)
	{
		return;
	}

	constexpr size_type init_val_required_blocks = sizeof(unsigned long long) / sizeof(block_type);
	if /*constexpr*/(init_val_required_blocks == 1)
	{
		m_blocks[0] = init_val;
	}
	else
	{
		const unsigned long long block_mask = static_cast<unsigned long long>(one_block);
		const size_type blocks_to_init = std::min(m_blocks.size(), init_val_required_blocks);
		for(size_type i = 0; i < blocks_to_init; ++i)
		{
			m_blocks[i] = block_type((init_val >> (i * bits_per_block) & block_mask));
		}
	}
	sanitize();
}

template<typename Block, typename Allocator>
#if (__cplusplus >= 201703L)
constexpr
#endif 
dynamic_bitset<Block, Allocator>::dynamic_bitset(
  std::initializer_list<block_type> init_vals,
  const allocator_type& allocator)
  : m_blocks(allocator), m_bits_number(0)
{
	append(init_vals);
}

#if (__cplusplus >= 201703L)
template<typename Block, typename Allocator>
template<typename _CharT, typename _Traits>
constexpr dynamic_bitset<Block, Allocator>::dynamic_bitset(
  std::basic_string_view<_CharT, _Traits> str,
  typename std::basic_string_view<_CharT, _Traits>::size_type pos,
  typename std::basic_string_view<_CharT, _Traits>::size_type n,
  _CharT zero,
  _CharT one,
  const allocator_type& allocator)
  : m_blocks(allocator), m_bits_number(0)
{
	assert(pos < str.size());
	init_from_string(str, pos, n, zero, one);
}

template<typename Block, typename Allocator>
template<typename _CharT, typename _Traits, typename _Alloc>
constexpr dynamic_bitset<Block, Allocator>::dynamic_bitset(
  const std::basic_string<_CharT, _Traits, _Alloc>& str,
  typename std::basic_string<_CharT, _Traits, _Alloc>::size_type pos,
  typename std::basic_string<_CharT, _Traits, _Alloc>::size_type n,
  _CharT zero,
  _CharT one,
  const allocator_type& allocator)
  : m_blocks(allocator), m_bits_number(0)
{
	assert(pos < str.size());
	init_from_string(std::basic_string_view<_CharT, _Traits>(str), pos, n, zero, one);
}

template<typename Block, typename Allocator>
template<typename _CharT, typename _Traits>
constexpr dynamic_bitset<Block, Allocator>::dynamic_bitset(
  const _CharT* str,
  typename std::basic_string<_CharT>::size_type pos,
  typename std::basic_string<_CharT>::size_type n,
  _CharT zero,
  _CharT one,
  const allocator_type& allocator)
  : m_blocks(allocator), m_bits_number(0)
{
	init_from_string(std::basic_string_view<_CharT, _Traits>(str), pos, n, zero, one);
}
#endif

template<typename Block, typename Allocator>
#if (__cplusplus >= 201703L)
constexpr
#endif
void dynamic_bitset<Block, Allocator>::resize(size_type nbits, bool value)
{
	if(nbits == m_bits_number)
	{
		return;
	}

	const size_type old_num_blocks = num_blocks();
	const size_type new_num_blocks = blocks_required(nbits);

	const block_type init_value = value ? one_block : zero_block;
	if(new_num_blocks != old_num_blocks)
	{
		m_blocks.resize(new_num_blocks, init_value);
	}

	if(value && nbits > m_bits_number && old_num_blocks > 0)
	{
		// set value of the new bits in the old last block
		const size_type extra_bits = extra_bits_number();
		if(extra_bits > 0)
		{
			m_blocks[old_num_blocks - 1] |= (init_value << extra_bits);
		}
	}

	m_bits_number = nbits;
	sanitize();
	assert(check_consistency());
}

template<typename Block, typename Allocator>
constexpr void dynamic_bitset<Block, Allocator>::clear()
{
	m_blocks.clear();
	m_bits_number = 0;
}

template<typename Block, typename Allocator>
#if (__cplusplus >= 201703L)
constexpr
#endif 
void dynamic_bitset<Block, Allocator>::push_back(bool value)
{
	const size_type new_last_bit = m_bits_number++;
	if(m_bits_number <= m_blocks.size() * bits_per_block)
	{
		if(value)
		{
			set(new_last_bit, value);
		}
	}
	else
	{
		m_blocks.push_back(block_type(value));
	}
	assert(operator[](new_last_bit) == value);
	assert(check_consistency());
}

template<typename Block, typename Allocator>
#if (__cplusplus >= 201703L)
constexpr
#endif 
void dynamic_bitset<Block, Allocator>::pop_back()
{
	if(empty())
	{
		return;
	}

	--m_bits_number;
	if(m_blocks.size() > blocks_required(m_bits_number))
	{
		m_blocks.pop_back();
		// no extra bits: sanitize not required
		assert(extra_bits_number() == 0);
	}
	else
	{
		sanitize();
	}
	assert(check_consistency());
}

template<typename Block, typename Allocator>
constexpr void dynamic_bitset<Block, Allocator>::append(block_type block)
{
	const size_type extra_bits = extra_bits_number();
	if(extra_bits == 0)
	{
		m_blocks.push_back(block);
	}
	else
	{
		last_block() |= (block << extra_bits);
		m_blocks.push_back(block_type(block >> (bits_per_block - extra_bits)));
	}

	m_bits_number += bits_per_block;
	assert(check_consistency());
}

template<typename Block, typename Allocator>
constexpr void dynamic_bitset<Block, Allocator>::append(std::initializer_list<block_type> blocks)
{
	if(blocks.size() == 0)
	{
		return;
	}

	append(blocks.cbegin(), blocks.cend());
}

template<typename Block, typename Allocator>
template<typename BlockInputIterator>
constexpr void dynamic_bitset<Block, Allocator>::append(BlockInputIterator first,
                                                        BlockInputIterator last)
{
	if(first == last)
	{
		return;
	}

	// if random access iterators, std::distance complexity is constant
	if /*constexpr*/(std::is_same<
	               typename std::iterator_traits<BlockInputIterator>::iterator_category,
	               std::random_access_iterator_tag>::value)
	{
		assert(std::distance(first, last) > 0);
		m_blocks.reserve(m_blocks.size() + static_cast<size_type>(std::distance(first, last)));
	}

	const size_type extra_bits = extra_bits_number();
	const size_type unused_bits = unused_bits_number();
	if(extra_bits == 0)
	{
		auto pos = m_blocks.insert(std::end(m_blocks), first, last);
		assert(std::distance(pos, std::end(m_blocks)) > 0);
		m_bits_number +=
		  static_cast<size_type>(std::distance(pos, std::end(m_blocks))) * bits_per_block;
	}
	else
	{
		last_block() |= (*first << extra_bits);
		block_type block = block_type(*first >> unused_bits);
		++first;
		while(first != last)
		{
			block |= (*first << extra_bits);
			m_blocks.push_back(block);
			m_bits_number += bits_per_block;
			block = block_type(*first >> unused_bits);
			++first;
		}
		m_blocks.push_back(block);
		m_bits_number += bits_per_block;
	}

	assert(check_consistency());
}
template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::operator&=(
  const dynamic_bitset<Block, Allocator>& rhs)
{
	assert(size() == rhs.size());
	//apply(rhs, std::bit_and());
	for(size_type i = 0; i < m_blocks.size(); ++i)
	{
		m_blocks[i] &= rhs.m_blocks[i];
	}
	return *this;
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::operator|=(
  const dynamic_bitset<Block, Allocator>& rhs)
{
	assert(size() == rhs.size());
	//apply(rhs, std::bit_or());
	for(size_type i = 0; i < m_blocks.size(); ++i)
	{
		m_blocks[i] |= rhs.m_blocks[i];
	}
	return *this;
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::operator^=(
  const dynamic_bitset<Block, Allocator>& rhs)
{
	assert(size() == rhs.size());
	//apply(rhs, std::bit_xor());
	for(size_type i = 0; i < m_blocks.size(); ++i)
	{
		m_blocks[i] ^= rhs.m_blocks[i];
	}
	return *this;
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::operator-=(
  const dynamic_bitset<Block, Allocator>& rhs)
{
	assert(size() == rhs.size());
	//apply(rhs, [](const block_type& x, const block_type& y) { return (x & ~y); });
	for(size_type i = 0; i < m_blocks.size(); ++i)
	{
		m_blocks[i] &= ~rhs.m_blocks[i];
	}
	return *this;
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::operator<<=(
  size_type shift)
{
	if(shift != 0)
	{
		if(shift >= m_bits_number)
		{
			reset();
		}
		else
		{
			apply_left_shift(shift);
			sanitize(); // unused bits can have changed, reset them to 0
		}
	}
	return *this;
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::operator>>=(
  size_type shift)
{
	if(shift != 0)
	{
		if(shift >= m_bits_number)
		{
			reset();
		}
		else
		{
			apply_right_shift(shift);
		}
	}
	return *this;
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator> dynamic_bitset<Block, Allocator>::operator<<(
  size_type shift) const
{
	return dynamic_bitset<Block, Allocator>(*this) <<= shift;
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator> dynamic_bitset<Block, Allocator>::operator>>(
  size_type shift) const
{
	return dynamic_bitset<Block, Allocator>(*this) >>= shift;
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator> dynamic_bitset<Block, Allocator>::operator~() const
{
	dynamic_bitset<Block, Allocator> bitset(*this);
	bitset.flip();
	return bitset;
}

template<typename Block, typename Allocator>
#if (__cplusplus >= 201703L)
constexpr
#endif
dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::set(size_type pos,
                                                                                  size_type len,
                                                                                  bool value)
{
	assert(pos < size());
	if(len == 0)
	{
		return *this;
	}
	assert(pos + len - 1 < size());

	const size_type first_block = block_index(pos);
	const size_type last_block = block_index(pos + len - 1);
	const size_type first_bit_index = bit_index(pos);
	const size_type last_bit_index = bit_index(pos + len - 1);

	if(first_block == last_block)
	{
		set_block_bits(m_blocks[first_block], first_bit_index, last_bit_index, value);
	}
	else
	{
		size_type first_full_block = first_block;
		size_type last_full_block = last_block;

		if(first_bit_index != 0)
		{
			++first_full_block; // first block is not full
			set_block_bits(m_blocks[first_block], first_bit_index, block_last_bit_index, value);
		}

		if(last_bit_index != block_last_bit_index)
		{
			--last_full_block; // last block is not full
			set_block_bits(m_blocks[last_block], 0, last_bit_index, value);
		}

		const block_type full_block = value ? one_block : zero_block;
		for(size_type i = first_full_block; i <= last_full_block; ++i)
		{
			m_blocks[i] = full_block;
		}
	}

	return *this;
}

template<typename Block, typename Allocator>
#if (__cplusplus >= 201703L)
constexpr
#endif
dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::set(size_type pos,
                                                                                  bool value)
{
	assert(pos < size());

	if(value)
	{
		m_blocks[block_index(pos)] |= bit_mask(pos);
	}
	else
	{
		m_blocks[block_index(pos)] &= ~bit_mask(pos);
	}

	return *this;
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::set()
{
	std::fill(std::begin(m_blocks), std::end(m_blocks), one_block);
	sanitize();
	return *this;
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::reset(size_type pos,
                                                                                    size_type len)
{
	return set(pos, len, false);
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::reset(size_type pos)
{
	return set(pos, false);
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::reset()
{
	std::fill(std::begin(m_blocks), std::end(m_blocks), zero_block);
	return *this;
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::flip(size_type pos,
                                                                                   size_type len)
{
	assert(pos < size());
	if(len == 0)
	{
		return *this;
	}
	assert(pos + len - 1 < size());

	const size_type first_block = block_index(pos);
	const size_type last_block = block_index(pos + len - 1);
	const size_type first_bit_index = bit_index(pos);
	const size_type last_bit_index = bit_index(pos + len - 1);

	if(first_block == last_block)
	{
		flip_block_bits(m_blocks[first_block], first_bit_index, last_bit_index);
	}
	else
	{
		size_type first_full_block = first_block;
		size_type last_full_block = last_block;

		if(first_bit_index != 0)
		{
			++first_full_block; // first block is not full
			flip_block_bits(m_blocks[first_block], first_bit_index, block_last_bit_index);
		}

		if(last_bit_index != block_last_bit_index)
		{
			--last_full_block; // last block is not full
			flip_block_bits(m_blocks[last_block], 0, last_bit_index);
		}

		for(size_type i = first_full_block; i <= last_full_block; ++i)
		{
			m_blocks[i] = block_type(~m_blocks[i]);
		}
	}

	return *this;
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::flip(size_type pos)
{
	assert(pos < size());
	m_blocks[block_index(pos)] ^= bit_mask(pos);
	return *this;
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::flip()
{
	std::transform(
	  m_blocks.cbegin(), m_blocks.cend(), m_blocks.begin(), std::bit_not<block_type>());
	sanitize();
	return *this;
}

template<typename Block, typename Allocator>
constexpr bool dynamic_bitset<Block, Allocator>::test(size_type pos) const
{
	assert(pos < size());
	return (m_blocks[block_index(pos)] & bit_mask(pos)) != zero_block;
}

template<typename Block, typename Allocator>
constexpr bool dynamic_bitset<Block, Allocator>::test_set(size_type pos, bool value)
{
	bool const result = test(pos);
	if(result != value)
	{
		set(pos, value);
	}
	return result;
}

template<typename Block, typename Allocator>
constexpr bool dynamic_bitset<Block, Allocator>::all() const
{
	if(empty())
	{
		return true;
	}

	const block_type full_block = one_block;
	if(extra_bits_number() == 0)
	{
		for(const block_type& block: m_blocks)
		{
			if(block != full_block)
			{
				return false;
			}
		}
	}
	else
	{
		for(size_type i = 0; i < m_blocks.size() - 1; ++i)
		{
			if(m_blocks[i] != full_block)
			{
				return false;
			}
		}
		if(last_block() != (full_block >> unused_bits_number()))
		{
			return false;
		}
	}
	return true;
}

template<typename Block, typename Allocator>
constexpr bool dynamic_bitset<Block, Allocator>::any() const
{
	for(const block_type& block: m_blocks)
	{
		if(block != zero_block)
		{
			return true;
		}
	}
	return false;
}

template<typename Block, typename Allocator>
constexpr bool dynamic_bitset<Block, Allocator>::none() const
{
	return !any();
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::size_type dynamic_bitset<Block,
                                                                              Allocator>::count()
  const noexcept
{
	if(empty())
	{
		return 0;
	}

#ifdef DYNAMIC_BITSET_USE_LIBPOPCNT
	const size_type count =
	  static_cast<size_type>(popcnt(m_blocks.data(), m_blocks.size() * sizeof(block_type)));
#else
	size_type count = 0;

	// full blocks
	for(size_type i = 0; i < m_blocks.size() - 1; ++i)
	{
		count += block_count(m_blocks[i]);
	}

	// last block
	const block_type& block = last_block();
	if(block != zero_block)
	{
		const size_t extra_bits = extra_bits_number();
		if(extra_bits == 0)
		{
			count += block_count(block);
		}
		else
		{
			count += block_count(block, extra_bits);
		}
	}
#endif
	return count;
}

template<typename Block, typename Allocator>
#if (__cplusplus >= 201703L)
constexpr
#endif
typename dynamic_bitset<Block, Allocator>::reference dynamic_bitset<Block, Allocator>::
operator[](size_type pos)
{
	assert(pos < size());
	return dynamic_bitset<Block, Allocator>::reference(*this, pos);
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::const_reference dynamic_bitset<
  Block,
  Allocator>::operator[](size_type pos) const
{
	return test(pos);
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::size_type dynamic_bitset<Block,
                                                                              Allocator>::size()
  const noexcept
{
	return m_bits_number;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::size_type dynamic_bitset<Block, Allocator>::
  num_blocks() const noexcept
{
	return m_blocks.size();
}

template<typename Block, typename Allocator>
constexpr bool dynamic_bitset<Block, Allocator>::empty() const noexcept
{
	return size() == 0;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::size_type dynamic_bitset<Block,
                                                                              Allocator>::capacity()
  const noexcept
{
	return m_blocks.capacity() * bits_per_block;
}

template<typename Block, typename Allocator>
constexpr void dynamic_bitset<Block, Allocator>::reserve(size_type num_bits)
{
	m_blocks.reserve(blocks_required(num_bits));
}

template<typename Block, typename Allocator>
constexpr void dynamic_bitset<Block, Allocator>::shrink_to_fit()
{
	m_blocks.shrink_to_fit();
}

template<typename Block, typename Allocator>
constexpr bool dynamic_bitset<Block, Allocator>::is_subset_of(
  const dynamic_bitset<Block, Allocator>& bitset) const
{
	assert(size() == bitset.size());
	for(size_type i = 0; i < m_blocks.size(); ++i)
	{
		if((m_blocks[i] & ~bitset.m_blocks[i]) != zero_block)
		{
			return false;
		}
	}
	return true;
}

template<typename Block, typename Allocator>
constexpr bool dynamic_bitset<Block, Allocator>::is_proper_subset_of(
  const dynamic_bitset<Block, Allocator>& bitset) const
{
	assert(size() == bitset.size());
	bool is_proper = false;
	for(size_type i = 0; i < m_blocks.size(); ++i)
	{
		const block_type& self_block = m_blocks[i];
		const block_type& other_block = bitset.m_blocks[i];

		if((self_block & ~other_block) != zero_block)
		{
			return false;
		}
		if((~self_block & other_block) != zero_block)
		{
			is_proper = true;
		}
	}
	return is_proper;
}

template<typename Block, typename Allocator>
constexpr bool dynamic_bitset<Block, Allocator>::intersects(
  const dynamic_bitset<Block, Allocator>& bitset) const
{
	const size_type min_blocks_number = std::min(m_blocks.size(), bitset.m_blocks.size());
	for(size_type i = 0; i < min_blocks_number; ++i)
	{
		if((m_blocks[i] & bitset.m_blocks[i]) != zero_block)
		{
			return true;
		}
	}
	return false;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::size_type dynamic_bitset<Block, Allocator>::
  find_first() const
{
	for(size_type i = 0; i < m_blocks.size(); ++i)
	{
		if(m_blocks[i] != zero_block)
		{
			return i * bits_per_block + first_on(m_blocks[i]);
		}
	}
	return npos;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::size_type dynamic_bitset<Block, Allocator>::
  find_next(size_type prev) const
{
	if(empty() || prev >= (size() - 1))
	{
		return npos;
	}

	const size_type first_bit = prev + 1;
	const size_type first_block = block_index(first_bit);
	const size_type first_bit_index = bit_index(first_bit);
	const block_type first_block_shifted = block_type(m_blocks[first_block] >> first_bit_index);

	if(first_block_shifted != zero_block)
	{
		return first_bit + first_on(first_block_shifted);
	}
	else
	{
		for(size_type i = first_block + 1; i < m_blocks.size(); ++i)
		{
			if(m_blocks[i] != zero_block)
			{
				return i * bits_per_block + first_on(m_blocks[i]);
			}
		}
	}
	return npos;
}

template<typename Block, typename Allocator>
constexpr void dynamic_bitset<Block, Allocator>::swap(dynamic_bitset<Block, Allocator>& other)
{
	std::swap(m_blocks, other.m_blocks);
	std::swap(m_bits_number, other.m_bits_number);
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::allocator_type dynamic_bitset<
  Block,
  Allocator>::get_allocator() const
{
	return m_blocks.get_allocator();
}

template<typename Block, typename Allocator>
template<typename _CharT, typename _Traits, typename _Alloc>
constexpr std::basic_string<_CharT, _Traits, _Alloc> dynamic_bitset<Block, Allocator>::to_string(
  _CharT zero,
  _CharT one) const
{
	const size_type len = size();
	std::basic_string<_CharT, _Traits, _Alloc> str(len, zero);
	for(size_type i_block = 0; i_block < m_blocks.size(); ++i_block)
	{
		if(m_blocks[i_block] == zero_block)
		{
			continue;
		}
		block_type mask = block_type(1);
		const size_type limit =
		  i_block * bits_per_block < len ? len - i_block * bits_per_block : bits_per_block;
		for(size_type i_bit = 0; i_bit < limit; ++i_bit)
		{
			if((m_blocks[i_block] & mask) != zero_block)
			{
				_Traits::assign(str[len - (i_block * bits_per_block + i_bit + 1)], one);
			}
			mask <<= 1;
		}
	}
	return str;
}

#if (__cplusplus >= 201703L)
template<typename Block, typename Allocator>
template<typename Function, typename... Parameters>
constexpr void dynamic_bitset<Block, Allocator>::iterate_bits_on(Function&& function,
                                                                 Parameters&&... parameters) const
{
	if /*constexpr*/(!std::is_invocable_v<Function, size_t, Parameters...>)
	{
		static_assert(dependent_false<Function>::value, "Function take invalid arguments");
		// function should take (size_t, parameters...) as arguments
	}

	if /*constexpr*/(std::is_same<std::invoke_result_t<Function, size_t, Parameters...>::value, void>)
	{
		size_t i_bit = find_first();
		while(i_bit != npos)
		{
			std::invoke(
			  std::forward<Function>(function), i_bit, std::forward<Parameters>(parameters)...);
			i_bit = find_next(i_bit);
		}
	}
	else if /*constexpr*/(std::is_convertible_v<std::invoke_result_t<Function, size_t, Parameters...>,
	                                        bool>)
	{
		size_t i_bit = find_first();
		while(i_bit != npos)
		{
			if(!std::invoke(
			     std::forward<Function>(function), i_bit, std::forward<Parameters>(parameters)...))
			{
				break;
			}
			i_bit = find_next(i_bit);
		}
	}
	else
	{
		static_assert(dependent_false<Function>::value, "Function have invalid return type");
		// return type should be void, or convertible to bool
	}
}
#endif

template<typename Block, typename Allocator>
#if (__cplusplus >= 201703L)
constexpr
#endif
typename dynamic_bitset<Block, Allocator>::block_type* dynamic_bitset<Block, Allocator>::
  data() noexcept
{
	return m_blocks.data();
}

template<typename Block, typename Allocator>
constexpr const typename dynamic_bitset<Block, Allocator>::block_type* dynamic_bitset<
  Block,
  Allocator>::data() const noexcept
{
	return m_blocks.data();
}

template<typename Block_, typename Allocator_>
#if (__cplusplus >= 201703L)
[[nodiscard]]
#endif
constexpr bool operator==(const dynamic_bitset<Block_, Allocator_>& lhs,
                                        const dynamic_bitset<Block_, Allocator_>& rhs)
{
	return (lhs.m_bits_number == rhs.m_bits_number) && (lhs.m_blocks == rhs.m_blocks);
}

template<typename Block_, typename Allocator_>
#if (__cplusplus >= 201703L)
[[nodiscard]]
#endif
constexpr bool operator<(const dynamic_bitset<Block_, Allocator_>& lhs,
                                       const dynamic_bitset<Block_, Allocator_>& rhs)
{
	using size_type = typename dynamic_bitset<Block_, Allocator_>::size_type;
	using block_type = typename dynamic_bitset<Block_, Allocator_>::block_type;
	const size_type lhs_size = lhs.size();
	const size_type rhs_size = rhs.size();
	const size_type lhs_blocks_size = lhs.m_blocks.size();
	const size_type rhs_blocks_size = rhs.m_blocks.size();

	if(lhs_size == rhs_size)
	{
		// if comparison of two empty bitsets
		if(lhs_size == 0)
		{
			return false;
		}

		for(size_type i = lhs_blocks_size - 1; i > 0; --i)
		{
			if(lhs.m_blocks[i] != rhs.m_blocks[i])
			{
				return lhs.m_blocks[i] < rhs.m_blocks[i];
			}
		}
		return lhs.m_blocks[0] < rhs.m_blocks[0];
	}

	// empty bitset inferior to 0-only bitset
	if(lhs_size == 0)
	{
		return true;
	}
	if(rhs_size == 0)
	{
		return false;
	}

	const bool rhs_longer = rhs_size > lhs_size;
	const dynamic_bitset<Block_, Allocator_>& longest_bitset = rhs_longer ? rhs : lhs;
	const size_type longest_blocks_size = std::max(lhs_blocks_size, rhs_blocks_size);
	const size_type shortest_blocks_size = std::min(lhs_blocks_size, rhs_blocks_size);
	for(size_type i = longest_blocks_size - 1; i >= shortest_blocks_size; --i)
	{
		if(longest_bitset.m_blocks[i] != block_type(0))
		{
			return rhs_longer;
		}
	}

	for(size_type i = shortest_blocks_size - 1; i > 0; --i)
	{
		if(lhs.m_blocks[i] != rhs.m_blocks[i])
		{
			return lhs.m_blocks[i] < rhs.m_blocks[i];
		}
	}
	if(lhs.m_blocks[0] != rhs.m_blocks[0])
	{
		return lhs.m_blocks[0] < rhs.m_blocks[0];
	}
	return lhs_size < rhs_size;
}

//=================================================================================================
// dynamic_bitset private functions implementations
//=================================================================================================

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::size_type dynamic_bitset<Block, Allocator>::
  blocks_required(size_type nbits) noexcept
{
	return nbits / bits_per_block + static_cast<size_type>(nbits % bits_per_block > 0);
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::size_type dynamic_bitset<Block, Allocator>::
  block_index(size_type pos) noexcept
{
	return pos / bits_per_block;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::size_type dynamic_bitset<Block, Allocator>::
  bit_index(size_type pos) noexcept
{
	return pos % bits_per_block;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::block_type dynamic_bitset<Block, Allocator>::
  bit_mask(size_type pos) noexcept
{
	return block_type(block_type(1) << bit_index(pos));
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::block_type dynamic_bitset<Block, Allocator>::
  bit_mask(size_type first, size_type last) noexcept
{
	first = bit_index(first);
	last = bit_index(last);
	if(last == (block_last_bit_index))
	{
		return block_type(one_block << first);
	}
	else
	{
		return block_type(((block_type(1) << (last + 1)) - 1) ^ ((block_type(1) << first) - 1));
	}
}

template<typename Block, typename Allocator>
constexpr void dynamic_bitset<Block, Allocator>::set_block_bits(block_type& block,
                                                                size_type first,
                                                                size_type last,
                                                                bool val) noexcept
{
	if(val)
	{
		block |= bit_mask(first, last);
	}
	else
	{
		block &= ~bit_mask(first, last);
	}
}

template<typename Block, typename Allocator>
constexpr void dynamic_bitset<Block, Allocator>::flip_block_bits(block_type& block,
                                                                 size_type first,
                                                                 size_type last) noexcept
{
	block ^= bit_mask(first, last);
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::size_type dynamic_bitset<Block, Allocator>::
  block_count(const block_type& block) noexcept
{
	if(block == zero_block)
	{
		return 0;
	}

#if defined(DYNAMIC_BITSET_GCC) \
  || (defined(DYNAMIC_BITSET_CLANG) && defined(DYNAMIC_BITSET_CLANG_builtin_popcount))
	if /*constexpr*/(std::is_same<block_type, unsigned long long>::value)
	{
		return static_cast<size_type>(__builtin_popcountll(block));
	}
	if /*constexpr*/(std::is_same<block_type, unsigned long>::value)
	{
		return static_cast<size_type>(__builtin_popcountl(block));
	}
	if /*constexpr*/(sizeof(block_type) <= sizeof(unsigned int))
	{
		return static_cast<size_type>(__builtin_popcount(static_cast<unsigned int>(block)));
	}
#endif

	size_type count = 0;
	block_type mask = 1;
	for(size_type bit_index = 0; bit_index < bits_per_block; ++bit_index)
	{
		count += ((block & mask) != zero_block);
		mask <<= 1;
	}
	return count;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::size_type dynamic_bitset<Block, Allocator>::
  block_count(const block_type& block, size_type nbits) noexcept
{
	assert(nbits <= bits_per_block);
	if(block == zero_block)
	{
		return 0;
	}

#if defined(DYNAMIC_BITSET_GCC) \
  || (defined(DYNAMIC_BITSET_CLANG) && defined(DYNAMIC_BITSET_CLANG_builtin_popcount))
	const block_type shifted_block = block_type(block << (bits_per_block - nbits));
	if /*constexpr*/(std::is_same<block_type, unsigned long long>::value)
	{
		return static_cast<size_type>(__builtin_popcountll(shifted_block));
	}
	if /*constexpr*/(std::is_same<block_type, unsigned long>::value)
	{
		return static_cast<size_type>(__builtin_popcountl(shifted_block));
	}
	if /*constexpr*/(sizeof(block_type) <= sizeof(unsigned int))
	{
		return static_cast<size_type>(__builtin_popcount(static_cast<unsigned int>(shifted_block)));
	}
#endif

	size_type count = 0;
	block_type mask = 1;
	for(size_type bit_index = 0; bit_index < nbits; ++bit_index)
	{
		count += ((block & mask) != zero_block);
		mask <<= 1;
	}

	return count;
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::size_type dynamic_bitset<Block, Allocator>::
  first_on(const block_type& block) noexcept
{
	assert(block != zero_block);

#if defined(DYNAMIC_BITSET_GCC) \
  || (defined(DYNAMIC_BITSET_CLANG) && defined(DYNAMIC_BITSET_CLANG_builtin_ctz))
	if /*constexpr*/(std::is_same<block_type, unsigned long long>::value)
	{
		return static_cast<size_type>(__builtin_ctzll(block));
	}
	if /*constexpr*/(std::is_same<block_type, unsigned long>::value)
	{
		return static_cast<size_type>(__builtin_ctzl(block));
	}
	if /*constexpr*/(sizeof(block_type) <= sizeof(unsigned int))
	{
		return static_cast<size_type>(__builtin_ctz(static_cast<unsigned int>(block)));
	}
#elif defined(DYNAMIC_BITSET_MSVC)
#	if defined(DYNAMIC_BITSET_MSVC_64)
	if /*constexpr*/(std::is_same<block_type, unsigned __int64>::value)
	{
		unsigned long index = std::numeric_limits<unsigned long>::max();
		_BitScanForward64(&index, block);
		return static_cast<size_type>(index);
	}
#	endif
	if /*constexpr*/(std::is_same<block_type, unsigned long>::value)
	{
		unsigned long index = std::numeric_limits<unsigned long>::max();
		_BitScanForward(&index, block);
		return static_cast<size_type>(index);
	}
	if /*constexpr*/(sizeof(block_type) <= sizeof(unsigned long))
	{
		unsigned long index = std::numeric_limits<unsigned long>::max();
		_BitScanForward(&index, static_cast<unsigned long>(block));
		return static_cast<size_type>(index);
	}
#endif

	block_type mask = block_type(1);
	for(size_type i = 0; i < bits_per_block; ++i)
	{
		if((block & mask) != zero_block)
		{
			return i;
		}
		mask <<= 1;
	}
	return npos;
}

#if (__cplusplus >= 201703L)
template<typename Block, typename Allocator>
template<typename _CharT, typename _Traits>
constexpr void dynamic_bitset<Block, Allocator>::init_from_string(
  std::basic_string_view<_CharT, _Traits> str,
  typename std::basic_string_view<_CharT, _Traits>::size_type pos,
  typename std::basic_string_view<_CharT, _Traits>::size_type n,
  [[maybe_unused]] _CharT zero,
  _CharT one)
{
	assert(pos < str.size());

	const size_type size = std::min(n, str.size() - pos);
	m_bits_number = size;

	m_blocks.clear();
	m_blocks.resize(blocks_required(size));
	for(size_t i = 0; i < size; ++i)
	{
		const _CharT c = str[(pos + size - 1) - i];
		assert(c == zero || c == one);
		if(c == one)
		{
			set(i);
		}
	}
}
#endif

template<typename Block, typename Allocator>
#if (__cplusplus >= 201703L)
constexpr
#endif
typename dynamic_bitset<Block, Allocator>::block_type& dynamic_bitset<Block, Allocator>::
  get_block(size_type pos)
{
	return m_blocks[block_index(pos)];
}

template<typename Block, typename Allocator>
constexpr const typename dynamic_bitset<Block, Allocator>::block_type& dynamic_bitset<
  Block,
  Allocator>::get_block(size_type pos) const
{
	return m_blocks[block_index(pos)];
}

template<typename Block, typename Allocator>
#if (__cplusplus >= 201703L)
constexpr
#endif
typename dynamic_bitset<Block, Allocator>::block_type& dynamic_bitset<Block, Allocator>::
  last_block()
{
	return m_blocks[m_blocks.size() - 1];
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::block_type dynamic_bitset<Block, Allocator>::
  last_block() const
{
	return m_blocks[m_blocks.size() - 1];
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::size_type dynamic_bitset<Block, Allocator>::
  extra_bits_number() const noexcept
{
	return bit_index(m_bits_number);
}

template<typename Block, typename Allocator>
constexpr typename dynamic_bitset<Block, Allocator>::size_type dynamic_bitset<Block, Allocator>::
  unused_bits_number() const noexcept
{
	return bits_per_block - extra_bits_number();
}

template<typename Block, typename Allocator>
template<typename BinaryOperation>
constexpr void dynamic_bitset<Block, Allocator>::apply(
  const dynamic_bitset<Block, Allocator>& other,
  BinaryOperation binary_op)
{
	assert(num_blocks() == other.num_blocks());
	std::transform(m_blocks.cbegin(),
	               m_blocks.cend(),
	               other.m_blocks.cbegin(),
	               std::begin(m_blocks),
	               binary_op);
}

template<typename Block, typename Allocator>
template<typename UnaryOperation>
constexpr void dynamic_bitset<Block, Allocator>::apply(UnaryOperation unary_op)
{
	std::transform(m_blocks.cbegin(), m_blocks.cend(), m_blocks.begin(), unary_op);
}

template<typename Block, typename Allocator>
constexpr void dynamic_bitset<Block, Allocator>::apply_left_shift(size_type shift)
{
	assert(shift > 0);
	assert(shift < capacity());

	const size_type blocks_shift = shift / bits_per_block;
	const size_type bits_offset = shift % bits_per_block;

	if(bits_offset == 0)
	{
		for(size_type i = m_blocks.size() - 1; i >= blocks_shift; --i)
		{
			m_blocks[i] = m_blocks[i - blocks_shift];
		}
	}
	else
	{
		const size_type reverse_bits_offset = bits_per_block - bits_offset;
		for(size_type i = m_blocks.size() - 1; i > blocks_shift; --i)
		{
			m_blocks[i] =
			  block_type((m_blocks[i - blocks_shift] << bits_offset)
			             | block_type(m_blocks[i - blocks_shift - 1] >> reverse_bits_offset));
		}
		m_blocks[blocks_shift] = block_type(m_blocks[0] << bits_offset);
	}

	// set bit that came at the right to 0 in unmodified blocks
	std::fill(std::begin(m_blocks),
	          std::begin(m_blocks)
	            + static_cast<typename decltype(m_blocks)::difference_type>(blocks_shift),
	          zero_block);
}

template<typename Block, typename Allocator>
constexpr void dynamic_bitset<Block, Allocator>::apply_right_shift(size_type shift)
{
	assert(shift > 0);
	assert(shift < capacity());

	const size_type blocks_shift = shift / bits_per_block;
	const size_type bits_offset = shift % bits_per_block;
	const size_type last_block_to_shift = m_blocks.size() - blocks_shift - 1;

	if(bits_offset == 0)
	{
		for(size_type i = 0; i <= last_block_to_shift; ++i)
		{
			m_blocks[i] = m_blocks[i + blocks_shift];
		}
	}
	else
	{
		const size_type reverse_bits_offset = bits_per_block - bits_offset;
		for(size_type i = 0; i < last_block_to_shift; ++i)
		{
			m_blocks[i] =
			  block_type((m_blocks[i + blocks_shift] >> bits_offset)
			             | block_type(m_blocks[i + blocks_shift + 1] << reverse_bits_offset));
		}
		m_blocks[last_block_to_shift] = block_type(m_blocks[m_blocks.size() - 1] >> bits_offset);
	}

	// set bit that came at the left to 0 in unmodified blocks
	std::fill(
	  std::begin(m_blocks)
	    + static_cast<typename decltype(m_blocks)::difference_type>(last_block_to_shift + 1),
	  std::end(m_blocks),
	  zero_block);
}

template<typename Block, typename Allocator>
#if (__cplusplus >= 201703L)
constexpr
#endif
void dynamic_bitset<Block, Allocator>::sanitize()
{
	size_type shift = m_bits_number % bits_per_block;
	if(shift > 0)
	{
		last_block() &= ~(one_block << shift);
	}
}

template<typename Block, typename Allocator>
constexpr bool dynamic_bitset<Block, Allocator>::check_unused_bits() const noexcept
{
	const size_type extra_bits = extra_bits_number();
	if(extra_bits > 0)
	{
		return (last_block() & (one_block << extra_bits)) == zero_block;
	}
	return true;
}

template<typename Block, typename Allocator>
constexpr bool dynamic_bitset<Block, Allocator>::check_size() const noexcept
{
	return blocks_required(size()) == m_blocks.size();
}

template<typename Block, typename Allocator>
constexpr bool dynamic_bitset<Block, Allocator>::check_consistency() const noexcept
{
	return check_unused_bits() && check_size();
}

//=================================================================================================
// dynamic_bitset external functions implementations
//=================================================================================================

template<typename Block, typename Allocator>
constexpr bool operator!=(const dynamic_bitset<Block, Allocator>& lhs,
                          const dynamic_bitset<Block, Allocator>& rhs)
{
	return !(lhs == rhs);
}

template<typename Block, typename Allocator>
constexpr bool operator<=(const dynamic_bitset<Block, Allocator>& lhs,
                          const dynamic_bitset<Block, Allocator>& rhs)
{
	return !(rhs < lhs);
}

template<typename Block, typename Allocator>
constexpr bool operator>(const dynamic_bitset<Block, Allocator>& lhs,
                         const dynamic_bitset<Block, Allocator>& rhs)
{
	return rhs < lhs;
}

template<typename Block, typename Allocator>
constexpr bool operator>=(const dynamic_bitset<Block, Allocator>& lhs,
                          const dynamic_bitset<Block, Allocator>& rhs)
{
	return !(lhs < rhs);
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator> operator&(const dynamic_bitset<Block, Allocator>& lhs,
                                                     const dynamic_bitset<Block, Allocator>& rhs)
{
	dynamic_bitset<Block, Allocator> result(lhs);
	return result &= rhs;
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator> operator|(const dynamic_bitset<Block, Allocator>& lhs,
                                                     const dynamic_bitset<Block, Allocator>& rhs)
{
	dynamic_bitset<Block, Allocator> result(lhs);
	return result |= rhs;
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator> operator^(const dynamic_bitset<Block, Allocator>& lhs,
                                                     const dynamic_bitset<Block, Allocator>& rhs)
{
	dynamic_bitset<Block, Allocator> result(lhs);
	return result ^= rhs;
}

template<typename Block, typename Allocator>
constexpr dynamic_bitset<Block, Allocator> operator-(const dynamic_bitset<Block, Allocator>& lhs,
                                                     const dynamic_bitset<Block, Allocator>& rhs)
{
	dynamic_bitset<Block, Allocator> result(lhs);
	return result -= rhs;
}

template<typename _CharT, typename _Traits, typename Block, typename Allocator>
constexpr std::basic_ostream<_CharT, _Traits>& operator<<(
  std::basic_ostream<_CharT, _Traits>& os,
  const dynamic_bitset<Block, Allocator>& bitset)
{
	// A better implementation is possible
	return os << bitset.template to_string<_CharT, _Traits>();
}

template<typename _CharT, typename _Traits, typename Block, typename Allocator>
constexpr std::basic_istream<_CharT, _Traits>& operator>>(std::basic_istream<_CharT, _Traits>& is,
                                                          dynamic_bitset<Block, Allocator>& bitset)
{
	// A better implementation is possible
	constexpr _CharT zero = _CharT('0');
	constexpr _CharT one = _CharT('1');
	typename std::basic_istream<_CharT, _Traits>::sentry s(is);
	if(!s)
	{
		return is;
	}

	dynamic_bitset<Block, Allocator> reverse_bitset;
	_CharT val;
	is.get(val);
	while(is.good())
	{
		if(val == one)
		{
			reverse_bitset.push_back(true);
		}
		else if(val == zero)
		{
			reverse_bitset.push_back(false);
		}
		else
		{
			is.unget();
			break;
		}
		is.get(val);
	}

	bitset.clear();
	if(!reverse_bitset.empty())
	{
		for(typename dynamic_bitset<Block, Allocator>::size_type i = reverse_bitset.size() - 1;
		    i > 0;
		    --i)
		{
			bitset.push_back(reverse_bitset.test(i));
		}
		bitset.push_back(reverse_bitset.test(0));
	}

	return is;
}

template<typename Block, typename Allocator>
constexpr void swap(dynamic_bitset<Block, Allocator>& bitset1,
                    dynamic_bitset<Block, Allocator>& bitset2)
{
	bitset1.swap(bitset2);
}

#ifndef DYNAMIC_BITSET_NO_NAMESPACE
} // namespace sul
#endif

#endif //SUL_DYNAMIC_BITSET_HPP
