/*
 *  SNPRINTF, VSNPRINT -- counted versions of printf
 *
 *	These versions have been grabbed off the net.  They have been
 *	cleaned up to compile properly and support for .precision and
 *	%lx has been added.
 */

/**************************************************************
 * Original:
 * Patrick Powell Tue Apr 11 09:48:21 PDT 1995
 * A bombproof version of doprnt (dopr) included.
 * Sigh.  This sort of thing is always nasty do deal with.  Note that
 * the version here does not include floating point...
 *
 * snprintf() is used instead of sprintf() as it does limit checks
 * for string length.  This covers a nasty loophole.
 *
 * The other functions are there to prevent NULL pointers from
 * causing nast effects.
 **************************************************************/

#include <stdarg.h>
#include <string.h>

static void fmtstr(char *, int, int, int);
static void fmtnum(long, int, int, int, int, int);
static void dostr(char *, int);
static void dopr(char *, const char *, va_list);

static char *output, *end;

/* VARARGS3 */
int
snprintf(char *str, size_t count, const char *fmt, ...)
{
    va_list ap;
    int len;

    va_start(ap, fmt);
    len = vsnprintf(str, count, fmt, ap);
    va_end(ap);
    return len;
}

int
vsnprintf(char *str, size_t count, const char *fmt, va_list args)
{
    str[0] = 0;
    end = str + count - 1;
    dopr(str, fmt, args);
    if (count > 0)
	end[0] = 0;
    return strlen(str);
}

static void
dopr_outch(int c)
{
    if (end == 0 || output < end)
	*output++ = c;
}

/*
 * dopr(): poor man's version of doprintf
 */

static void
dopr(char *buffer, const char *format, va_list args)
{
    int ch;
    long value;
    int longflag  = 0;
    int pointflag = 0;
    int maxwidth  = 0;
    char *strvalue;
    int ljust;
    int len;
    int zpad;

    output = buffer;
    while ((ch = *format++) != '\0') {
	switch (ch) {
	    case '%':
		ljust = len = zpad = maxwidth = 0;
		longflag = pointflag = 0;
nextch:
		ch = *format++;
		switch (ch) {
		    case 0:
			dostr("**end of format**" , 0);
			return;
		    case '-':
			ljust = 1;
			goto nextch;
		    case '0':	/* set zero padding if len not set */
			if (len == 0 && !pointflag)
			    zpad = '0';
			/* FALLTHROUGH */
		    case '1': case '2': case '3':
		    case '4': case '5': case '6':
		    case '7': case '8': case '9':
			if (pointflag)
			    maxwidth = maxwidth*10 + ch - '0';
			else
			    len = len*10 + ch - '0';
			goto nextch;
		    case '*': 
			if (pointflag)
			    maxwidth = va_arg(args, int);
			else
			    len = va_arg(args, int);
			goto nextch;
		    case '.':
			pointflag = 1;
			goto nextch;
		    case 'l':
			longflag = 1;
			goto nextch;
		    case 'u': case 'U':
			/*fmtnum(value,base,dosign,ljust,len,zpad) */
			if (longflag)
			    value = va_arg(args, long);
			else
			    value = va_arg(args, int);
			fmtnum(value, 10,0, ljust, len, zpad);
			break;
		    case 'o': case 'O':
			if (longflag)
			    value = va_arg(args, long);
			else
			    value = va_arg(args, int);
			fmtnum(value, 8,0, ljust, len, zpad);
			break;
		    case 'd': case 'D':
			if (longflag)
			    value = va_arg(args, long);
			else
			    value = va_arg(args, int);
			fmtnum(value, 10,1, ljust, len, zpad);
			break;
		    case 'x':
			if (longflag)
			    value = va_arg(args, long);
			else
			    value = va_arg(args, int);
			fmtnum(value, 16,0, ljust, len, zpad);
			break;
		    case 'X':
			if (longflag)
			    value = va_arg(args, long);
			else
			    value = va_arg(args, int);
			fmtnum(value,-16,0, ljust, len, zpad);
			break;
		    case 's':
			strvalue = va_arg(args, char *);
			if (maxwidth > 0 || !pointflag) {
			    if (pointflag && len > maxwidth)
				len = maxwidth; /* Adjust padding */
			    fmtstr(strvalue, ljust, len, maxwidth);
			}
			break;
		    case 'c':
			ch = va_arg(args, int);
			dopr_outch(ch);
			break;
		    case '%':
			dopr_outch(ch);
			continue;
		    default:
			dostr("???????" , 0);
		}
		break;
	    default:
		dopr_outch(ch);
		break;
	}
    }
    *output = 0;
}

static void
fmtstr(char *value, int ljust, int len, int maxwidth)
{
    int padlen, length;		/* amount to pad */

    if (value == 0)
	value = "<NULL>";

    length = strlen(value);
    if (length > maxwidth && maxwidth)
	length = maxwidth;
    padlen = len - length;
    if (padlen < 0)
	padlen = 0;
    if (ljust)
	padlen = -padlen;
    while (padlen > 0) {
	dopr_outch(' ');
	--padlen;
    }
    dostr(value, maxwidth);
    while (padlen < 0) {
	dopr_outch(' ');
	++padlen;
    }
}

static void
fmtnum(long value, int base, int dosign, int ljust, int len, int zpad)
{
    int signvalue = 0;
    unsigned long uvalue;
    char convert[20];
    int place = 0;
    int padlen = 0;	/* amount to pad */
    int caps = 0;

    uvalue = value;
    if (dosign) {
	if (value < 0) {
	    signvalue = '-';
	    uvalue = -value;
	}
    }
    if (base < 0) {
	caps = 1;
	base = -base;
    }
    do {
	convert[place++] = (caps ? "0123456789ABCDEF" : "0123456789abcdef")
			   [(int)(uvalue % (unsigned)base)];
	uvalue = (uvalue / (unsigned)base);
    } while(uvalue);
    convert[place] = 0;
    padlen = len - place;
    if (padlen < 0)
	padlen = 0;
    if (ljust)
	padlen = -padlen;
    if (zpad && padlen > 0) {
	if (signvalue) {
	    dopr_outch(signvalue);
	    --padlen;
	    signvalue = 0;
	}
	while (padlen > 0) {
	    dopr_outch(zpad);
	    --padlen;
	}
    }
    while (padlen > 0) {
	dopr_outch(' ');
	--padlen;
    }
    if (signvalue)
	dopr_outch(signvalue);
    while (place > 0)
	dopr_outch(convert[--place]);
    while (padlen < 0) {
	dopr_outch(' ');
	++padlen;
    }
}

static void
dostr(char *str, int cut)
{
    if (cut)
	while (*str && cut-- > 0)
	    dopr_outch(*str++);
    else
	while (*str)
	    dopr_outch(*str++);
}
