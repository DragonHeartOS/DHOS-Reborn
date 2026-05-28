#include <CommonLib/cstring>

#include <CommonLib/Types.h>

auto strlen(char const *str) -> usize
{
	char const *s;
	for (s = str; *s; ++s)
		;
	return static_cast<usize>(s - str);
}

auto strcpy(char *destination, char const *source) -> char *
{
	// return if no memory is allocated to the destination
	if (destination == nullptr) {
		return nullptr;
	}

	// take a pointer pointing to the beginning of the destination string
	char *ptr = destination;

	// copy the C-string pointed by source into the array
	// pointed by destination
	while (*source != '\0') {
		*destination = *source;
		destination++;
		source++;
	}

	// include the terminating null character
	*destination = '\0';

	// the destination is returned by standard `strcpy()`
	return ptr;
}

auto revstr(char *str1) -> void
{
	int i, len, temp;
	len = (int)strlen(str1);
	for (i = 0; i < len / 2; i++) {
		temp = str1[i];
		str1[i] = str1[len - i - 1];
		str1[len - i - 1] = (char)temp;
	}
}

auto itoa(int n, char s[]) -> void
{
	int i, sign;

	if ((sign = n) < 0) // record sign
		n = -n;         // make n positive
	i = 0;
	do {                       // generate digits in reverse order
		s[i++] = n % 10 + '0'; // get next digit
	} while ((n /= 10) > 0); // delete it
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	revstr(s);
}

auto memcpy(void *dst, void *src, usize size) -> void
{
	auto *dst_ { reinterpret_cast<u8 *>(dst) };
	auto *src_ { reinterpret_cast<u8 *>(src) };
	for (usize i {}; i < size; i++)
		dst_[i] = src_[i];
}
