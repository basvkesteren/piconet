/*
    Piconet RS232 ethernet interface

    std.c
           
    Copyright (c) 2006,2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
Some all-around functions
*/
#include "std.h"

int abs(int j)
/*!
  Returns absolute value of given signed integer
*/
{
	return (j < 0) ? -j : j;
}

int fpow(const int ground, const int n)
/*!
  Returns ground^n (fixed point)
 */
{
    int i;
    int p=1;

    for(i=1; i<=n; ++i) {
        p=p*ground;
    }

    return p;
}

unsigned char ctoi(const unsigned char c, const unsigned char base)
/*!
 *  Convert the given ASCII character to the corresponding number, using the given base.
 *  Note that this function does not check wheter 'c' actually is a digit!
 */
{
    if (c >= 'a' && base > 10) {
        return c-'a'+10;
    }

    if (c >= 'A' && base > 10) {
        return c-'A'+10;
    }

    return c-'0';
}

bool isnumber(const unsigned char c, const unsigned char base)
/*!
 *  Determine wheter the given char is a digit according to the given base.
 *  We assume ASCII encoding here, so '0' < 'A' < 'a'.
 */
{
    if (c < '0') {
        /* This is not a digit, no matter what base.. */
        return FALSE;
    }
    if (base > 10) {
        if ((c >= 'a' && c <= 'a' + base-11) || (c >= 'A' && c <= 'A' + base-11)) {
            return TRUE;
        }
    }

    if (c >= '0' && c <= '0' + base-1) {
        return TRUE;
    }

    return FALSE;
}

unsigned char inttostrn(unsigned int num, char *c, unsigned char limit, const unsigned char base)
/*!
 *  Coverts the given number 'num' whit base 'base' to a string. The result is stored in 'c',
 *  the length of the result is returned.
 *
 *  Use 'limit' to set the maximum length of the output string, which cannot be more than STD_MAXNUMLENGTH. 0 equals STD_MAXNUMLENGTH in this case.
 */
{
    const unsigned char chars[] = "0123456789ABCDEF";
    unsigned char i=0;
    unsigned char j=0;
    unsigned int rest;

    /* Convert it to a string */
    if(num==0) {
        if(limit == 1) {
            c[0]='\0';
            return 0;
        }
        else {
            c[0]='0';
            c[1]='\0';
            return 1;
        }
    }
    else {
        while(num && i<32) {
            if(i == limit-1) {
                break;
            }
            rest=num%base;
            num/=base;
            c[i]=chars[rest];
            i++;
        }
        /* Terminate string */
        c[i]='\0';
        i--; /* Don't include the '\0' in the counter, that should not be reversed... */
    }

    /* The string is now reversed (that is, everything before the '\0'). Let's fix that */
    while(j<i) {
        rest=c[j];
        c[j]=c[i];
        c[i]=rest;
        i--;
        j++;
    }

    return j+i+1;
}

unsigned int strtoint(const char *c, const unsigned char base)
/*!
 *  Convert the given string to a number, stopping when any invalid charachter is found in the string.
 *  Spaces at the beginning of the string are ignored
 */
{
    unsigned int result = 0;

    /* Skip spaces at beginning of string */
    while(*c && *c == ' ') {
        c++;
    }

    while(*c) {
        if(isnumber(*c,base)) {
            result *= base;
            result += ctoi(*c,base);
        }
        else {
            /* Abort on anything invalid */
            break;
        }
        c++;
    }
    return result;
}