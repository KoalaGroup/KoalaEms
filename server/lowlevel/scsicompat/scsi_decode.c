static const char* cvsid __attribute__((unused))=
    "$ZEL: scsi_decode.c,v 1.4 2011/04/06 20:30:27 wuestner Exp $";

#include "scsicompat.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <ctype.h>
#include <rcs_ids.h>

RCS_REGISTER(cvsid, "lowlevel/scsicompat")

/*
 * Decode: Decode the data section of a scsireq.  This decodes
 * trivial grammar:
 *
 * fields : field fields
 *        ;
 *
 * field : field_specifier
 *       | control
 *       ;
 *
 * control : 's' seek_value
 *       | 's' '+' seek_value
 *       ;
 *
 * seek_value : DECIMAL_NUMBER
 *       | 'v'				// For indirect seek, i.e., value from the arg list
 *       ;
 *
 * field_specifier : type_specifier field_width
 *       | '{' NAME '}' type_specifier field_width
 *       ;
 *
 * field_width : DECIMAL_NUMBER
 *       ;
 *
 * type_specifier : 'i'	// Integral types (i1, i2, i3, i4)
 *       | 'b'				// Bits
 *       | 't'				// Bits
 *       | 'c'				// Character arrays
 *       | 'z'				// Character arrays with zeroed trailing spaces
 *       ;
 *
 * Notes:
 * 1. Integral types are swapped into host order.
 * 2. Bit fields are allocated MSB to LSB to match the SCSI spec documentation.
 * 3. 's' permits "seeking" in the string.  "s+DECIMAL" seeks relative to
 *    DECIMAL; "sDECIMAL" seeks absolute to decimal.
 * 4. 's' permits an indirect reference.  "sv" or "s+v" will get the
 *    next integer value from the arg array.
 * 5. Field names can be anything between the braces
 *
 * BUGS:
 * i and b types are promoted to ints.
 *
 */

static int do_buff_decode(u_char *databuf, size_t len,
void (*arg_put)(void *, int , void *, int, char *), void *puthook,
char *fmt, va_list ap)
{
	int assigned = 0;
	int width;
	int suppress;
	int plus;
	int done = 0;
	static u_char mask[] = {0, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};
	int value;
	u_char *base = databuf;
	char letter;
	char field_name[80];

#	define ARG_PUT(ARG) \
	do \
	{ \
		if (!suppress) \
		{ \
			if (arg_put) \
				(*arg_put)(puthook, (letter == 't' ? 'b' : letter), \
				(void *)((long)(ARG)), 1, field_name); \
			else \
				*(va_arg(ap, int *)) = (ARG); \
			assigned++; \
		} \
		field_name[0] = 0; \
		suppress = 0; \
	} while (0)

	u_char bits = 0;	/* For bit fields */
	int shift = 0;		/* Bits already shifted out */
	suppress = 0;
	field_name[0] = 0;

	while (!done)
	{
		switch(letter = *fmt)
		{
			case ' ':	/* White space */
			case '\t':
			case '\r':
			case '\n':
			case '\f':
			fmt++;
			break;

			case '#':	/* Comment */
			while (*fmt && (*fmt != '\n'))
				fmt++;
			if (fmt)
				fmt++;	/* Skip '\n' */
			break;

			case '*':	/* Suppress assignment */
			fmt++;
			suppress = 1;
			break;

			case '{':	/* Field Name */
			{
				int i = 0;
				fmt++;	/* Skip '{' */
				while (*fmt && (*fmt != '}'))
				{
					if (i < sizeof(field_name))
						field_name[i++] = *fmt;

					fmt++;
				}
				if (fmt)
					fmt++;	/* Skip '}' */
				field_name[i] = 0;
			}
			break;

			case 't':	/* Bit (field) */
			case 'b':	/* Bits */
			fmt++;
			width = strtol(fmt, &fmt, 10);
			if (width > 8)
				done = 1;
			else
			{
				if (shift <= 0)
				{
					bits = *databuf++;
					shift = 8;
				}
				value = (bits >> (shift - width)) & mask[width];

#if 0
				printf("shift %2d bits %02x value %02x width %2d mask %02x\n",
				shift, bits, value, width, mask[width]);
#endif

				ARG_PUT(value);

				shift -= width;
			}

			break;

			case 'i':	/* Integral values */
			shift = 0;
			fmt++;
			width = strtol(fmt, &fmt, 10);
			switch(width)
			{
				case 1:
				ARG_PUT(*databuf);
				databuf++;
				break;

				case 2:
				ARG_PUT(
				(*databuf) << 8 |
				*(databuf + 1));
				databuf += 2;
				break;

				case 3:
				ARG_PUT(
				(*databuf) << 16 |
				(*(databuf + 1)) << 8 |
				*(databuf + 2));
				databuf += 3;
				break;

				case 4:
				ARG_PUT(
				(*databuf) << 24 |
				(*(databuf + 1)) << 16 |
				(*(databuf + 2)) << 8 |
				*(databuf + 3));
				databuf += 4;
				break;

				default:
				done = 1;
			}

			break;

			case 'c':	/* Characters (i.e., not swapped) */
			case 'z':	/* Characters with zeroed trailing spaces  */
			shift = 0;
			fmt++;
			width = strtol(fmt, &fmt, 10);
			if (!suppress)
			{
				if (arg_put)
					(*arg_put)(puthook, (letter == 't' ? 'b' : letter),
					databuf, width, field_name);
				else
				{
					char *dest;
					dest = va_arg(ap, char *);
					bcopy((void*)databuf, dest, width);
					if (letter == 'z')
					{
						char *p;
						for (p = dest + width - 1;
						(p >= (char *)dest) && (*p == ' '); p--)
							*p = 0;
					}
				}
				assigned++;
			}
			databuf += width;
			field_name[0] = 0;
			suppress = 0;
			break;

			case 's':	/* Seek */
			shift = 0;
			fmt++;
			if (*fmt == '+')
			{
				plus = 1;
				fmt++;
			}
			else
				plus = 0;

			if (tolower(*fmt) == 'v')
			{
				/* You can't suppress a seek value.  You also
				 * can't have a variable seek when you are using
				 * "arg_put".
				 */
				width = (arg_put) ? 0 : va_arg(ap, int);
				fmt++;
			}
			else
				width = strtol(fmt, &fmt, 10);

			if (plus)
				databuf += width;	/* Relative seek */
			else
				databuf = base + width;	/* Absolute seek */

			break;

			case 0:
			done = 1;
			break;

			default:
			printf("Unknown letter in format: %c\n", letter);
			fmt++;
		}
	}

	return assigned;
}

int scsireq_buff_decode(u_char *buff, size_t len, char *fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	return do_buff_decode(buff, len, 0, 0, fmt, ap);
}

