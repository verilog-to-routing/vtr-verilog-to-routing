#ifndef OBFUSCATE_H
#define OBFUSCATE_H

#ifdef _MSC_VER
	#define AY_CAT(X,Y) AY_CAT2(X,Y)
	#define AY_CAT2(X,Y) X##Y
	#define AY_LINE int(AY_CAT(__LINE__,U))
#else
	#define AY_LINE __LINE__
#endif

#ifndef AY_OBFUSCATE_DEFAULT_KEY
	// The default 64 bit key to obfuscate strings with.
	// This can be user specified by defining AY_OBFUSCATE_DEFAULT_KEY before 
	// including obfuscate.h
	#define AY_OBFUSCATE_DEFAULT_KEY ay::generate_key(AY_LINE)
#endif

namespace ay
{
	using size_type = unsigned long long;
	using key_type = unsigned long long;

	// Generate a pseudo-random key that spans all 8 bytes
	constexpr key_type generate_key(key_type seed)
	{
		// Use the MurmurHash3 64-bit finalizer to hash our seed
		key_type key = seed;
		key ^= (key >> 33);
		key *= 0xff51afd7ed558ccd;
		key ^= (key >> 33);
		key *= 0xc4ceb9fe1a85ec53;
		key ^= (key >> 33);

		// Make sure that a bit in each byte is set
		key |= 0x0101010101010101ull;

		return key;
	}

	// Obfuscates or deobfuscates data with key
	constexpr void cipher(char* data, size_type size, key_type key)
	{
		// Obfuscate with a simple XOR cipher based on key
		for (size_type i = 0; i < size; i++)
		{
			data[i] ^= char(key >> ((i % 8) * 8));
		}
	}

	// Obfuscates a string at compile time
	template <size_type N, key_type KEY>
	class obfuscator
	{
	public:
		// Obfuscates the string 'data' on construction
		constexpr obfuscator(const char* data)
		{
			// Copy data
			for (size_type i = 0; i < N; i++)
			{
				m_data[i] = data[i];
			}

			// On construction each of the characters in the string is
			// obfuscated with an XOR cipher based on key
			cipher(m_data, N, KEY);
		}

		constexpr const char* data() const
		{
			return &m_data[0];
		}

		constexpr size_type size() const
		{
			return N;
		}

		constexpr key_type key() const
		{
			return KEY;
		}

	private:

		char m_data[N]{};
	};

	// Handles decryption and re-encryption of an encrypted string at runtime
	template <size_type N, key_type KEY>
	class obfuscated_data
	{
	public:
		obfuscated_data(const obfuscator<N, KEY>& obfuscator)
		{
			// Copy obfuscated data
			for (size_type i = 0; i < N; i++)
			{
				m_data[i] = obfuscator.data()[i];
			}
		}

		~obfuscated_data()
		{
			// Zero m_data to remove it from memory
			for (size_type i = 0; i < N; i++)
			{
				m_data[i] = 0;
			}
		}

		// Returns a pointer to the plain text string, decrypting it if
		// necessary
		operator char*()
		{
			decrypt();
			return m_data;
		}

		// Manually decrypt the string
		void decrypt()
		{
			if (m_encrypted)
			{
				cipher(m_data, N, KEY);
				m_encrypted = false;
			}
		}

		// Manually re-encrypt the string
		void encrypt()
		{
			if (!m_encrypted)
			{
				cipher(m_data, N, KEY);
				m_encrypted = true;
			}
		}

		// Returns true if this string is currently encrypted, false otherwise.
		bool is_encrypted() const
		{
			return m_encrypted;
		}

	private:

		// Local storage for the string. Call is_encrypted() to check whether or
		// not the string is currently obfuscated.
		char m_data[N];

		// Whether data is currently encrypted
		bool m_encrypted{ true };
	};

	// This function exists purely to extract the number of elements 'N' in the
	// array 'data'
	template <size_type N, key_type KEY = AY_OBFUSCATE_DEFAULT_KEY>
	constexpr auto make_obfuscator(const char(&data)[N])
	{
		return obfuscator<N, KEY>(data);
	}
}

// Obfuscates the string 'data' at compile-time and returns a reference to a
// ay::obfuscated_data object with global lifetime that has functions for
// decrypting the string and is also implicitly convertable to a char*
#define AY_OBFUSCATE(data) AY_OBFUSCATE_KEY(data, AY_OBFUSCATE_DEFAULT_KEY)

// Obfuscates the string 'data' with 'key' at compile-time and returns a
// reference to a ay::obfuscated_data object with global lifetime that has
// functions for decrypting the string and is also implicitly convertable to a
// char*
#define AY_OBFUSCATE_KEY(data, key) \
	[]() -> ay::obfuscated_data<sizeof(data)/sizeof(data[0]), key>& { \
		static_assert(sizeof(decltype(key)) == sizeof(ay::key_type), "key must be a 64 bit unsigned integer"); \
		static_assert((key) >= (1ull << 56), " must span all 8 bytes"); \
		constexpr auto n = sizeof(data)/sizeof(data[0]); \
		constexpr auto obfuscator = ay::make_obfuscator<n, key>(data); \
		static auto obfuscated_data = ay::obfuscated_data<n, key>(obfuscator); \
		return obfuscated_data; \
	}()

#endif
