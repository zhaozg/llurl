/* Copyright (c) 2024 llurl contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "llurl.h"
#include <string.h>

/* State machine states for URL parsing */
enum state {
  s_dead = 0,
  s_start,
  s_schema,
  s_schema_slash,
  s_schema_slash_slash,
  s_server_start,
  s_server,
  s_server_with_at,
  s_path,
  s_query_or_fragment,
  s_query,
  s_fragment,
  s_num_states
};

/* Character classes for DFA table */
enum char_class {
  cc_invalid = 0,     /* Invalid characters */
  cc_alpha,           /* a-z, A-Z */
  cc_digit,           /* 0-9 */
  cc_slash,           /* / */
  cc_colon,           /* : */
  cc_question,        /* ? */
  cc_hash,            /* # */
  cc_at,              /* @ */
  cc_dot,             /* . */
  cc_dash,            /* - */
  cc_plus,            /* + */
  cc_percent,         /* % */
  cc_ampersand,       /* & */
  cc_equals,          /* = */
  cc_semicolon,       /* ; */
  cc_dollar,          /* $ */
  cc_exclamation,     /* ! */
  cc_asterisk,        /* * */
  cc_comma,           /* , */
  cc_lparen,          /* ( */
  cc_rparen,          /* ) */
  cc_apostrophe,      /* ' */
  cc_underscore,      /* _ */
  cc_tilde,           /* ~ */
  cc_lbracket,        /* [ */
  cc_rbracket,        /* ] */
  cc_pipe,            /* | */
  cc_lbrace,          /* { */
  cc_rbrace,          /* } */
  cc_num_char_classes
};

/* Character classification lookup tables for performance */

/* Lookup table for userinfo characters (user:pass@host) */
/* Includes ALPHANUM + MARK + %:;&=+$, */
static const unsigned char userinfo_char[256] = {
/*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
        0,       1,       0,       0,       1,       1,       1,       1,
/*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
        1,       1,       1,       1,       1,       1,       1,       0,
/*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
        1,       1,       1,       1,       1,       1,       1,       1,
/*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
        1,       1,       1,       1,       0,       1,       0,       0,
/*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
        0,       1,       1,       1,       1,       1,       1,       1,
/*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
        1,       1,       1,       1,       1,       1,       1,       1,
/*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
        1,       1,       1,       1,       1,       1,       1,       1,
/*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
        1,       1,       1,       0,       0,       0,       0,       1,
/*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
        0,       1,       1,       1,       1,       1,       1,       1,
/* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
        1,       1,       1,       1,       1,       1,       1,       1,
/* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
        1,       1,       1,       1,       1,       1,       1,       1,
/* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
        1,       1,       1,       0,       0,       0,       1,       0,
/* 128-255: Extended ASCII - all invalid */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/* Character class lookup table for DFA - maps each byte to a character class */
static const unsigned char char_class_table[256] = {
/*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
/*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
/*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
/*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
/*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
  cc_invalid,cc_exclamation,cc_invalid,cc_hash,cc_dollar,cc_percent,cc_ampersand,cc_apostrophe,
/*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
  cc_lparen,cc_rparen,cc_asterisk,cc_plus,cc_comma,cc_dash,cc_dot,cc_slash,
/*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
  cc_digit,cc_digit,cc_digit,cc_digit,cc_digit,cc_digit,cc_digit,cc_digit,
/*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
  cc_digit,cc_digit,cc_colon,cc_semicolon,cc_invalid,cc_equals,cc_invalid,cc_question,
/*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
  cc_at,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,
/*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
  cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,
/*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
  cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,
/*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
  cc_alpha,cc_alpha,cc_alpha,cc_lbracket,cc_invalid,cc_rbracket,cc_invalid,cc_underscore,
/*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
  cc_invalid,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,
/* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
  cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,
/* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
  cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,cc_alpha,
/* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
  cc_alpha,cc_alpha,cc_alpha,cc_lbrace,cc_pipe,cc_rbrace,cc_tilde,cc_invalid,
/* 128-255: Extended ASCII - all invalid */
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,
  cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid,cc_invalid
};

/* Branch prediction hints for performance */
#if defined(__GNUC__) || defined(__clang__)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

/* DFA state transition table: url_state_table[current_state][char_class] = next_state
 * This table drives the state machine, eliminating most switch-case overhead
 * s_dead (0) indicates an error/invalid transition
 * Special value 0xFF means "stay in current state"
 */
#define STAY 0xFF
static const unsigned char url_state_table[s_num_states][cc_num_char_classes] = {
  /* s_dead - error state, all transitions lead to dead */
  { s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead,
    s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead,
    s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead },

  /* s_start - initial state */
  { s_dead,       /* cc_invalid */
    s_schema,     /* cc_alpha - start of schema */
    s_dead,       /* cc_digit */
    s_path,       /* cc_slash - relative URL */
    s_dead,       /* cc_colon */
    s_dead,       /* cc_question */
    s_dead,       /* cc_hash */
    s_dead,       /* cc_at */
    s_dead,       /* cc_dot */
    s_dead,       /* cc_dash */
    s_dead,       /* cc_plus */
    s_dead,       /* cc_percent */
    s_dead,       /* cc_ampersand */
    s_dead,       /* cc_equals */
    s_dead,       /* cc_semicolon */
    s_dead,       /* cc_dollar */
    s_dead,       /* cc_exclamation */
    s_path,       /* cc_asterisk - * for asterisk-form */
    s_dead,       /* cc_comma */
    s_dead,       /* cc_lparen */
    s_dead,       /* cc_rparen */
    s_dead,       /* cc_apostrophe */
    s_dead,       /* cc_underscore */
    s_dead,       /* cc_tilde */
    s_dead,       /* cc_lbracket */
    s_dead,       /* cc_rbracket */
    s_dead,       /* cc_pipe */
    s_dead,       /* cc_lbrace */
    s_dead },     /* cc_rbrace */

  /* s_schema - parsing schema (http, https, etc.) */
  { s_dead,       /* cc_invalid */
    STAY,         /* cc_alpha */
    STAY,         /* cc_digit */
    s_dead,       /* cc_slash */
    s_schema_slash, /* cc_colon - end of schema */
    s_dead,       /* cc_question */
    s_dead,       /* cc_hash */
    s_dead,       /* cc_at */
    STAY,         /* cc_dot */
    STAY,         /* cc_dash */
    STAY,         /* cc_plus */
    s_dead,       /* cc_percent */
    s_dead,       /* cc_ampersand */
    s_dead,       /* cc_equals */
    s_dead,       /* cc_semicolon */
    s_dead,       /* cc_dollar */
    s_dead,       /* cc_exclamation */
    s_dead,       /* cc_asterisk */
    s_dead,       /* cc_comma */
    s_dead,       /* cc_lparen */
    s_dead,       /* cc_rparen */
    s_dead,       /* cc_apostrophe */
    s_dead,       /* cc_underscore */
    s_dead,       /* cc_tilde */
    s_dead,       /* cc_lbracket */
    s_dead,       /* cc_rbracket */
    s_dead,       /* cc_pipe */
    s_dead,       /* cc_lbrace */
    s_dead },     /* cc_rbrace */

  /* s_schema_slash - expect first / after schema: */
  { s_dead, s_dead, s_dead, s_schema_slash_slash, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead,
    s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead,
    s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead },

  /* s_schema_slash_slash - expect second / after schema:/ */
  { s_dead, s_dead, s_dead, s_server_start, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead,
    s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead,
    s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead },

  /* s_server_start - start of server/host, handled specially in code */
  { s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead,
    s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead,
    s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead },

  /* s_server - parsing server/host */
  { s_dead,       /* cc_invalid */
    STAY,         /* cc_alpha */
    STAY,         /* cc_digit */
    s_path,       /* cc_slash - end of host, start path */
    STAY,         /* cc_colon - port separator or IPv6 */
    s_query_or_fragment, /* cc_question - start query */
    s_dead,       /* cc_hash */
    s_server_with_at, /* cc_at - previous was userinfo */
    STAY,         /* cc_dot */
    STAY,         /* cc_dash */
    STAY,         /* cc_plus */
    STAY,         /* cc_percent */
    STAY,         /* cc_ampersand */
    STAY,         /* cc_equals */
    STAY,         /* cc_semicolon */
    STAY,         /* cc_dollar */
    STAY,         /* cc_exclamation */
    STAY,         /* cc_asterisk */
    STAY,         /* cc_comma */
    STAY,         /* cc_lparen */
    STAY,         /* cc_rparen */
    STAY,         /* cc_apostrophe */
    STAY,         /* cc_underscore */
    STAY,         /* cc_tilde */
    STAY,         /* cc_lbracket - IPv6 */
    STAY,         /* cc_rbracket - IPv6 */
    STAY,         /* cc_pipe */
    STAY,         /* cc_lbrace */
    STAY },       /* cc_rbrace */

  /* s_server_with_at - parsing server after @ (cannot have another @) */
  { s_dead,       /* cc_invalid */
    STAY,         /* cc_alpha */
    STAY,         /* cc_digit */
    s_path,       /* cc_slash */
    STAY,         /* cc_colon */
    s_query_or_fragment, /* cc_question */
    s_dead,       /* cc_hash */
    s_dead,       /* cc_at - double @ not allowed */
    STAY,         /* cc_dot */
    STAY,         /* cc_dash */
    STAY,         /* cc_plus */
    STAY,         /* cc_percent */
    STAY,         /* cc_ampersand */
    STAY,         /* cc_equals */
    STAY,         /* cc_semicolon */
    STAY,         /* cc_dollar */
    STAY,         /* cc_exclamation */
    STAY,         /* cc_asterisk */
    STAY,         /* cc_comma */
    STAY,         /* cc_lparen */
    STAY,         /* cc_rparen */
    STAY,         /* cc_apostrophe */
    STAY,         /* cc_underscore */
    STAY,         /* cc_tilde */
    STAY,         /* cc_lbracket */
    STAY,         /* cc_rbracket */
    STAY,         /* cc_pipe */
    STAY,         /* cc_lbrace */
    STAY },       /* cc_rbrace */

  /* s_path - parsing path */
  { s_dead,       /* cc_invalid */
    STAY,         /* cc_alpha */
    STAY,         /* cc_digit */
    STAY,         /* cc_slash */
    STAY,         /* cc_colon */
    s_query_or_fragment, /* cc_question */
    s_query_or_fragment, /* cc_hash */
    STAY,         /* cc_at */
    STAY,         /* cc_dot */
    STAY,         /* cc_dash */
    STAY,         /* cc_plus */
    STAY,         /* cc_percent */
    STAY,         /* cc_ampersand */
    STAY,         /* cc_equals */
    STAY,         /* cc_semicolon */
    STAY,         /* cc_dollar */
    STAY,         /* cc_exclamation */
    STAY,         /* cc_asterisk */
    STAY,         /* cc_comma */
    STAY,         /* cc_lparen */
    STAY,         /* cc_rparen */
    STAY,         /* cc_apostrophe */
    STAY,         /* cc_underscore */
    STAY,         /* cc_tilde */
    STAY,         /* cc_lbracket */
    STAY,         /* cc_rbracket */
    STAY,         /* cc_pipe */
    STAY,         /* cc_lbrace */
    STAY },       /* cc_rbrace */

  /* s_query_or_fragment - determining whether ? or # comes next */
  { s_dead, s_dead, s_dead, s_dead, s_dead, s_query, s_fragment, s_dead, s_dead, s_dead,
    s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead,
    s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead, s_dead },

  /* s_query - parsing query string */
  { s_dead,       /* cc_invalid */
    STAY,         /* cc_alpha */
    STAY,         /* cc_digit */
    STAY,         /* cc_slash */
    STAY,         /* cc_colon */
    STAY,         /* cc_question - ? allowed in query */
    s_fragment,   /* cc_hash - start fragment */
    STAY,         /* cc_at */
    STAY,         /* cc_dot */
    STAY,         /* cc_dash */
    STAY,         /* cc_plus */
    STAY,         /* cc_percent */
    STAY,         /* cc_ampersand */
    STAY,         /* cc_equals */
    STAY,         /* cc_semicolon */
    STAY,         /* cc_dollar */
    STAY,         /* cc_exclamation */
    STAY,         /* cc_asterisk */
    STAY,         /* cc_comma */
    STAY,         /* cc_lparen */
    STAY,         /* cc_rparen */
    STAY,         /* cc_apostrophe */
    STAY,         /* cc_underscore */
    STAY,         /* cc_tilde */
    STAY,         /* cc_lbracket */
    STAY,         /* cc_rbracket */
    STAY,         /* cc_pipe */
    STAY,         /* cc_lbrace */
    STAY },       /* cc_rbrace */

  /* s_fragment - parsing fragment */
  { s_dead,       /* cc_invalid */
    STAY,         /* cc_alpha */
    STAY,         /* cc_digit */
    STAY,         /* cc_slash */
    STAY,         /* cc_colon */
    STAY,         /* cc_question - ? allowed in fragment */
    STAY,         /* cc_hash - # allowed in fragment */
    STAY,         /* cc_at */
    STAY,         /* cc_dot */
    STAY,         /* cc_dash */
    STAY,         /* cc_plus */
    STAY,         /* cc_percent */
    STAY,         /* cc_ampersand */
    STAY,         /* cc_equals */
    STAY,         /* cc_semicolon */
    STAY,         /* cc_dollar */
    STAY,         /* cc_exclamation */
    STAY,         /* cc_asterisk */
    STAY,         /* cc_comma */
    STAY,         /* cc_lparen */
    STAY,         /* cc_rparen */
    STAY,         /* cc_apostrophe */
    STAY,         /* cc_underscore */
    STAY,         /* cc_tilde */
    STAY,         /* cc_lbracket */
    STAY,         /* cc_rbracket */
    STAY,         /* cc_pipe */
    STAY,         /* cc_lbrace */
    STAY }        /* cc_rbrace */
};

/* Character classification macros for performance - expand inline */
#define IS_ALPHA(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_HEX(c) (IS_DIGIT(c) || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))
#define IS_ALPHANUM(c) (IS_ALPHA(c) || IS_DIGIT(c))

/* Check if character is alpha */
static inline int is_alpha(unsigned char ch) {
  return IS_ALPHA(ch);
}

/* Check if character is digit */
static inline int is_digit(unsigned char ch) {
  return IS_DIGIT(ch);
}

/* Check if character is valid in userinfo (user:pass@host) */
static inline int is_userinfo_char(unsigned char ch) {
  return userinfo_char[ch];
}

/* Mark field as present in the URL */
static inline void mark_field(struct http_parser_url *u, enum http_parser_url_fields field) {
  u->field_set |= (1 << field);
}

/* Parse port number from string */
static int parse_port(const char *buf, size_t len, uint16_t *port) {
  uint32_t val = 0;
  size_t i;

  if (UNLIKELY(len == 0 || len > 5)) {
    return -1;
  }

  for (i = 0; i < len; i++) {
    if (UNLIKELY(!is_digit(buf[i]))) {
      return -1;
    }
    val = val * 10 + (buf[i] - '0');
    if (UNLIKELY(val > 65535)) {
      return -1;
    }
  }

  *port = (uint16_t)val;
  return 0;
}

/* Helper to finalize host field and extract port if present */
static inline void finalize_host_with_port(struct http_parser_url *u,
                                           const char *buf,
                                           size_t field_start,
                                           size_t end_pos,
                                           size_t port_start,
                                           int found_colon) {
  /* Validate port_start is after field_start and before end_pos */
  if (found_colon && port_start > field_start && port_start < end_pos) {
    uint16_t port_val;
    size_t port_len = end_pos - port_start;
    if (parse_port(buf + port_start, port_len, &port_val) == 0) {
      u->port = port_val;
      u->field_data[UF_HOST].off = field_start;
      u->field_data[UF_HOST].len = port_start - field_start - 1; /* -1 for : */
      mark_field(u, UF_PORT);
      u->field_data[UF_PORT].off = port_start;
      u->field_data[UF_PORT].len = port_len;
    } else {
      /* Invalid port, treat whole thing as host */
      u->field_data[UF_HOST].off = field_start;
      u->field_data[UF_HOST].len = end_pos - field_start;
    }
  } else {
    /* No port, just write host */
    u->field_data[UF_HOST].off = field_start;
    u->field_data[UF_HOST].len = end_pos - field_start;
  }
}

/* Initialize all http_parser_url members to 0 - avoid memset overhead */
void http_parser_url_init(struct http_parser_url *u) {
  size_t i;
  u->field_set = 0;
  u->port = 0;
  /* Initialize field_data array explicitly to avoid uninitialized memory */
  for (i = 0; i < UF_MAX; i++) {
    u->field_data[i].off = 0;
    u->field_data[i].len = 0;
  }
}

/* Parse a URL; return nonzero on failure */
int http_parser_parse_url(const char *buf, size_t buflen,
                          int is_connect,
                          struct http_parser_url *u) {
  enum state state;
  enum http_parser_url_fields field = UF_MAX;
  size_t field_start = 0;
  size_t i;
  unsigned char ch;
  size_t port_start = 0; /* Track port position during host parsing */
  int found_colon = 0;   /* Flag to track if we found : in host */
  int bracket_depth = 0; /* Track IPv6 bracket depth; negative = malformed (extra ]) */

  /* Initialize the URL structure - optimized to avoid memset */
  http_parser_url_init(u);

  /* Handle empty URLs */
  if (UNLIKELY(buflen == 0)) {
    return 1;
  }

  /* Set initial state based on request type */
  if (is_connect) {
    /* CONNECT requests expect authority form (host:port) */
    state = s_server_start;
    field = UF_HOST;
    field_start = 0;
    mark_field(u, field);
  } else {
    state = s_start;
  }

  /* Optimized DFA-based parsing loop with hybrid approach */
  for (i = 0; i < buflen; i++) {
    ch = buf[i];

    /* Fast path: use DFA table for simple state transitions */
    if (LIKELY(state == s_schema || state == s_path || state == s_query || state == s_fragment)) {
      enum state next_state = url_state_table[state][char_class_table[ch]];

      if (LIKELY(next_state == STAY)) {
        /* Stay in current state - common case, continue immediately */
        continue;
      }

      if (UNLIKELY(next_state == s_dead)) {
        return 1;
      }

      /* Handle state exit actions */
      if (state == s_schema && next_state == s_schema_slash) {
        /* End of schema - write field data */
        u->field_data[field].off = field_start;
        u->field_data[field].len = i - field_start;
        state = next_state;
        continue;
      }

      if (state == s_path && next_state == s_query_or_fragment) {
        /* Path ended */
        u->field_data[field].off = field_start;
        u->field_data[field].len = i - field_start;
        state = next_state;
        i--; /* Re-process this character */
        continue;
      }

      if (state == s_query && next_state == s_fragment) {
        /* Query to fragment transition */
        u->field_data[field].off = field_start;
        u->field_data[field].len = i - field_start;
        field = UF_FRAGMENT;
        field_start = i + 1;
        mark_field(u, field);
        state = next_state;
        continue;
      }

      /* For other transitions from simple states, update state and fall through */
      state = next_state;
    }

    /* Handle complex states and state entry actions */
    switch (state) {
      case s_start:
        if (ch == '/' || ch == '*') {
          /* Relative URL starting with path */
          state = s_path;
          field = UF_PATH;
          field_start = i;
          mark_field(u, field);
        } else if (LIKELY(is_alpha(ch))) {
          /* Absolute URL with schema */
          state = s_schema;
          field = UF_SCHEMA;
          field_start = i;
          mark_field(u, field);
        } else {
          return 1;
        }
        break;

      case s_schema_slash:
        if (LIKELY(ch == '/')) {
          state = s_schema_slash_slash;
        } else {
          return 1;
        }
        break;

      case s_schema_slash_slash:
        if (LIKELY(ch == '/')) {
          state = s_server_start;
        } else {
          return 1;
        }
        break;

      case s_server_start:
        field = UF_HOST;
        field_start = i;
        mark_field(u, field);
        state = s_server;
        found_colon = 0;
        port_start = 0;
        bracket_depth = 0;
        /* Fall through to s_server */
        /* FALLTHROUGH */

      case s_server:
      case s_server_with_at:
        if (ch == '/') {
          finalize_host_with_port(u, buf, field_start, i, port_start, found_colon);
          field = UF_PATH;
          field_start = i;
          mark_field(u, field);
          state = s_path;
        } else if (ch == '?') {
          finalize_host_with_port(u, buf, field_start, i, port_start, found_colon);
          field = UF_QUERY;
          field_start = i + 1;
          mark_field(u, field);
          state = s_query;
        } else if (ch == '@') {
          if (UNLIKELY(state == s_server_with_at)) {
            return 1;
          }
          if (field == UF_HOST) {
            u->field_data[UF_USERINFO].off = field_start;
            u->field_data[UF_USERINFO].len = i - field_start;
            mark_field(u, UF_USERINFO);
            u->field_set &= ~(1 << UF_HOST);
          }
          state = s_server_with_at;
          field_start = i + 1;
          field = UF_HOST;
          mark_field(u, field);
          found_colon = 0;
          port_start = 0;
          bracket_depth = 0;
        } else if (ch == '[') {
          bracket_depth++;
        } else if (ch == ']') {
          bracket_depth--;
          if (UNLIKELY(bracket_depth < 0)) {
            return 1;
          }
        } else if (ch == ':') {
          if (bracket_depth == 0 && !found_colon) {
            found_colon = 1;
            port_start = i + 1;
          }
        } else if (!is_userinfo_char(ch)) {
          return 1;
        }
        break;

      case s_query_or_fragment:
        if (ch == '?') {
          field = UF_QUERY;
          field_start = i + 1;
          mark_field(u, field);
          state = s_query;
        } else if (ch == '#') {
          field = UF_FRAGMENT;
          field_start = i + 1;
          mark_field(u, field);
          state = s_fragment;
        } else {
          return 1;
        }
        break;

      default:
        break;
    }
  }

  /* Handle final field */
  if (field != UF_MAX) {
    if (field == UF_HOST) {
      /* Handle inline port parsing for final host field */
      finalize_host_with_port(u, buf, field_start, i, port_start, found_colon);
    } else {
      u->field_data[field].off = field_start;
      u->field_data[field].len = i - field_start;
    }
  }

  return 0; /* Success */
}
