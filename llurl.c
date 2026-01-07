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
  s_fragment
};

/* Character classification lookup tables for performance */

/* Check if character is alphanumeric */
static const unsigned char normal_url_char[256] = {
/*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
        0,       1,       1,       0,       1,       1,       1,       1,
/*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
        1,       1,       1,       1,       1,       1,       1,       1,
/*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
        1,       1,       1,       1,       1,       1,       1,       1,
/*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
        1,       1,       1,       1,       1,       1,       1,       0,
/*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
        1,       1,       1,       1,       1,       1,       1,       1,
/*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
        1,       1,       1,       1,       1,       1,       1,       1,
/*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
        1,       1,       1,       1,       1,       1,       1,       1,
/*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
        1,       1,       1,       1,       1,       1,       1,       1,
/*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
        1,       1,       1,       1,       1,       1,       1,       1,
/* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
        1,       1,       1,       1,       1,       1,       1,       1,
/* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
        1,       1,       1,       1,       1,       1,       1,       1,
/* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
        1,       1,       1,       1,       1,       1,       1,       0,
/* 128-255: Extended ASCII - all invalid */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/* Check if character is alpha */
static inline int is_alpha(unsigned char ch) {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

/* Check if character is digit */
static inline int is_digit(unsigned char ch) {
  return ch >= '0' && ch <= '9';
}

/* Check if character is alphanumeric */
static inline int is_alphanum(unsigned char ch) {
  return is_alpha(ch) || is_digit(ch);
}

/* Check if character is valid in userinfo (user:pass@host) */
static inline int is_userinfo_char(unsigned char ch) {
  /* ALPHANUM + MARK + %:;&=+$, */
  return is_alphanum(ch) ||
         ch == '-' || ch == '_' || ch == '.' || ch == '!' || ch == '~' ||
         ch == '*' || ch == '\'' || ch == '(' || ch == ')' ||
         ch == '%' || ch == ';' || ch == ':' || ch == '&' || ch == '=' ||
         ch == '+' || ch == '$' || ch == ',';
}

/* Check if character is valid in URL path/query/fragment */
static inline int is_url_char(unsigned char ch) {
  return normal_url_char[ch];
}

/* Mark field as present in the URL */
static inline void mark_field(struct http_parser_url *u, enum http_parser_url_fields field) {
  u->field_set |= (1 << field);
}

/* Parse port number from string */
static int parse_port(const char *buf, size_t len, uint16_t *port) {
  uint32_t val = 0;
  size_t i;
  
  if (len == 0 || len > 5) {
    return -1;
  }
  
  for (i = 0; i < len; i++) {
    if (!is_digit(buf[i])) {
      return -1;
    }
    val = val * 10 + (buf[i] - '0');
    if (val > 65535) {
      return -1;
    }
  }
  
  *port = (uint16_t)val;
  return 0;
}

/* Initialize all http_parser_url members to 0 */
void http_parser_url_init(struct http_parser_url *u) {
  memset(u, 0, sizeof(*u));
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
  
  /* Initialize the URL structure */
  http_parser_url_init(u);
  
  /* Handle empty URLs */
  if (buflen == 0) {
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
  
  for (i = 0; i < buflen; i++) {
    ch = buf[i];
    
    switch (state) {
      case s_start:
        if (ch == '/' || ch == '*') {
          /* Relative URL starting with path */
          state = s_path;
          field = UF_PATH;
          field_start = i;
          mark_field(u, field);
        } else if (is_alpha(ch)) {
          /* Absolute URL with schema */
          state = s_schema;
          field = UF_SCHEMA;
          field_start = i;
          mark_field(u, field);
        } else {
          return 1; /* Invalid start character */
        }
        break;
      
      case s_schema:
        if (is_alphanum(ch) || ch == '+' || ch == '-' || ch == '.') {
          /* Continue schema */
        } else if (ch == ':') {
          /* End of schema */
          u->field_data[field].off = field_start;
          u->field_data[field].len = i - field_start;
          state = s_schema_slash;
        } else {
          return 1; /* Invalid character in schema */
        }
        break;
      
      case s_schema_slash:
        if (ch == '/') {
          state = s_schema_slash_slash;
        } else {
          return 1; /* Expected / after schema: */
        }
        break;
      
      case s_schema_slash_slash:
        if (ch == '/') {
          state = s_server_start;
        } else {
          return 1; /* Expected // after schema: */
        }
        break;
      
      case s_server_start:
        field = UF_HOST;
        field_start = i;
        mark_field(u, field);
        state = s_server;
        /* Fall through to s_server */
        /* FALLTHROUGH */
        
      case s_server:
      case s_server_with_at:
        if (ch == '/') {
          /* End of host, start of path */
          u->field_data[field].off = field_start;
          u->field_data[field].len = i - field_start;
          
          /* Check for port in host */
          if (u->field_data[UF_HOST].len > 0) {
            const char *host_start = buf + u->field_data[UF_HOST].off;
            size_t host_len = u->field_data[UF_HOST].len;
            size_t j;
            
            /* Look for : separating host and port */
            for (j = host_len; j > 0; j--) {
              if (host_start[j-1] == ':') {
                /* Found port separator */
                if (j < host_len) {
                  uint16_t port_val;
                  if (parse_port(host_start + j, host_len - j, &port_val) == 0) {
                    u->port = port_val;
                    u->field_data[UF_HOST].len = j - 1;
                    mark_field(u, UF_PORT);
                    u->field_data[UF_PORT].off = u->field_data[UF_HOST].off + j;
                    u->field_data[UF_PORT].len = host_len - j;
                  }
                }
                break;
              } else if (host_start[j-1] == ']') {
                /* IPv6 address, stop looking for port */
                break;
              }
            }
          }
          
          field = UF_PATH;
          field_start = i;
          mark_field(u, field);
          state = s_path;
        } else if (ch == '?') {
          /* End of host, start of query */
          u->field_data[field].off = field_start;
          u->field_data[field].len = i - field_start;
          
          field = UF_QUERY;
          field_start = i + 1;
          mark_field(u, field);
          state = s_query;
        } else if (ch == '@') {
          if (state == s_server_with_at) {
            return 1; /* Double @ not allowed */
          }
          /* Previous content was userinfo */
          if (field == UF_HOST) {
            u->field_data[UF_USERINFO].off = field_start;
            u->field_data[UF_USERINFO].len = i - field_start;
            mark_field(u, UF_USERINFO);
            u->field_set &= ~(1 << UF_HOST); /* Clear host flag */
          }
          state = s_server_with_at;
          field_start = i + 1;
          field = UF_HOST;
          mark_field(u, field);
        } else if (is_userinfo_char(ch) || ch == '[' || ch == ']') {
          /* Valid server character */
        } else {
          return 1; /* Invalid character in server */
        }
        break;
      
      case s_path:
        if (is_url_char(ch)) {
          /* Continue path */
        } else {
          /* End of path */
          u->field_data[field].off = field_start;
          u->field_data[field].len = i - field_start;
          state = s_query_or_fragment;
          i--; /* Re-process this character */
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
          return 1; /* Invalid character */
        }
        break;
      
      case s_query:
        if (is_url_char(ch) || ch == '?') {
          /* Continue query (? is allowed in query string) */
        } else if (ch == '#') {
          /* End of query, start of fragment */
          u->field_data[field].off = field_start;
          u->field_data[field].len = i - field_start;
          
          field = UF_FRAGMENT;
          field_start = i + 1;
          mark_field(u, field);
          state = s_fragment;
        } else {
          return 1; /* Invalid character in query */
        }
        break;
      
      case s_fragment:
        if (is_url_char(ch) || ch == '?' || ch == '#') {
          /* Continue fragment (? and # are allowed) */
        } else {
          return 1; /* Invalid character in fragment */
        }
        break;
      
      default:
        return 1; /* Invalid state */
    }
  }
  
  /* Handle final field */
  if (field != UF_MAX) {
    u->field_data[field].off = field_start;
    u->field_data[field].len = i - field_start;
    
    /* Special handling for host field - extract port if present */
    if (field == UF_HOST && u->field_data[UF_HOST].len > 0) {
      const char *host_start = buf + u->field_data[UF_HOST].off;
      size_t host_len = u->field_data[UF_HOST].len;
      size_t j;
      
      /* Look for : separating host and port */
      for (j = host_len; j > 0; j--) {
        if (host_start[j-1] == ':') {
          /* Found port separator */
          if (j < host_len) {
            uint16_t port_val;
            if (parse_port(host_start + j, host_len - j, &port_val) == 0) {
              u->port = port_val;
              u->field_data[UF_HOST].len = j - 1;
              mark_field(u, UF_PORT);
              u->field_data[UF_PORT].off = u->field_data[UF_HOST].off + j;
              u->field_data[UF_PORT].len = host_len - j;
            }
          }
          break;
        } else if (host_start[j-1] == ']') {
          /* IPv6 address, stop looking for port */
          break;
        }
      }
    }
  }
  
  return 0; /* Success */
}
