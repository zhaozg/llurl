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
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS AND TYPE DEFINITIONS
 * ============================================================================ */

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

/* ============================================================================
 * CHARACTER CLASSIFICATION LOOKUP TABLES
 * ============================================================================ */

/* Lookup table for alpha characters (a-z, A-Z) - eliminates branching */
static const unsigned char alpha_char[256] = {
/*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
        0,       1,       1,       1,       1,       1,       1,       1,
/*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
        1,       1,       1,       1,       1,       1,       1,       1,
/*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
        1,       1,       1,       1,       1,       1,       1,       1,
/*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
        1,       1,       1,       0,       0,       0,       0,       0,
/*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
        0,       1,       1,       1,       1,       1,       1,       1,
/* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
        1,       1,       1,       1,       1,       1,       1,       1,
/* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
        1,       1,       1,       1,       1,       1,       1,       1,
/* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
        1,       1,       1,       0,       0,       0,       0,       0,
/* 128-255: Extended ASCII - all invalid */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/* Lookup table for hex characters (0-9, a-f, A-F) - eliminates branching */
static const unsigned char hex_char[256] = {
/*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
        1,       1,       1,       1,       1,       1,       1,       1,
/*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
        1,       1,       0,       0,       0,       0,       0,       0,
/*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
        0,       1,       1,       1,       1,       1,       1,       0,
/*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
        0,       1,       1,       1,       1,       1,       1,       0,
/* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
        0,       0,       0,       0,       0,       0,       0,       0,
/* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
        0,       0,       0,       0,       0,       0,       0,       0,
/* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
        0,       0,       0,       0,       0,       0,       0,       0,
/* 128-255: Extended ASCII - all invalid */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

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

/* ============================================================================
 * BRANCH PREDICTION HINTS AND OPTIMIZATION MACROS
 * ============================================================================ */

/* Branch prediction hints for performance */
#if defined(__GNUC__) || defined(__clang__)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

/* Character classification macros - now using lookup tables for branchless operation */
#define IS_ALPHA(c) (alpha_char[(unsigned char)(c)])
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_HEX(c) (hex_char[(unsigned char)(c)])
#define IS_ALPHANUM(c) (IS_ALPHA(c) || IS_DIGIT(c))

/* ============================================================================
 * DFA STATE TRANSITION TABLE
 * ============================================================================ */

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

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/* Check if character is alpha */
static inline int is_alpha(unsigned char ch) {
  return IS_ALPHA(ch);
}

/* Check if character is valid in userinfo (user:pass@host) */
static inline int is_userinfo_char(unsigned char ch) {
  return userinfo_char[ch];
}

/* Mark field as present in the URL */
static inline void mark_field(struct http_parser_url *u, enum http_parser_url_fields field) {
  u->field_set |= (1 << field);
}

/* Initialize URL structure - public API function */
void http_parser_url_init(struct http_parser_url *u) {
  memset(u, 0, sizeof(*u));
}

/* Parse port number from string */
static int parse_port(const char *buf, size_t len, uint16_t *port) {
  /* 优化：直接用内联数值转换，减少循环和分支 */
  uint32_t val = 0;
  size_t i = 0;
  if (UNLIKELY(len == 0 || len > 5)) {
    return -1;
  }
  while (i < len) {
    unsigned char c = buf[i];
    if (c < '0' || c > '9') {
      return -1;
    }
    val = val * 10 + (c - '0');
    if (val > 65535) {
      return -1;
    }
    i++;
  }
  *port = (uint16_t)val;
  return 0;
}

/* Helper to finalize host field and extract port if present */
static inline int finalize_host_with_port(struct http_parser_url *u,
                                           const char *buf,
                                           size_t field_start,
                                           size_t end_pos,
                                           size_t port_start,
                                           int found_colon) {
  size_t host_off = field_start;
  size_t host_len = (found_colon && port_start > field_start && port_start < end_pos)
                      ? port_start - field_start - 1
                      : end_pos - field_start;

  // 检查是否为 IPv6 地址（以 [ 开头，] 在 host 内部且在 port 前）
  if (host_len >= 2 && buf[host_off] == '[') {
    // 查找最后一个 ']'，而不是假定 host_off + host_len - 1
    size_t end = host_off + host_len - 1;
    size_t last_bracket = 0;
    int found_bracket = 0;
    for (size_t k = host_off; k <= end; ++k) {
      if (buf[k] == ']') { last_bracket = k; found_bracket = 1; }
    }
    if (!found_bracket) {
      // IPv6 host 缺少闭合 ]
      return 0;
    }
    if (last_bracket > host_off) {
      size_t after_bracket = last_bracket + 1;
      // 检查 ] 后是否有 :port
      if (after_bracket < end && buf[after_bracket] == ':') {
        uint16_t port_val;
        size_t port_len = end - after_bracket;
        if (parse_port(buf + after_bracket + 1, port_len, &port_val) == 0) {
          u->port = port_val;
          u->field_data[UF_PORT].off = after_bracket + 1;
          u->field_data[UF_PORT].len = port_len;
          mark_field(u, UF_PORT);
        } else {
          // 端口非法
          return 0;
        }
        host_len = last_bracket - host_off - 1;
      } else {
        host_len = last_bracket - host_off - 1;
      }
      host_off += 1;
    }
  }

  if (found_colon && port_start > field_start && port_start < end_pos) {
    uint16_t port_val;
    size_t port_len = end_pos - port_start;
    if (parse_port(buf + port_start, port_len, &port_val) == 0) {
      u->port = port_val;
      u->field_data[UF_HOST].off = host_off;
      u->field_data[UF_HOST].len = host_len;
      mark_field(u, UF_PORT);
      u->field_data[UF_PORT].off = port_start;
      u->field_data[UF_PORT].len = port_len;
    } else {
      // Invalid port, return error
      return 0;
    }
  } else {
    // No port, just write host
    u->field_data[UF_HOST].off = host_off;
    u->field_data[UF_HOST].len = host_len;
  }
  return 1;
}

/* Handle protocol-relative URLs (//host/path) by adding a fake schema */
static int parse_protocol_relative_url(const char *buf, size_t buflen,
                                       int is_connect,
                                       struct http_parser_url *u) {
  const char *fake_schema = "http://";
  size_t schema_len = strlen(fake_schema);
  size_t fake_len = schema_len + buflen - 2;
  char *tmp = (char *)malloc(fake_len + 1);
  if (!tmp) return 1;
  
  strcpy(tmp, fake_schema);
  memcpy(tmp + schema_len, buf + 2, buflen - 2); /* Skip original // */
  tmp[fake_len] = '\0';
  
  struct http_parser_url u2;
  http_parser_url_init(&u2);
  int ret = http_parser_parse_url(tmp, fake_len, is_connect, &u2);
  
  if (ret == 0) {
    for (int f = 0; f < UF_MAX; ++f) {
      if (u2.field_set & (1 << f)) {
        u->field_set |= (1 << f);
        if (f == UF_SCHEMA) {
          /* For input without schema, don't set protocol field */
          continue;
        } else {
          u->field_data[f].off = u2.field_data[f].off >= schema_len ? u2.field_data[f].off - schema_len + 2 : 0;
          u->field_data[f].len = u2.field_data[f].len;
        }
      }
    }
    u->port = u2.port;
  }
  free(tmp);
  return ret;
}

/* Validate percent-encoding in host field, allowing IPv6 zone IDs */
static int validate_host_percent_encoding(const char *buf, size_t off, size_t len) {
  /* Skip validation if this appears to be an IPv6 address with zone ID */
  if (strchr(buf + off, '%') != NULL && strchr(buf + off, ':') != NULL) {
    return 1; /* Valid - IPv6 with zone ID */
  }
  
  /* Validate percent encoding */
  size_t j = 0;
  while (j < len) {
    if (buf[off + j] == '%') {
      if (j + 2 >= len) {
        return 0; /* Invalid - incomplete percent encoding */
      }
      if (!IS_HEX(buf[off + j + 1]) || !IS_HEX(buf[off + j + 2])) {
        return 0; /* Invalid - non-hex characters after % */
      }
      j += 2;
    }
    j++;
  }
  return 1; /* Valid */
}

/* ============================================================================
 * MAIN URL PARSING FUNCTION
 * ============================================================================ */

/* Parse a URL; return nonzero on failure */
/* 线程安全说明：本函数无全局状态，结构体独立，适用于多线程环境。 */
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

  /* Handle protocol-relative URLs (//host/path) */
  if (!is_connect && buflen >= 2 && buf[0] == '/' && buf[1] == '/') {
    return parse_protocol_relative_url(buf, buflen, is_connect, u);
  }

  /* Handle empty URLs */
  if (UNLIKELY(buflen == 0)) {
    return 1;
  }

  /* Optimize: Detect initial state early to reduce branching */
  if (is_connect) {
    /* CONNECT requests expect authority form (host:port) */
    state = s_server_start;
    field = UF_HOST;
    field_start = 0;
    mark_field(u, field);
  } else {
    /* Fast initial state detection to avoid unnecessary transitions */
    ch = (unsigned char)buf[0];
    if (ch == '/') {
      /* Relative URL - start directly at path */
      state = s_path;
      field = UF_PATH;
      field_start = 0;
      mark_field(u, field);
    } else if (ch == '*') {
      /* Asterisk form - special path */
      state = s_path;
      field = UF_PATH;
      field_start = 0;
      mark_field(u, field);
    } else if (LIKELY(is_alpha(ch))) {
      /* Absolute URL with schema */
      state = s_schema;
      field = UF_SCHEMA;
      field_start = 0;
      mark_field(u, field);
    } else {
      /* Invalid start character */
      return 1;
    }
  }

  /* Optimized DFA-based parsing loop with batch processing */
  for (i = 0; i < buflen; i++) {
    ch = (unsigned char)buf[i];

    /* Fast batch processing for path state - scan ahead to find delimiters */
    if (state == s_path) {
      /* Look ahead to find ? or # to batch process the path */
      size_t j = i;
      while (j < buflen) {
        unsigned char c = (unsigned char)buf[j];
        if (c == '?' || c == '#') {
          break;
        }
        if (UNLIKELY(char_class_table[c] == cc_invalid)) {
          return 1;
        }
        j++;
      }
      
      if (j > i) {
        if (j < buflen) {
          /* Found delimiter, move to just before it */
          i = j - 1;
          /* Will process delimiter in next iteration */
          continue;
        } else {
          /* Reached end of buffer, no delimiter found */
          /* Path continues to end, set i = buflen so final field handling works correctly */
          i = buflen;
          break;
        }
      }
      
      /* Handle delimiter at current position */
      ch = (unsigned char)buf[i];
      if (ch == '?' || ch == '#') {
        /* Save path and transition to s_query_or_fragment state */
        /* This state will be handled by the switch statement below */
        u->field_data[field].off = field_start;
        u->field_data[field].len = i - field_start;
        state = s_query_or_fragment;
        i--;
        continue;
      }
      continue;
    }
    
    /* Fast batch processing for query state - use memchr for hardware acceleration */
    if (state == s_query) {
      /* Use memchr to find '#' delimiter - hardware optimized */
      const char *hash_pos = memchr(buf + i, '#', buflen - i);
      
      if (hash_pos) {
        /* Found #, validate characters between current position and # */
        size_t hash_idx = hash_pos - buf;
        for (size_t j = i; j < hash_idx; j++) {
          if (UNLIKELY(char_class_table[(unsigned char)buf[j]] == cc_invalid)) {
            return 1;
          }
        }
        
        /* Save query field and transition to fragment */
        u->field_data[field].off = field_start;
        u->field_data[field].len = hash_idx - field_start;
        field = UF_FRAGMENT;
        field_start = hash_idx + 1;
        mark_field(u, field);
        state = s_fragment;
        i = hash_idx;
        continue;
      } else {
        /* No # found, validate all remaining characters */
        for (size_t j = i; j < buflen; j++) {
          if (UNLIKELY(char_class_table[(unsigned char)buf[j]] == cc_invalid)) {
            return 1;
          }
        }
        /* Query extends to end */
        i = buflen;
        break;
      }
    }
    
    /* Fast batch processing for fragment state - validate and consume to end */
    if (state == s_fragment) {
      size_t j = i;
      while (j < buflen) {
        if (UNLIKELY(char_class_table[(unsigned char)buf[j]] == cc_invalid)) {
          return 1;
        }
        j++;
      }
      /* Fragment is valid, skip to end */
      i = buflen - 1;
      continue;
    }

    /* Schema state with fast path */
    if (state == s_schema) {
      enum state next_state = url_state_table[state][char_class_table[ch]];

      if (LIKELY(next_state == STAY)) {
        /* Stay in current state - common case, continue immediately */
        continue;
      }

      if (UNLIKELY(next_state == s_dead)) {
        return 1;
      }

      /* Handle state exit actions */
      if (next_state == s_schema_slash) {
        /* End of schema - write field data */
        u->field_data[field].off = field_start;
        u->field_data[field].len = i - field_start;
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
        // 新增：如果下一个字符不是 host 的合法起始字符，直接返回错误
        if (i >= buflen || buf[i] == '/' || buf[i] == '?' || buf[i] == '#') {
          return 1;
        }
        /* Fall through to s_server */
        /* FALLTHROUGH */

      case s_server:
      case s_server_with_at: {
        /* Batch scanning optimization for server state */
        /* When not in bracket and seeing regular characters, scan ahead to next delimiter */
        if (bracket_depth == 0 && ch != '@' && ch != '[' && ch != ':' && 
            ch != '/' && ch != '?' && ch != '#' && is_userinfo_char(ch)) {
          /* Fast scan to next delimiter */
          size_t j = i + 1;
          while (j < buflen) {
            unsigned char c = (unsigned char)buf[j];
            if (c == '@' || c == '[' || c == ':' || c == '/' || c == '?' || c == '#') {
              break;
            }
            if (!is_userinfo_char(c)) {
              return 1;
            }
            j++;
          }
          /* Skip ahead if we found multiple valid characters */
          if (j > i + 1) {
            i = j - 1;
            continue;
          }
        }
        
        /* 优化分支结构，减少循环内条件判断 */
        if (ch == '/') {
          if (!finalize_host_with_port(u, buf, field_start, i, port_start, found_colon)) {
            return 1;
          }
          field = UF_PATH;
          field_start = i;
          mark_field(u, field);
          state = s_path;
          break;
        }
        if (ch == '?') {
          if (!finalize_host_with_port(u, buf, field_start, i, port_start, found_colon)) {
            return 1;
          }
          field = UF_QUERY;
          field_start = i + 1;
          mark_field(u, field);
          state = s_query;
          break;
        }
        if (ch == '@') {
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
          break;
        }
        if (ch == '[') {
          /* IPv6 fast path - batch process the entire IPv6 address */
          bracket_depth = 1;
          i++;
          
          /* Scan to closing bracket using memchr for speed */
          const char *bracket_end = memchr(buf + i, ']', buflen - i);
          
          if (!bracket_end) {
            /* No closing bracket found */
            return 1;
          }
          
          size_t bracket_pos = bracket_end - buf;
          
          /* Validate IPv6 characters between [ and ] */
          for (size_t j = i; j < bracket_pos; j++) {
            unsigned char c = (unsigned char)buf[j];
            /* IPv6 chars: 0-9, a-f, A-F, :, . or % for zone ID */
            if (c == '%') {
              /* Zone ID detected - skip to closing bracket */
              j++;
              while (j < bracket_pos) j++;
              break;
            }
            if (!IS_HEX(c) && c != ':' && c != '.') {
              return 1;
            }
          }
          
          /* Move to closing bracket */
          i = bracket_pos;
          bracket_depth = 0;
          break;
        }
        if (ch == ']') {
          bracket_depth--;
          if (UNLIKELY(bracket_depth < 0)) {
            return 1;
          }
          break;
        }
        if (ch == ':') {
          if (bracket_depth == 0 && !found_colon) {
            found_colon = 1;
            port_start = i + 1;
          }
          break;
        }
        /* 用查表方式判断合法 userinfo 字符 */
        if (!is_userinfo_char(ch)) {
          return 1;
        }
        break;
      }

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
      if (!finalize_host_with_port(u, buf, field_start, i, port_start, found_colon)) {
        return 1;
      }
    } else {
      /* 优化：仅对可能多次赋值的字段允许覆盖，其余字段只在未设置时写入 */
      if (field == UF_PATH || field == UF_QUERY || field == UF_FRAGMENT) {
        u->field_data[field].off = field_start;
        u->field_data[field].len = i - field_start;
      } else if (!(u->field_set & (1 << field))) {
        u->field_data[field].off = field_start;
        u->field_data[field].len = i - field_start;
      }
    }
  }

  /* CONNECT 模式下，必须严格是 host[:port]，不能有 path/query/fragment */
  if (is_connect) {
    if (state != s_server && state != s_server_with_at) {
      return 1;
    }
    // 检查 host 字段后是否还有多余字符（如 path/query/fragment）
    if (i < buflen) {
      return 1;
    }
    // 必须包含端口
    if (!(u->field_set & (1 << UF_PORT))) {
      return 1;
    }
  }

  /* --- ENHANCEMENT: Reject schema with no host (e.g. http://) --- */
  if (!is_connect && (u->field_set & (1 << UF_SCHEMA)) && !(u->field_set & (1 << UF_HOST))) {
    return 1;
  }

  /* --- ENHANCEMENT: Reject invalid percent-encoding in host, but allow IPv6 zone id --- */
  if (u->field_set & (1 << UF_HOST)) {
    if (!validate_host_percent_encoding(buf, u->field_data[UF_HOST].off, u->field_data[UF_HOST].len)) {
      return 1;
    }
  }

  return 0; /* Success */
}
