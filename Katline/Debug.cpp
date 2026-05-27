#include <Katline/Debug.h>
#include <Katline/Katline.h>

#include <stdarg.h>

// TODO: Move all of those into separate library
size_t
strlen(char const* str)
{
    char const* s;

    for (s = str; *s; ++s)
        ;
    return (size_t)(s - str);
}

char* strcpy(char* destination, char const* source)
{
    // return if no memory is allocated to the destination
    if (destination == NULL) {
        return NULL;
    }

    // take a pointer pointing to the beginning of the destination string
    char* ptr = destination;

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

void revstr(char* str1)
{
    int i, len, temp;
    len = (int)strlen(str1);
    for (i = 0; i < len / 2; i++) {
        temp = str1[i];
        str1[i] = str1[len - i - 1];
        str1[len - i - 1] = (char)temp;
    }
}

void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0) /* record sign */
        n = -n;         /* make n positive */
    i = 0;
    do {                       /* generate digits in reverse order */
        s[i++] = n % 10 + '0'; /* get next digit */
    } while ((n /= 10) > 0); /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    revstr(s);
}

namespace Katline {

namespace Debug {

static bool s_framebuffer_logging_enabled = false;

void set_framebuffer_logging_enabled(bool enabled)
{
    s_framebuffer_logging_enabled = enabled;
}

void write_formatted(char const* str, ...)
{
    va_list vl;
    int i = 0;
    int j = 0;
    char buffer[100] = { 0 };

    unsigned char temp[20];

    va_start(vl, str[i]);
    while (str && str[i]) {
        if (str[i] == '%') {
            i++;
            switch (str[i]) {
            case 'c': {
                buffer[j] = (char)va_arg(vl, int);
                j++;
                break;
            }
            case 'd': {
                itoa(va_arg(vl, int), (char*)temp);
                strcpy(&buffer[j], (char const*)temp);
                j += strlen((char const*)temp);
                break;
            }
            }
        } else {
            buffer[j] = str[i];
            j++;
        }
        i++;
    }

    k_serial_controller.write_string(buffer);

    if (s_framebuffer_logging_enabled)
        k_framebuffer_controller.put_string(buffer);
}

}

}
