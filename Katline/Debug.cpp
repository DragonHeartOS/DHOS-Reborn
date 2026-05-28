#include <CommonLib/String.h>
#include <Katline/Debug.h>
#include <Katline/Katline.h>

#include <stdarg.h>

namespace Katline {

namespace Debug {

static bool s_framebuffer_logging_enabled = false;

auto set_framebuffer_logging_enabled(bool enabled) -> void
{
	s_framebuffer_logging_enabled = enabled;
}

auto write_formatted(char const *str, ...) -> void
{
	va_list vl;
	int i = 0;
	int j = 0;

	char buffer[100] = { 0 };
	unsigned char temp[20];

	va_start(vl, str);

	while (str && str[i]) {
		if (str[i] == '%') {
			i++;

			int precision = -1;

			if (str[i] == '.') {
				i++;

				// %.*s
				if (str[i] == '*') {
					precision = va_arg(vl, int);
					i++;
				} else {
					// %.5s
					precision = 0;

					while (str[i] >= '0' && str[i] <= '9') {
						precision = precision * 10 + (str[i] - '0');
						i++;
					}
				}
			}

			switch (str[i]) {

			case 'c': {
				buffer[j++] = (char)va_arg(vl, int);
				break;
			}

			case 'd': {
				CL::itoa(va_arg(vl, int), (char *)temp);

				CL::strcpy(&buffer[j], (char const *)temp);
				j += CL::strlen((char const *)temp);

				break;
			}

			case 's': {
				char const *s = va_arg(vl, char const *);

				if (!s)
					s = "(null)";

				size_t len = CL::strlen(s);

				if (precision >= 0 && (size_t)precision < len)
					len = static_cast<size_t>(precision);

				for (size_t k = 0; k < len; k++)
					buffer[j++] = s[k];

				break;
			}

			case '%': {
				buffer[j++] = '%';
				break;
			}

			default: {
				buffer[j++] = '%';
				buffer[j++] = str[i];
				break;
			}
			}

		} else {
			buffer[j++] = str[i];
		}

		i++;
	}

	buffer[j] = '\0';

	va_end(vl);

	k_serial_controller.write_string(buffer);

	if (s_framebuffer_logging_enabled)
		k_framebuffer_controller.put_string(buffer);
}

}

}
