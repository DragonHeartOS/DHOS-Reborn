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
	usize i = 0;
	usize j = 0;

	char buffer[100] = { 0 };
	char temp[20];

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
				itoa(va_arg(vl, int), temp);

				strcpy(&buffer[j], temp);
				j += strlen(temp);

				break;
			}

			case 's': {
				char const *s = va_arg(vl, char const *);

				if (!s)
					s = "(null)";

				usize len = strlen(s);

				if (precision >= 0 && static_cast<usize>(precision) < len)
					len = static_cast<usize>(precision);

				for (usize k = 0; k < len; k++)
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
