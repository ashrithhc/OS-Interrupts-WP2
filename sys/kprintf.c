#include <sys/kprintf.h>
#include "stdarg.h"

void kprintf(const char *fmt, ...)
{
	const char *temp1; register char *temp2;

	va_list valist;
	va_start(valist, fmt);

	for(temp1 = fmt, temp2 = (char *)0xb8000; *temp1; temp1+=1, temp2+=2) {
		if(*temp1 != '%') *temp2 = *temp1;
		else {
			temp1 += 1;
			if(*temp1 == 'c') *temp2 = va_arg(valist, int);
			else *temp2 = *temp1;
		}
	}

	va_end(valist);
}
