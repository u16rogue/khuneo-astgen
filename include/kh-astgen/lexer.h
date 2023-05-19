#pragma once

#include <kh-core/types.h>

typedef enum _kh_token_type {
  KH_TOK_INVALID,
  KH_TOK_IDENTIFIER,
  KH_TOK_STRING,
  KH_TOK_CHARSYM,
  KH_TOK_U64,
  KH_TOK_F64,
} kh_token_type;

typedef enum _kh_lexer_status {
  KH_LEXER_STATUS_OK,
  KH_LEXER_STATUS_UNKERR,
  KH_LEXER_STATUS_INVALID_UTF8,
  KH_LEXER_STATUS_NO_LEX_MATCH,
  KH_LEXER_STATUS_BUFFER_EXHAUSTED,
  KH_LEXER_STATUS_SYNTAX_ERROR,
  KH_LEXER_STATUS_INVALID_STRING_SYNTAX, // [17/04/2023] could really name this better
} kh_lexer_status;

typedef enum _kh_lexer_response {
  // Lexer finished parsing the tokens
  KH_LEXER_RESPONSE_OK,

  // Lexer failure (must abort)
  KH_LEXER_RESPONSE_ERROR,

  // Provided buffer in context has been exhausted, replace the buffer with a new one and call lexer again
  // with the same context to continue. The memory reallocation is left to the caller. You can either create a new buffer
  // or realloc the same memory then just set token_buffer at the start of the extended memory and setting token_buffer_size
  // to the extended memory size keeping the entire memory block contiguous
  KH_LEXER_RESPONSE_BUFFER_EXHAUSTED, 
} kh_lexer_response;

typedef union _kh_lexer_token_entry_value {
  kh_utf8    charsym;
  kh_u64     u64;
  kh_f64     f64;

  struct {
    kh_u32 index;
    kh_u32 size;
  } string;
} kh_lexer_token_entry_value;

typedef struct _kh_lexer_token_entry {
  kh_token_type              type;
  kh_lexer_token_entry_value value;

#if defined(KH_TRACK_LINE_COLUMN)
  kh_u32 line;
  kh_u32 column;
#endif
} kh_lexer_token_entry;

typedef struct _kh_lexer_context {
  kh_lexer_status status;

  kh_lexer_token_entry * token_buffer;      // Pointer to a buffer
  kh_sz                  token_buffer_size; // TOTAL size of the buffer in SIZE (not count)
  kh_sz                  itoken_buffer;     // Token buffer index (in bytes)

  const kh_utf8 * src;      // Source
  kh_sz           src_size; // Source length (in bytes)
  kh_sz           isrc;     // Source index

#if defined(KH_TRACK_LINE_COLUMN)
  kh_u32 line;
  kh_u32 column;
#endif
} kh_lexer_context;

/*
 *  Runs the lexer with a given context.
 */
kh_lexer_response kh_lexer(kh_lexer_context * ctx);

/*
 *  Obtains the first token entry in a context.
 *  `c` is an out pointer.
 *  Returns true if there is an entry and was placed on `c` otherwise false
 */
kh_bool kh_lexer_token_entry_first(kh_lexer_context * ctx, kh_lexer_token_entry ** c);

/*
 *  Obtains the next token based off the current `c`
 *  `c` is an in and out pointer
 *  Returns true if a next entry is available and has been placed to `c` otherwise false
 */
kh_bool kh_lexer_token_entry_next(kh_lexer_context * ctx, kh_lexer_token_entry ** c);

/*
 *  Obtains the type field of a token entry
 */
kh_token_type kh_lexer_token_entry_type_get(const kh_lexer_token_entry * c);

/*
 *  Obtains the line field of a token entry. Returns 0
 *  if `Line` is not being tracked.
 *  (As it should be impossible to have line 0)
 */
kh_u32 kh_lexer_token_entry_line_get(const kh_lexer_token_entry * c);

/*
 *  Obtains the column field of a token entry. Returns 0
 *  if `Line` is not being tracked
 *  (As it should be impossible to have line 0)
 */
kh_u32 kh_lexer_token_entry_column_get(const kh_lexer_token_entry * c);

/*
 *  Reports the size of the structure `kh_lexer_token_entry`
 */
const kh_sz kh_lexer_token_entry_size();

/*
 *  Obtains the value of a token entry
 */
kh_u64  kh_lexer_token_entry_value_u64_get(const kh_lexer_token_entry * c);
kh_f64  kh_lexer_token_entry_value_f64_get(const kh_lexer_token_entry * c);
kh_utf8 kh_lexer_token_entry_value_charsym_get(const kh_lexer_token_entry * c);

/*
 *  Obtains the value field of string type token entry
 */
kh_u32 kh_lexer_token_entry_value_str_index_get(const kh_lexer_token_entry * c); 
kh_u32 kh_lexer_token_entry_value_str_sz_get(const kh_lexer_token_entry * c);
