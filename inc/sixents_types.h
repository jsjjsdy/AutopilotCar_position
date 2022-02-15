/**
 * @copyright Copyright (c) 2020 - 2020 Beijing Sixents Technology Co., Ltd.
 *       All rights reserved.
 * @file    sixents_types.h
 * @author  sixents@sixents.com
 * @version 1.0
 * @date    2020-5-30
 * @brief   定义基本数据类型
 * @details
 *
 * @note
 *    change history:
 *    <2020-5-30>  | 1.0 | sixents@sixents.com | Create initial version
 */

#ifndef _SIXENTS_TYPES_H_
#define _SIXENTS_TYPES_H_

#include <stddef.h> /* NULL */

/**
 * basic types definition
 */

/** character */
typedef char sixents_char;

/** 8bits signed integer */
typedef signed char sixents_int8;

/** 8bits unsigned integer */
typedef unsigned char sixents_uint8;

/** 16bits signed integer */
typedef signed short sixents_int16;

/** 16bits unsigned integer */
typedef unsigned short sixents_uint16;

/** 32bits signed integer */
typedef signed int sixents_int32;

/** 32bits unsigned integer */
typedef unsigned int sixents_uint32;

/** 64bits signed integer */
typedef signed long long sixents_int64;

/** 64bits unsigned integer */
typedef unsigned long long sixents_uint64;

/** 32bits precision float number */
typedef float sixents_float32;

/** 64bits precision float number */
typedef double sixents_float64;

/** NULL */
#define sixents_null ((void *)0)

/** boolean representation */
typedef enum
{
    /* FALSE value */
    SIXENTS_FALSE,
    /* TRUE value */
    SIXENTS_TRUE
} sixents_bool;

#endif // !_SIXENTS_TYPES_H_