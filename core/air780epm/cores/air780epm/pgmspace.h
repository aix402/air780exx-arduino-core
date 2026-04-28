#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef void prog_void;
typedef char prog_char;
typedef unsigned char prog_uchar;
typedef char prog_int8_t;
typedef unsigned char prog_uint8_t;
typedef short prog_int16_t;
typedef unsigned short prog_uint16_t;
typedef long prog_int32_t;
typedef unsigned long prog_uint32_t;

#ifndef PROGMEM
#define PROGMEM
#endif

#ifndef PGM_P
#define PGM_P const char *
#endif

#ifndef PGM_VOID_P
#define PGM_VOID_P const void *
#endif

#ifndef PSTR
#define PSTR(str) (str)
#endif

#ifndef pgm_read_byte
#define pgm_read_byte(address_short) (*(const uint8_t *)(address_short))
#endif

#ifndef pgm_read_word
#define pgm_read_word(address_short) (*(const uint16_t *)(address_short))
#endif

#ifndef pgm_read_dword
#define pgm_read_dword(address_short) (*(const uint32_t *)(address_short))
#endif

#ifndef pgm_read_float
#define pgm_read_float(address_short) (*(const float *)(address_short))
#endif

#ifndef pgm_read_ptr
#define pgm_read_ptr(address_short) (*(const void *const *)(address_short))
#endif

#ifndef pgm_get_far_address
#define pgm_get_far_address(value) ((uint32_t)(&(value)))
#endif

#define pgm_read_byte_near(address_short) pgm_read_byte(address_short)
#define pgm_read_word_near(address_short) pgm_read_word(address_short)
#define pgm_read_dword_near(address_short) pgm_read_dword(address_short)
#define pgm_read_float_near(address_short) pgm_read_float(address_short)
#define pgm_read_ptr_near(address_short) pgm_read_ptr(address_short)
#define pgm_read_byte_far(address_short) pgm_read_byte(address_short)
#define pgm_read_word_far(address_short) pgm_read_word(address_short)
#define pgm_read_dword_far(address_short) pgm_read_dword(address_short)
#define pgm_read_float_far(address_short) pgm_read_float(address_short)
#define pgm_read_ptr_far(address_short) pgm_read_ptr(address_short)

#define memcmp_P memcmp
#define memcpy_P memcpy
#define memmove_P memmove
#define strcpy_P strcpy
#define strncpy_P strncpy
#define strcat_P strcat
#define strncat_P strncat
#define strcmp_P strcmp
#define strncmp_P strncmp
#define strlen_P strlen
#define strstr_P strstr
#define printf_P printf
#define sprintf_P sprintf
#define snprintf_P snprintf
#define vsnprintf_P vsnprintf
