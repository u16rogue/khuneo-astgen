#include "lexer.h"
#include <kh-core/utf8.h>

// ---------------------------------------------------------------------------------------------------- 

static const kh_utf8 tok_charsyms[] = {
  '+',
  '-',
  '*',
  '/',
  '#',
  '\\',
  '|',
  '&',
  '=',
  '@',
  '!',
  ':',
};

static const kh_utf8 tok_charsyms_pair[][2] = {
  { '(', ')' },
  { '{', '}' },
  { '[', ']' },
  { '<', '>' },
};

static const kh_utf8 tok_stringcont[] = {
  '\'',
  '`',
  '"'
};

// ---------------------------------------------------------------------------------------------------- 

#if defined(KH_TRACK_LINE_COLUMN)
  #define KH_HLP_ADD_COLUMN(x) ctx->column += x
#else
  #define KH_HLP_ADD_COLUMN(x)
#endif

static kh_bool is_src_end(kh_lexer_run_context * ctx, kh_u32 offset) {
  return (ctx->isrc + offset) >= ctx->src_size;
}

/*
 *  [14/04/2023]
 *  acquire_entry does not allocate memory, memory is provided by
 *  the caller and is expanded by responding to a KH_LEXER_RESPONSE_BUFFER_EXHAUSTED
 *  therefore it should be guaranteed that the only reason acquire_entry would ever
 *  return NULL is to request a buffer reallocation. When calling acquire_entry and receiving
 *  a NULL simply return a KH_LEX_ABORT and the lexer will check the context and tell the caller
 *  for more memory.
 *
 *  TL;DR:
 *  > It's guaranteed that a NULL return is not a critical error and to return with KH_LEX_ABORT
 *  upon receiving one.
 *  > To not modify `ctx->status`
 *  > Even if it says ABORT it will not shutdown the lexer, when ctx is set to signal a buffer exhaust the abort
 *  is turned into the appropriate *_BUFFER_EXHAUSTED
 *
 */
static kh_lexer_token_entry * acquire_entry(kh_lexer_run_context * ctx) {
  kh_sz new_index = ctx->itoken_buffer + sizeof(kh_lexer_run_context);
  if (new_index >= ctx->token_buffer_size) {
    ctx->status = KH_LEXER_STATUS_BUFFER_EXHAUSTED;
    return 0;
  }

  kh_lexer_token_entry * new_entry = (kh_lexer_token_entry *)((kh_u8 *)ctx->token_buffer + ctx->itoken_buffer);
  ctx->itoken_buffer = new_index;
  return new_entry;
}

// ---------------------------------------------------------------------------------------------------- 

typedef enum _kh_lex_resp {
  KH_LEX_MATCH,
  KH_LEX_PASS,
  KH_LEX_ABORT, // [10/04/2023] When returning an abort status it's recommended to set ctx->status to let the caller know what happened. or dont. (you can use KH_LEXER_STATUS_UNKERR)
} kh_lex_resp;

static kh_lex_resp lex_whitespace(kh_lexer_run_context * ctx) {
  switch (ctx->src[ctx->isrc]) {
    case ' ':
      KH_HLP_ADD_COLUMN(1);
      break;
    case '\r':
#if defined(KH_TRACK_LINE_COLUMN)
      ctx->column = 1;
#endif
      break;
    case '\n':
#if defined(KH_TRACK_LINE_COLUMN)
      ctx->column = 1;
      ++ctx->line;
#endif
      break;
    case '\t':
#if defined(KH_TRACK_LINE_COLUMN)
#if !defined(KH_TAB_SPACE_COUNT)
#error "khuneo > astgen > lexer > KH_TRACK_LINE_COLUMN is enabled please define KH_TAB_SPACE_COUNT to a numerical value to represent the equivalent space count of a tab character."
#endif
      ctx->column += KH_TAB_SPACE_COUNT;
#endif
      break;
    default:
      return KH_LEX_PASS;
  }

  ++ctx->isrc;
  return KH_LEX_MATCH;
}

static kh_lex_resp lex_comments(kh_lexer_run_context * ctx) {
  if (ctx->src[ctx->isrc] != '/' || is_src_end(ctx, 1))
    return KH_LEX_PASS;

  kh_utf8 cnext = ctx->src[ctx->isrc + 1];
  kh_bool single_line = cnext == '/';
  if (!single_line && cnext != '*')
    return KH_LEX_PASS;

  ctx->isrc += 2;
  KH_HLP_ADD_COLUMN(2);

  const kh_u32 delim_offset = single_line ? 1 : 2; // [11/04/2023] '\n' or "*/"

  // [11/04/2023] Under the impression that the boolean expression will
  // produce a 0 false or a 1 true we use them as a seek value. the commented code shows the intent
  while (!is_src_end(ctx, !single_line /*? 0 : 1*/)) { 

    if ( (single_line  && ctx->src[ctx->isrc] == '\n') // For single line comments
    ||   (!single_line && *(const kh_u16 *)&ctx->src[ctx->isrc] == *(const kh_u16 *)"*/") // For multiline comments
    ) {
      ctx->isrc += delim_offset;
      KH_HLP_ADD_COLUMN(delim_offset);
      break;
    }

    // [11/04/2023] TODO: not necessary but it would be good to check if it responds with a KH_LEX_ABORT
    if (lex_whitespace(ctx) == KH_LEX_MATCH) { // [11/04/2023] NOTE: lex_whitespace already advances ctx->isrc
      continue;
    }

    kh_sz char_sz = kh_utf8_char_len(ctx->src[ctx->isrc]);
    if (char_sz == -1) {
      ctx->status = KH_LEXER_STATUS_INVALID_UTF8;
      return KH_LEX_ABORT;
    }

    ctx->isrc += char_sz;
    KH_HLP_ADD_COLUMN(1);
  }

  return KH_LEX_MATCH;
}

static kh_lex_resp lex_charsymbols(kh_lexer_run_context * ctx) {
  const kh_u32 nchars  = sizeof(tok_charsyms) / sizeof(tok_charsyms[0]);
  const kh_u32 ngroups = sizeof(tok_charsyms_pair) / sizeof(tok_charsyms_pair[0]);

  kh_lexer_token_entry * entry = 0;
  const kh_utf8 cch = ctx->src[ctx->isrc];
  for (kh_u32 i = 0; i < nchars; ++i) {
    if (tok_charsyms[i] != cch)
      continue;
    entry = acquire_entry(ctx);
    if (!entry) {
      return KH_LEX_ABORT;
    }
    return KH_LEX_MATCH;
  }

  // [17/04/2023] Its separated incase we want to do lazy evaluation and parse as groups
  for (kh_u32 i = 0; i < ngroups; ++i) {
    if (tok_charsyms_pair[i][0] == cch && tok_charsyms_pair[i][1] == cch)
      continue;
    entry = acquire_entry(ctx);
    if (!entry) {
      return KH_LEX_ABORT;
    }
    return KH_LEX_MATCH;
  }

  if (!entry)
    return KH_LEX_PASS;

  entry->type          = KH_TOK_CHARSYM;
  entry->value.charsym = cch;
#if defined(KH_TRACK_LINE_COLUMN)
  entry->line   = ctx->line;
  entry->column = ctx->column;
  ++ctx->column;
#endif

  ++ctx->isrc;
  return KH_LEX_MATCH;
}

static kh_lex_resp lex_strings(kh_lexer_run_context * ctx) {
  kh_utf8 str_delim = '\0';
  const kh_u32 nstrdelim = sizeof(tok_stringcont) / sizeof(tok_stringcont[0]);

  for (kh_u32 i = 0; i < nstrdelim; ++i) {
    if (tok_stringcont[i] == ctx->src[ctx->isrc]) {
      str_delim = tok_stringcont[i];
      break;
    }
  }

  if (str_delim == '\0')
    return KH_LEX_PASS;

  kh_lexer_token_entry * entry = acquire_entry(ctx);
  if (!entry)
    return KH_LEX_ABORT;

  kh_sz start_index = ctx->isrc;
  ++ctx->isrc;
  KH_HLP_ADD_COLUMN(1);

  while (!is_src_end(ctx, 0)) {
    kh_utf8 cc = ctx->src[ctx->isrc];
    if (cc == str_delim && ctx->isrc != 1 && ctx->src[ctx->isrc - 1] != '\\') {
      str_delim = '\0';
      break;
    }

    if (lex_whitespace(ctx) == KH_LEX_MATCH)
      continue;

    kh_sz csz = kh_utf8_char_len(cc);
    if (csz == -1) {
      ctx->status = KH_LEXER_STATUS_INVALID_UTF8;
      return KH_LEX_ABORT;
    }

    ctx->isrc += csz;
    KH_HLP_ADD_COLUMN(1);
  }

  // [17/04/2023] We set str_delim back to \0 to signal the ending string delimeter was found, if the loop
  // ended without this flag being set that means the string is malformed
  if (str_delim != '\0') {
    ctx->status = KH_LEXER_STATUS_INVALID_STRING_SYNTAX;
    return KH_LEX_ABORT;
  }

  // Move from the matched str_delim
  ++ctx->isrc;
  KH_HLP_ADD_COLUMN(1);

  entry->type = KH_TOK_STRING;
  entry->value.string.index = start_index;
  entry->value.string.size  = ctx->isrc - start_index;

  return KH_LEX_MATCH;
}

static kh_lex_resp lex_keywords(kh_lexer_run_context * ctx) {
  return KH_LEX_PASS;
}

static kh_lex_resp lex_identifiers(kh_lexer_run_context * ctx) {
  return KH_LEX_PASS;
}

static kh_lex_resp lex_numbers(kh_lexer_run_context * ctx) {
  return KH_LEX_PASS;
}

typedef kh_lex_resp(*lexer_cb_t)(kh_lexer_run_context *);

static const lexer_cb_t lexers[] = {
  lex_whitespace,
  lex_comments,
  lex_charsymbols,
  lex_strings,
  lex_keywords,
  lex_identifiers,
  lex_numbers,
};

// ---------------------------------------------------------------------------------------------------- 

kh_lexer_response kh_lexer(kh_lexer_run_context * ctx) {
  const int nlexers = sizeof(lexers) / sizeof(void *);
  kh_lex_resp resp = KH_LEX_ABORT;

  while (!is_src_end(ctx, 0)) {

    for (int i = 0; i < nlexers; ++i) {
      resp = lexers[i](ctx);
      if (resp == KH_LEX_PASS) {
        continue;
      } else if (resp == KH_LEX_ABORT) {
        if (ctx->status == KH_LEXER_STATUS_BUFFER_EXHAUSTED) // [14/04/2023] Respond with a buffer exhaust instead if that's the status so we dont shutdown the lexer
          return KH_LEXER_RESPONSE_BUFFER_EXHAUSTED;
        return KH_LEXER_RESPONSE_ERROR; // [10/04/2023] We dont set ctx->status as the lexer callbacks might've set it
      } else if (resp == KH_LEX_MATCH) {
        break;
      } else {
        // [10/04/2023] TODO: maybe throw an error? this should never be reachable
      }
      // [10/04/2023] This too should not reachable
    }

    // [10/04/2023] Abort if its still a pass which indicates no lexer matches
    if (resp == KH_LEX_PASS) {
      ctx->status = KH_LEXER_STATUS_NO_LEX_MATCH;
      return KH_LEXER_RESPONSE_ERROR;
    }

    // [10/04/2023] Under the assumption that the state is OK and we had a match
  }
  return KH_LEXER_RESPONSE_OK;
}

#undef KH_HLP_ADD_COLUMN
