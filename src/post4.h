/*
 * post4.h
 *
 * Copyright 2007, 2022 by Anthony Howe. All rights reserved.
 */

#ifndef __post4_h__
#define __post4_h__	1

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 *** Configurables
 ***********************************************************************/

#include "config.h"

#ifndef P4_BLOCK_SIZE
#define P4_BLOCK_SIZE			1024		/* in bytes */
#endif

#ifndef P4_NAME_SIZE
#define P4_NAME_SIZE			32		/* in bytes */
#endif

#ifndef P4_STACK_SIZE
#define P4_STACK_SIZE			64		/* in CELLS */
#endif

#ifndef P4_FLOAT_STACK_SIZE
#define P4_FLOAT_STACK_SIZE		6		/* in CELLS */
#endif

#ifndef P4_STRING_SIZE
#define P4_STRING_SIZE			256		/* in bytes */
#endif

#ifndef P4_INPUT_SIZE
#define P4_INPUT_SIZE			256		/* in bytes */
#endif

#ifndef P4_SAFE_PATH
#define P4_SAFE_PATH			"/usr/bin:/bin"
#endif

#ifndef P4_BLOCK_FILE
#define P4_BLOCK_FILE			".post4.blk"
#endif

#ifndef P4_CORE_PATH
#define P4_CORE_PATH			".:/usr/pkg/lib/post4:/usr/local/lib/post4:/usr/lib/post4"
#endif

#ifndef P4_CORE_FILE
#define P4_CORE_FILE			"post4.p4"
#endif

#ifndef P4_PAD_SIZE
#define P4_PAD_SIZE			(P4_INPUT_SIZE)
#endif

#ifndef P4_PIC_SIZE
#define P4_PIC_SIZE			(2 * sizeof (P4_Cell) * CHAR_BIT + 2)
#endif

/***********************************************************************
 *** No configuration below this point.
 ***********************************************************************/

#include <err.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
# include <stdint.h>
# endif
#endif

#ifdef HAVE_MATH_H
# include <math.h>
#endif
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_SYS_FILE_H
# include <sys/file.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

/***********************************************************************
 ***
 ***********************************************************************/

#define QUOTE(x)	QUOTE_THIS(x)
#define QUOTE_THIS(x)	#x
#define STRLEN(s)	(sizeof (s)-1)

/***********************************************************************
 *** Types
 ***********************************************************************/

#define P4_FALSE	( 0)
#define P4_TRUE		(~0)
#define P4_BOOL(x)	(-(x))		/* negate 2's complement C boolean */

typedef struct {
	int argc;
	char **argv;
	unsigned ds_size;
	unsigned rs_size;
	unsigned fs_size;
	const char *core_file;
	const char *block_file;
} P4_Options;

typedef void *P4_Code;			/* Address of labels, eg. ptr = &&label; */
typedef void *P4_Ptr;
#define P4_PTR_FMT "%lx"

typedef unsigned char	P4_Char;
typedef uintptr_t 	P4_Uint;
typedef intptr_t	P4_Int;

#define P4_CHAR_FMT	"0x%.2x"
#define P4_CHAR_BIT	CHAR_BIT
#define P4_CHAR_MAX	UCHAR_MAX

#define P4_INT_MAX	INTPTR_MAX
#define P4_UINT_MAX	UINTPTR_MAX
#define P4_UINT_BITS	LONG_BIT

#if P4_UINT_BITS == 64
# define P4_INT_FMT	"%ld"
# define P4_UINT_FMT	"%lu"
# define P4_HEX_FMT 	"$%.16lx"
# define P4_Uint_Half	uint32_t
# define DIV		ldiv
# define DIV_T		ldiv_t
#else
# define P4_INT_FMT	"%d"
# define P4_UINT_FMT	"%u"
# define P4_HEX_FMT 	"$%.8x"
# define P4_Uint_Half	uint16_t
# define DIV		div
# define DIV_T		div_t
#endif

#define P4_UINT_MSB	(~(~(P4_Uint)0 >> 1))
#define P4_HALF_SHIFT	(P4_UINT_BITS >> 1)
#define P4_LOWER_MASK	(~(P4_Uint)0 >> P4_HALF_SHIFT)

#define P4_SENTINEL	((P4_Uint)0xdeadbeefcafebabeL)

typedef size_t P4_Size;
#define P4_SIZE_FMT "%zu"

typedef time_t P4_Time;
#define P4_TIME_FMT 	P4_UINT_FMT

typedef union p4_cell P4_Cell;
typedef struct p4_word P4_Word;
typedef struct p4_ctx P4_Ctx;

typedef P4_Word *P4_Xt;

typedef struct {
	P4_Size		length;		/* Length of string less NUL byte. */
	P4_Char *	string;		/* Pointer to content plus terminating NUL byte. */
} P4_String;

typedef struct {
	FILE *		fp;		/* stdin or an open file, -1 string. */
	P4_Uint		blk;		/* If 0< then buffer is a block and this is the block number. */
	P4_Size		size;		/* Size of buffer. */
	P4_Size		length;		/* Length of input in buffer. */
	P4_Size		offset;		/* Offset of unconsumed input. */
	P4_Char *	buffer;
	P4_Int		unget;
} P4_Input;

#define P4_INPUT_IS_BLK(input)	((input).fp == (FILE *) -1 && (input).blk > 0)
#define P4_INPUT_IS_STR(input)	((input).fp == (FILE *) -1 && (input).blk == 0)
#define P4_INPUT_IS_FILE(input) ((input).fp != (FILE *) -1 && !P4_IS_TERM(input))
#define P4_INPUT_IS_TERM(input)	((input).fp == stdin)
#define P4_INPUT_PUSH(input)	{ P4_Input input_save = *input;
#define P4_INPUT_POP(input)	*input = input_save; }

typedef enum {
	P4_BLOCK_FREE,
	P4_BLOCK_CLEAN,
	P4_BLOCK_DIRTY,
} P4_Block_State;

typedef struct {
	P4_Int		state;
	P4_Uint		number;
	P4_Char		buffer[P4_BLOCK_SIZE];
} P4_Block;

union p4_cell {
	P4_Int		n;
	P4_Uint		u;
	P4_Size		z;
	P4_Time		t;
	P4_Cell	*	p;
	P4_Char *	s;
	P4_Word *	w;
	P4_Xt		xt;
};

#define P4_CELL				((P4_Int) sizeof (P4_Cell))
#define P4_ALIGN_SIZE(sz, pow2)    	(((sz) + (pow2-1)) & -(pow2))
#define P4_CELL_ALIGN(nbytes)		P4_ALIGN_SIZE(nbytes, sizeof (P4_Cell))

struct p4_word {
	/* Header */
	P4_Word *	prev;		/* Previous word definition. */
	P4_String	name;
	P4_Uint		bits;

#define P4_BIT_IMM			0x0001
#define P4_BIT_CREATED			0x0002
#define P4_BIT_HIDDEN			0x0004
#define P4_BIT_COMPILE			0x0008

#define P4_WORD_IS_IMM(w)		(((w)->bits & P4_BIT_IMM) == P4_BIT_IMM)
#define P4_WORD_SET_IMM(w)		((w)->bits |= P4_BIT_IMM)
#define P4_WORD_CLEAR_IMM(w)		((w)->bits &= ~P4_BIT_IMM)

#define P4_WORD_WAS_CREATED(w)		(((w)->bits & P4_BIT_CREATED) == P4_BIT_CREATED)
#define P4_WORD_SET_CREATED(w)		((w)->bits |= P4_BIT_CREATED)

#define P4_WORD_IS_HIDDEN(w)		(((w)->bits & P4_BIT_HIDDEN) == P4_BIT_HIDDEN)
#define P4_WORD_SET_HIDDEN(w)		((w)->bits |= P4_BIT_HIDDEN)
#define P4_WORD_CLEAR_HIDDEN(w)		((w)->bits &= ~P4_BIT_HIDDEN)

#define P4_WORD_IS_COMPILE(w)		(((w)->bits & P4_BIT_COMPILE) == P4_BIT_COMPILE)
#define P4_WORD_SET_COMPILE(w)		((w)->bits |= P4_BIT_COMPILE)
#define P4_WORD_CLEAR_COMPILE(w)	((w)->bits &= ~P4_BIT_COMPILE)

	/* Body */
	P4_Code		code;		/* Code field points primative. */
	P4_Size		mdata;		/* Size of data[] in address units. */
	P4_Size		ndata;		/* Length of data[] in address units. */
	P4_Cell		data[1];	/* Word grows by data cells. */
};

#define P4_WORD(name, code, bits)	{ NULL, { STRLEN(name), name }, bits, code, 0, 0, {{ 0 }} }

typedef struct {
	P4_Size		size;		/* Size of table in cells. */
	P4_Cell *	top;		/* Last element in the stack / array. */
	P4_Cell *	base;		/* Base of array; might be reallocated. */
} P4_Array, P4_Stack;

# define P4_PICK(stack, offset)		((stack).top[-(offset)])
# define P4_TOP(stack)			(*(stack).top)
# define P4_POP(stack)			(*(stack).top--)
# define P4_PUSH(stack, x)		(*++(stack).top = (P4_Cell)(x))
# define P4_RESET(stack)		((stack).top = (stack).base - 1)
# define P4_LENGTH(stack)		((stack).top + 1 - (stack).base)
# define P4_DROP(stack, n)		((stack).top -= (n))
# define P4_PUSH_CSTR(stack, s)		(P4_PUSH(stack, s), P4_PUSH(stack, strlen(s)))

typedef enum {
	P4_STATE_INTERPRET,
	P4_STATE_COMPILE = (-1),	/* Match Forth value for TRUE. */
} P4_State;

struct p4_ctx {
	P4_Stack	ds;		/* Data stack */
	P4_Stack	rs;		/* Return stack */
#ifdef HAVE_MATH_H
	P4_Stack	fs;		/* Float stack */
#endif
	P4_Int		state;
	P4_Word *	words;		/* Head of the dictionary word list. */
	P4_Uint		radix;		/* Input/Output radix */
	P4_Int		argc;
	char **		argv;
	P4_Input	input;
	P4_Block	block;
	P4_Int		block_fd;
	P4_Char		tty[P4_INPUT_SIZE];
	JMP_BUF		on_throw;
};

/***********************************************************************
 *** Exceptions
 ***********************************************************************/

#define P4_THROW_OK		( 0)

/* -255..-1 reserved by the standard. */

#define P4_THROW_ABORT		(-1)	/* ABORT */
#define P4_THROW_ABORT_MSG	(-2)	/* ABORT" */
#define P4_THROW_DS_OVER	(-3)	/* stack overflow */
#define P4_THROW_DS_UNDER	(-4)	/* stack underflow */
#define P4_THROW_RS_OVER	(-5)	/* return stack overflow */
#define P4_THROW_RS_UNDER	(-6)	/* return stack underflow */
#define P4_THROW_LOOP_DEPTH	(-7)	/* do-loops nested too deeply during execution */
#define P4_THROW_DICT_OVER	(-8)	/* dictionary overflow */
#define P4_THROW_SIGSEGV	(-9)	/* invalid memory address */
#define P4_THROW_DIV_ZERO	(-10)	/* division by zero */
#define P4_THROW_ERANGE		(-11)	/* result out of range */
#define P4_THROW_EINVAL		(-12)	/* argument type mismatch */
#define P4_THROW_UNDEFINED	(-13)	/* undefined word */
#define P4_THROW_COMPILE_ONLY	(-14)	/* interpreting a compile-only word */
#define P4_THROW__15		(-15)	/* invalid FORGET */
#define P4_THROW_EMPTY_NAME	(-16)	/* attempt to use zero-length string as a name */
#define P4_THROW_PIC_OVER	(-17)	/* pictured numeric output string overflow */
#define P4_THROW_PARSE_OVER	(-18)	/* parsed string overflow */
#define P4_THROW_NAME_TOO_LONG	(-19)	/* definition name too long */
#define P4_THROW__20		(-20)	/* write to a read-only location */
#define P4_THROW__21		(-21)	/* unsupported operation (e.g., AT-XY on a too-dumb terminal) */
#define P4_THROW_BAD_CONTROL	(-22)	/* control structure mismatch */
#define P4_THROW_SIGBUS		(-23)	/* address alignment exception */
#define P4_THROW_BAD_NUMBER	(-24)	/* invalid numeric argument */
#define P4_THROW_RS_IMBALANCE	(-25)	/* return stack imbalance */
#define P4_THROW__26		(-26)	/* loop parameters unavailable */
#define P4_THROW__27		(-27)	/* invalid recursion */
#define P4_THROW_SIGINT		(-28)	/* user interrupt */
#define P4_THROW_COMPILING	(-29)	/* compiler nesting */
#define P4_THROW__30		(-30)	/* obsolescent feature */
#define P4_THROW_NOT_CREATED	(-31)	/* word not defined by CREATE */
#define P4_THROW_BAD_NAME	(-32)	/* invalid name argument (e.g., TO xxx) */
#define P4_THROW_BLOCK_RD	(-33)	/* block read exception */
#define P4_THROW_BLOCK_WR	(-34)	/* block write exception */
#define P4_THROW_BLOCK_BAD	(-35)	/* invalid block number */
#define P4_THROW__36		(-36)	/* invalid file position */
#define P4_THROW_EIO		(-37)	/* file I/O exception */
#define P4_THROW_ENOENT		(-38)	/* non-existent file */
#define P4_THROW__39		(-39)	/* unexpected end of file */
#define P4_THROW_BAD_BASE	(-40)	/* invalid BASE for floating point conversion */
#define P4_THROW__41		(-41)	/* loss of precision */
#define P4_THROW__42		(-42)	/* floating-point divide by zero */
#define P4_THROW__43		(-43)	/* floating-point result out of range */
#define P4_THROW__44		(-44)	/* floating-point stack overflow */
#define P4_THROW__45		(-45)	/* floating-point stack underflow */
#define P4_THROW__46		(-46)	/* floating-point invalid argument */
#define P4_THROW__47		(-47)	/* compilation word list deleted */
#define P4_THROW__48		(-48)	/* invalid POSTPONE */
#define P4_THROW__49		(-49)	/* search-order overflow */
#define P4_THROW__50		(-50)	/* search-order underflow */
#define P4_THROW__51		(-51)	/* compilation word list changed */
#define P4_THROW__52		(-52)	/* control-flow stack overflow */
#define P4_THROW__53		(-53)	/* exception stack overflow */
#define P4_THROW__54		(-54)	/* floating-point underflow */
#define P4_THROW_SIGFPE		(-55)	/* floating-point unidentified fault */
#define P4_THROW_QUIT		(-56)	/* QUIT */
#define P4_THROW__57		(-57)	/* exception in sending or receiving a character */
#define P4_THROW__58		(-58)	/* [IF], [ELSE], or [THEN] exception */
#define P4_THROW_ALLOCATE	(-59)	/* ALLOCATE */
#define P4_THROW__60		(-60)	/* FREE */
#define P4_THROW_RESIZE		(-61)	/* RESIZE */
#define P4_THROW__62		(-62)	/* CLOSE-FILE */
#define P4_THROW__63		(-63)	/* CREATE-FILE */
#define P4_THROW__64		(-64)	/* DELETE-FILE */
#define P4_THROW__65		(-65)	/* FILE-POSITION */
#define P4_THROW__66		(-66)	/* FILE-SIZE */
#define P4_THROW__67		(-67)	/* FILE-STATUS */
#define P4_THROW__68		(-68)	/* FLUSH-FILE */
#define P4_THROW__69		(-69)	/* OPEN-FILE */
#define P4_THROW__70		(-70)	/* READ-FILE */
#define P4_THROW__71		(-71)	/* READ-LINE */
#define P4_THROW__72		(-72)	/* RENAME-FILE */
#define P4_THROW__73		(-73)	/* REPOSITION-FILE */
#define P4_THROW__74		(-74)	/* RESIZE-FILE */
#define P4_THROW__75		(-75)	/* WRITE-FILE */
#define P4_THROW__76		(-76)	/* WRITE-LINE */
#define P4_THROW__77		(-77)	/* Malformed xchar */
#define P4_THROW__78		(-78)	/* SUBSTITUTE */
#define P4_THROW__79		(-79)	/* REPLACES */
#define P4_THROW_future		(-80)	/* -80 .. -255 reserved for future assignment */

/* -4095..-256 reserved for system assignment. */

/***********************************************************************
 *** API
 ***********************************************************************/

/**
 * Initialise the global environment.
 */
extern void p4Init(void);

/**
 * Finalise the global environment.
 */
extern void p4Fini(void);

/**
 * Create a new interpreter context.
 *
 * @param opts
 *	Pointer to P4_Options.
 *
 * @return
 *	A pointer to an allocated P4_Ctx structure.
 */
extern P4_Ctx *p4Create(P4_Options *opts);

/**
 * @param ctx
 *	A pointer to an allocated P4_Ctx structure to free.
 */
extern void p4Free(P4_Ctx *ctx);

/**
 * @param ctx
 *	A pointer to an allocated P4_Ctx structure.
 *
 * @return
 *	Zero on success, otherwise an exception code other than BYE.
 */
extern int p4Eval(P4_Ctx *ctx);

/**
 * @param ctx
 *	A pointer to an allocated P4_Ctx structure.
 *
 * @param filepath
 *	A C string of a file path name to interpret.
 *	If NULL, then standard input will be read.
 *
 * @return
 *	Zero on success, otherwise an exception code other than BYE.
 */
extern int p4EvalFile(P4_Ctx *ctx, const char *filepath);

/**
 * @param ctx
 *	A pointer to an allocated P4_Ctx structure.
 *
 * @param string
 *	A string to interpret, possible not NUL terminated.
 *
 * @param length
 *	Length of string.
 *
 * @return
 *	Zero on success, otherwise an exception code other than BYE.
 */
extern int p4EvalString(P4_Ctx *ctx, P4_Char *string, size_t length);

/***********************************************************************
 *** Utility Functions
 ***********************************************************************/

/**
 * @param ch
 *	A backslash escaped character.
 *
 * @return
 *	The value that the backslash escape character represents.
 *
 *	\a	bell
 *	\b	backspace
 *	\e	escape
 *	\f	formfeed
 *	\n	linefeed
 *	\r	carriage-return
 *	\s	space
 *	\t	tab
 *	\v	vertical tab
 *	\?	delete
 *
 *	Other characters remain as themselves.
 */
extern int p4CharLiteral(int ch);

/**
 * @param s
 *	A pointer to a C string to reverse in place.
 *
 * @param length
 *	The length of the C string.
 */
extern void p4StrRev(P4_Char *s, P4_Size length);

extern int p4StrNum(P4_String str, P4_Uint base, P4_Int *out);

extern P4_Int p4GetC(P4_Input *source);

extern P4_Int p4Accept(P4_Input *source, P4_Char *buffer, P4_Size size);

/**
 * Handles parsing of "ccc<char>".
 *
 * @param input
 *	A pointer to an already filled input buffer.
 *
 * @param delim
 *	Input is read until this delimiter is seen. If delimiter
 *	is space ( ), then treat all white space as a delimiter.
 *
 * @param escape
 *	When true, allow for backslash escaped characters.
 *
 * @return
 *	A P4_String structure containing a pointer within the
 *	input buffer and the length of the string upto the delim
 *	character.
 *
 * @note
 *	Backslash escaped characters are converted to a literal and
 *	the input buffer reduced in size.
 *
 * @standard ANS-Forth 1994, extended
 */
extern P4_String p4Parse(P4_Input *input, P4_Uint delim, P4_Uint escape);

/**
 * Handles parsing of "<space>ccc<space>", skipping leading occurences of <char>.
 *
 * @param input
 *	A pointer to an already filled input buffer.
 *
 * @return
 *	The length of the parsed input; otherwise 0 on EOF or error.
 */
extern P4_String p4ParseName(P4_Input *input);

extern P4_Word *p4FindName(P4_Ctx *ctx, P4_Char *caddr, P4_Size length);

/***********************************************************************
 *** END
 ***********************************************************************/

#ifdef  __cplusplus
}
#endif

#endif /* __post4_h__ */
