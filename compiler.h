#ifndef COMPILER_H_
#define COMPILER_H_

struct string_view {
  char* begin;
  int length;
};

enum TType {
  UNKNOWN_TOK = 0,
  AUTO_TOK,
  BREAK_TOK,
  CASE_TOK,
  CHAR_TOK,
  CONST_TOK,
  CONTINUE_TOK,
  DEFAULT_TOK,
  DO_TOK,
  DOUBLE_TOK,
  ELSE_TOK,
  ENUM_TOK,
  EXTERN_TOK,
  FLOAT_TOK,
  FOR_TOK,
  GOTO_TOK,
  IF_TOK,
  INLINE_TOK,
  INT_TOK,
  LONG_TOK,
  REGISTER_TOK,
  RESTRICT_TOK,
  RETURN_TOK,
  SHORT_TOK,
  SIGNED_TOK,
  SIZEOF_TOK,
  STATIC_TOK,
  STRUCT_TOK,
  SWITCH_TOK,
  TYPEDEF_TOK,
  UNION_TOK,
  UNSIGNED_TOK,
  VOID_TOK,
  VOLATILE_TOK,
  WHILE_TOK,
  _ALIGNAS_TOK,
  _ALIGNOF_TOK,
  _ATOMIC_TOK,
  _BOOL_TOK,
  _COMPLEX_TOK,
  _DECIMAL128_TOK,
  _DECIMAL32_TOK,
  _DECIMAL64_TOK,
  _GENERIC_TOK,
  _IMAGINARY_TOK,
  _NORETURN_TOK,
  _STATIC_ASSERT_TOK,
  _THREAD_LOCAL_TOK,
  PLUS_TOK,
  PLUS_PLUS_TOK,
  MINUS_TOK,
  MINUS_MINUS_TOK,
  STAR_TOK,
  SLASH_TOK,
  MODULUS_TOK,
  AND_TOK,
  AND_AND_TOK,
  VERT_TOK,
  VERT_VERT_TOK,
  BANG_TOK,
  TILDE_TOK,
  CARROT_TOK,
  LSHIFT_TOK,
  RSHIFT_TOK,
  EQUAL_TOK,
  EQUAL_EQUAL_TOK,
  PLUS_EQUAL_TOK,
  MINUS_EQUAL_TOK,
  START_EQUAL_TOK,
  SLASH_EQUAL_TOK,
  MODULUS_EQUAL_TOK,
  AND_EQUAL_TOK,
  VERT_EQUAL_TOK,
  BANG_EQUAL_TOK,
  TILDE_EQUAL_TOK,
  CARROT_EQUAL_TOK,
  LSHIFT_EQUAL_TOK,
  RSHIFT_EQUAL_TOK,
  COMMA_TOK,
  PERIOD_TOK,
  ARROW_TOK,
  LESS_TOK,
  LESS_EQUAL_TOK,
  GREATER_TOK,
  GREATER_EQUAL_TOK,
  QMARK_TOK,
  COLON_TOK,
  SEMICOLON_TOK,
  LPAREN_TOK,
  RPAREN_TOK,
  LBRACE_TOK,
  RBRACE_TOK,
  LSQUARE_TOK,
  LSQUARE_LSQUARE_TOK,
  RSQUARE_TOK,
  RSQUARE_RSQUARE_TOK,
  CHAR_LITERAL_TOK,
  U8_CHAR_LITERAL_TOK,
  U16_CHAR_LITERAL_TOK,
  U32_CHAR_LITERAL_TOK,
  WIDE_CHAR_LITERAL_TOK,
  STR_LITERAL_TOK,
  U8_STR_LITERAL_TOK,
  U16_STR_LITERAL_TOK,
  U32_STR_LITERAL_TOK,
  WIDE_STR_LITERAL_TOK,
  INT_LITERAL_TOK,
  UNSIGNED_LITERAL_TOK,
  LONG_LITERAL_TOK,
  UNSIGNED_LONG_LITERAL_TOK,
  LONG_LONG_LITERAL_TOK,
  UNSIGNED_LONG_LONG_TOK,
  FLOAT_LITERAL_TOK,
  DOUBLE_LITERAL_TOK,
  LONG_DOUBLE_LITERAL_TOK,
  _DECIMAL128_LITERAL_TOK,
  _DECIMAL32_LITERAL_TOK,
  _DECIMAL64_LITERAL_TOK,
  IDENTIFIER_TOK,
  EOF_TOK
};

struct Token {
  char* file;
  int line;
  int position;
  enum TType type;
  struct string_view value;
};

#endif