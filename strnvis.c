/*-
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: strnvis.c,v 1.2 2004/02/20 20:43:03 akorty Exp $
 */

#include <sys/types.h>

#include <ctype.h>
#include <limits.h>
#include <string.h>

#include "strnvis.h"

#define	isoctal(c)	(((u_char)(c)) >= '0' && ((u_char)(c)) <= '7')
#define isvisible(c)	(((u_int)(c) <= UCHAR_MAX && isascii((u_char)(c)) && \
				isgraph((u_char)(c))) ||		     \
				((flag & VIS_SP) == 0 && (c) == ' ') ||	     \
				((flag & VIS_TAB) == 0 && (c) == '\t') ||    \
				((flag & VIS_NL) == 0 && (c) == '\n') ||     \
				((flag & VIS_SAFE) && ((c) == '\b' ||	     \
				(c) == '\007' || (c) == '\r' ||		     \
				isgraph((u_char)(c)))))

/*
 * vis - visually encode characters
 */
char *
vis(dst, c, flag, nextc)
	register char *dst;
	int c, nextc;
	register int flag;
{
	if (isvisible(c)) {
		*dst++ = c;
		if (c == '\\' && (flag & VIS_NOSLASH) == 0)
			*dst++ = '\\';
		*dst = '\0';
		return (dst);
	}

	if (flag & VIS_CSTYLE) {
		switch(c) {
		case '\n':
			*dst++ = '\\';
			*dst++ = 'n';
			goto done;
		case '\r':
			*dst++ = '\\';
			*dst++ = 'r';
			goto done;
		case '\b':
			*dst++ = '\\';
			*dst++ = 'b';
			goto done;
		case '\a':
			*dst++ = '\\';
			*dst++ = 'a';
			goto done;
		case '\v':
			*dst++ = '\\';
			*dst++ = 'v';
			goto done;
		case '\t':
			*dst++ = '\\';
			*dst++ = 't';
			goto done;
		case '\f':
			*dst++ = '\\';
			*dst++ = 'f';
			goto done;
		case ' ':
			*dst++ = '\\';
			*dst++ = 's';
			goto done;
		case '\0':
			*dst++ = '\\';
			*dst++ = '0';
			if (isoctal(nextc)) {
				*dst++ = '0';
				*dst++ = '0';
			}
			goto done;
		}
	}
	if (((c & 0177) == ' ') || (flag & VIS_OCTAL)) {	
		*dst++ = '\\';
		*dst++ = ((u_char)c >> 6 & 07) + '0';
		*dst++ = ((u_char)c >> 3 & 07) + '0';
		*dst++ = ((u_char)c & 07) + '0';
		goto done;
	}
	if ((flag & VIS_NOSLASH) == 0)
		*dst++ = '\\';
	if (c & 0200) {
		c &= 0177;
		*dst++ = 'M';
	}
	if (iscntrl(c)) {
		*dst++ = '^';
		if (c == 0177)
			*dst++ = '?';
		else
			*dst++ = c + '@';
	} else {
		*dst++ = '-';
		*dst++ = c;
	}
done:
	*dst = '\0';
	return (dst);
}

int
strnvis(dst, src, siz, flag)
	char *dst;
	const char *src;
	size_t siz;
	int flag;
{
	char c;
	char *start, *end;
	char tbuf[5];
	int  i;

	i = 0;
	for (start = dst, end = start + siz - 1; (c = *src) && dst < end; ) {
		if (isvisible(c)) {
			i = 1;
			*dst++ = c;
			if (c == '\\' && (flag & VIS_NOSLASH) == 0) {
				/* need space for the extra '\\' */
				if (dst < end)
					*dst++ = '\\';
				else {
					dst--;
					i = 2;
					break;
				}
			}
			src++;
		} else {
			i = vis(tbuf, c, flag, *++src) - tbuf;
			if (dst + i <= end) {
				memcpy(dst, tbuf, i);
				dst += i;
			} else {
				src--;
				break;
			}
		}
	}
	if (siz > 0)
		*dst = '\0';
	if (dst + i > end) {
		/* adjust return value for truncation */
		while ((c = *src))
			dst += vis(tbuf, c, flag, *++src) - tbuf;
	}
	return (dst - start);
}
