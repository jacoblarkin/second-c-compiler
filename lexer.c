#include "compiler.h"

#include <ctype.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct OperatorTokenPair {
  enum TType single;
  enum TType equals;
} operator_tokens[128] = {
  [(unsigned char)'+'] = {PLUS_TOK, PLUS_EQUAL_TOK},
  [(unsigned char)'-'] = {MINUS_TOK, MINUS_EQUAL_TOK},
  [(unsigned char)'*'] = {STAR_TOK, STAR_EQUAL_TOK},
  [(unsigned char)'/'] = {SLASH_TOK, SLASH_EQUAL_TOK},
  [(unsigned char)'%'] = {MODULUS_TOK, MODULUS_EQUAL_TOK},
  [(unsigned char)'!'] = {BANG_TOK, BANG_EQUAL_TOK},
  [(unsigned char)'~'] = {TILDE_TOK, TILDE_EQUAL_TOK},
  [(unsigned char)'&'] = {AND_TOK, AND_EQUAL_TOK},
  [(unsigned char)'|'] = {VERT_TOK, VERT_EQUAL_TOK},
  [(unsigned char)'^'] = {CARROT_TOK, CARROT_EQUAL_TOK},
  [(unsigned char)'<'] = {LESS_TOK, LESS_EQUAL_TOK},
  [(unsigned char)'>'] = {GREATER_TOK, GREATER_EQUAL_TOK},
  [(unsigned char)'='] = {EQUAL_TOK, EQUAL_EQUAL_TOK},
};

#define NUM_KEYWORD 59

struct KeywordTokenPair {
  const char* keyword;
  enum TType token;
} keywords[NUM_KEYWORD] = {
  {"alignas", ALIGNAS_TOK},
  {"alignof", ALIGNOF_TOK},
  {"auto", AUTO_TOK},
  {"bool", BOOL_TOK},
  {"break", BREAK_TOK},
  {"case", CASE_TOK},
  {"char", CHAR_TOK},
  {"const", CONST_TOK},
  {"constexpr", CONSTEXPR_TOK},
  {"continue", CONTINUE_TOK},
  {"default", DEFAULT_TOK},
  {"do", DO_TOK},
  {"double", DOUBLE_TOK},
  {"else", ELSE_TOK},
  {"enum", ENUM_TOK},
  {"extern", EXTERN_TOK},
  {"false", FALSE_TOK},
  {"float", FLOAT_TOK},
  {"for", FOR_TOK},
  {"goto", GOTO_TOK},
  {"if", IF_TOK},
  {"inline", INLINE_TOK},
  {"int", INT_TOK},
  {"long", LONG_TOK},
  {"nullptr", NULLPTR_TOK},
  {"register", REGISTER_TOK},
  {"restrict", RESTRICT_TOK},
  {"return", RETURN_TOK},
  {"short", SHORT_TOK},
  {"signed", SIGNED_TOK},
  {"sizeof", SIZEOF_TOK},
  {"static", STATIC_TOK},
  {"static_assert", STATIC_ASSERT_TOK},
  {"struct", STRUCT_TOK},
  {"switch", SWITCH_TOK},
  {"thread_local", THREAD_LOCAL_TOK},
  {"true", TRUE_TOK},
  {"typedef", TYPEDEF_TOK},
  {"typeof", TYPEOF_TOK},
  {"typeof_unequal", TYPEOF_UNEQUAL_TOK},
  {"union", UNION_TOK},
  {"unsigned", UNSIGNED_TOK},
  {"void", VOID_TOK},
  {"volatile", VOLATILE_TOK},
  {"while", WHILE_TOK},
  {"_Alignas", _ALIGNAS_TOK},
  {"_Alignof", _ALIGNOF_TOK},
  {"_Atomic", _ATOMIC_TOK},
  {"_BitInt", _BITINT_TOK},
  {"_Bool", _BOOL_TOK},
  {"_Complex", _COMPLEX_TOK},
  {"_Decimal128", _DECIMAL128_TOK},
  {"_Decimal32", _DECIMAL32_TOK},
  {"_Decimal64", _DECIMAL64_TOK},
  {"_Generic", _GENERIC_TOK},
  {"_Imaginary", _IMAGINARY_TOK},
  {"_Noreturn", _NORETURN_TOK},
  {"_Static_assert", _STATIC_ASSERT_TOK},
  {"_Thread_local", _THREAD_LOCAL_TOK}
};

struct lexer {
  char* current_file;
  char* buffer;
  size_t buffer_size;
  size_t buffer_loc;
  int line;
  int position;
  struct lexer* next;
};

PreprocessorTable prepTable;
MacroTable macroTable;
struct lexer* lexer = NULL;

jmp_buf jbuf;

struct string_view curr_def = {0};
int curr_def_pos = 0;
bool in_def = false;

static inline char previous()
{
  return lexer->buffer[lexer->buffer_loc - 1];
}

static inline char peek()
{
  return lexer->buffer[lexer->buffer_loc];
}

static inline bool isAtEnd()
{
  return peek() == '\0' || lexer->buffer_loc == lexer->buffer_size;
}

static inline char advance()
{
  char ret = peek();
  lexer->buffer_loc++;
  return ret;
}

static inline bool match(char expected)
{
  if(isAtEnd()) return false;
  if(peek() != expected) return false;
  advance();
  return true;
}

static inline bool matchOne(const char* chars)
{
  if(isAtEnd()) return false;
  while(*chars) {
    if(peek() == *chars++) {
      advance();
      return true;
    }
  }
  return false;
}

static inline bool matchDigit()
{
  if(isAtEnd()) return false;
  if(!isdigit(peek())) return false;
  advance();
  return true;
}

static inline bool matchXDigit()
{
  if(isAtEnd()) return false;
  if(!isxdigit(peek())) return false;
  advance();
  return true;
}

static inline bool matchAlpha()
{
  if(isAtEnd()) return false;
  if(!isalpha(peek())) return false;
  advance();
  return true;
}

static inline bool matchAlNum()
{
  if(isAtEnd()) return false;
  if(!isalnum(peek())) return false;
  advance();
  return true;
}

static inline bool matchSpace()
{
  if(isAtEnd()) return false;
  if(!isspace(peek())) return false;
  advance();
  return true;
}

static inline char peekNext()
{
  if(isAtEnd()) return '\0';
  return lexer->buffer[lexer->buffer_loc + 1];
}

_Noreturn
static void error(const char* msg, ...)
{
  fprintf(stderr, "Lexing Error (%s - line: %i, column: %i): ", 
          lexer->current_file, lexer->line, lexer->position);
  va_list ap;
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
  fputc('\n', stderr);
  abort();
}

static void warning(const char* msg, ...) {
  fprintf(stderr, "Lexing warning (%s - line %i, column %i): ",
          lexer->current_file, lexer->line, lexer->position);
  va_list ap;
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
  fputc('\n', stderr);
}

static struct lexer lexer_create(const char* filename) {
  return (struct lexer) {
    .current_file = strdup(filename),
    .buffer = NULL,
    .buffer_size = 0,
    .buffer_loc = 0,
    .line = 1,
    .position = 1,
    .next = NULL
  };
}

static void lexer_init(struct lexer* new_lexer)
{
  FILE* file = fopen(new_lexer->current_file, "r");
  if(!file) {
    error("Did not find file %s.\n", new_lexer->current_file);
  }

  int err = fseek(file, 0L, SEEK_END);
  if(err) {
    fclose(file);
    error("Error seeking end of file %s.\n", new_lexer->current_file);
  }
  long ftell_ret = ftell(file);
  if(ftell_ret == -1L) {
    fclose(file);
    error("Error getting end position of file %s from ftell.\n", new_lexer->current_file);
  }
  size_t size = (size_t)ftell_ret;
  rewind(file);

  new_lexer->buffer = malloc((size+1)*sizeof(char));
  if(!new_lexer->buffer) {
    fclose(file);
    error("Failed to allocate enough memory to read in file. Need %zu bytes.\n", size);
  }

  size_t bytes_read = fread(new_lexer->buffer, sizeof(char), size, file);
  int eof = feof(file);
  int ferr = ferror(file);
  fclose(file);
  if(bytes_read != size) {
    if(eof) {
      error("Unexpected EOF %f%% of the way through (%zu of %zu bytes).\n", 100.0 * ((double)bytes_read / (double)size), bytes_read, size);
    } else if (ferr){
      error("Error reading in %s.\n", new_lexer->current_file);
    } else {
      error("Read in fewer bytes (%zu) than expected (%zu). No error or EOF reported.\n", bytes_read, size);
    }
  }
  new_lexer->buffer[size] = '\0';
  new_lexer->buffer_size = size;
  new_lexer->buffer_loc = 0;

  prepTable = preproc_table_create();
  macroTable = macro_table_create();
}


static struct lexer lexer_push(const char* filename) {
  struct lexer new_lexer = lexer_create(filename);
  bool begining = !lexer;
  if(begining) lexer = &new_lexer;
  lexer_init(&new_lexer);
  struct lexer* tmp = malloc(sizeof(struct lexer));
  *tmp = new_lexer;
  tmp->next = begining ? NULL : lexer;
  lexer = tmp;
  return new_lexer;
}

static struct lexer lexer_push_text(struct string_view text)
{
  struct lexer new_lexer = lexer_create(lexer->current_file);
  new_lexer.buffer = strviewtostr(text);
  new_lexer.buffer_size = text.length;
  new_lexer.line = lexer->line;
  new_lexer.position = lexer->position;
  new_lexer.next = lexer;
  struct lexer* tmp = malloc(sizeof(struct lexer));
  *tmp = new_lexer;
  lexer = tmp;
  return new_lexer;
}

static inline struct lexer lexer_push_str(char* str)
{
  struct lexer new_lexer = lexer_create(lexer->current_file);
  new_lexer.buffer = str;
  new_lexer.buffer_size = strlen(str);
  new_lexer.line = lexer->line;
  new_lexer.position = lexer->position;
  new_lexer.next = lexer;
  struct lexer* tmp = malloc(sizeof(struct lexer));
  *tmp = new_lexer;
  lexer = tmp;
  return new_lexer;
}

static inline void lexer_pop()
{
  struct lexer* next = lexer->next;
  free(lexer);
  lexer = next;
}

void setup_lexer(const char* filename) {
  lexer_push(filename);
}

void lex_string(struct string_view* value)
{
  while(!match('"') && !isAtEnd()) {
    if(peek() == '\\' && peekNext() == '"') {
      advance();
      value->length++;
    }
    if(peek() == '\n') {
      lexer->line++;
      lexer->position=1;
    }
    advance();
    value->length++;
  }

  value->length++;
}

enum TType lex_number(struct string_view* value)
{
  bool dot = match('.');
  bool exp = false;
  bool hex = match('0') && matchOne("xX");
  bool bin = !hex && match('0') && matchOne("bB");
  bool oct = !hex && !bin && match('0');
  int u_suffix = 0;
  int l_suffix = 0;
  int f_suffix = 0;
  int d_suffix = 0;

  if(hex || bin) value->length++;

  // Handle binary literals separately since they can only accept 0 or 1
  if(bin) {
    while(match('0') || match('1')) {
      value->length++;
    }
    goto suffix_lexing;
  }

  // Integer part
  while(matchDigit() || (hex && matchXDigit()) || match('\'')) {
    value->length++;
    if(oct && (previous() == '8' || previous() == '9'))
      error("Expected only digits 0-7 in octal literal.");
  }

  if(oct) goto suffix_lexing; // No octal floats

  // Fractional part
  if(!dot && match('.')) {
    dot = true;
    value->length++;
    while(matchDigit() || (hex && matchXDigit()) || match('\'')) {
      value->length++;
    }
  }

  // Exponent
  if((!hex && !oct && (match('e') || match('E')))
     || (hex && (match('p') || match('P')))) {
    exp = true;
    value->length++;
    if(match('+') || match('-')) {
      value->length++;
    }
    while(matchDigit() || (hex && matchXDigit()) || match('\'')) {
      value->length++;
    } 
  }
 
suffix_lexing:

  if(matchOne("uU")) {
    u_suffix++;
    value->length++;
    goto suffix_lexing;
  } else if(matchOne("lL")) {
    l_suffix++;
    value->length++;
    goto suffix_lexing;
  } else if(matchOne("fF")) {
    f_suffix++;
    value->length++;
    goto suffix_lexing;
  } else if(matchOne("dD")) {
    d_suffix++;
    value->length++;
    goto suffix_lexing;
  }

  if(!isspace(peek()) && !ispunct(peek()) && !iscntrl(peek()))
    error("Unrecognized token. Expected number.");

  if(u_suffix) {
    if(u_suffix > 1) error("Expected at most 1 unsigned suffix.");
    if(dot || exp) error("Unsigned suffix not valid for floating point literals.");   
    if(u_suffix && f_suffix) error("Incompatible suffixes 'u' and 'f'.");
    if(u_suffix && d_suffix) error("Incompatible suffixes 'u' and 'd'.");
    switch(l_suffix) {
      case 0: 
        return hex ? UNSIGNED_HEX_LITERAL_TOK :
               oct ? UNSIGNED_OCT_LITERAL_TOK :
               bin ? UNSIGNED_BIN_LITERAL_TOK : 
                     UNSIGNED_LITERAL_TOK;
      case 1: 
        return hex ? UNSIGNED_LONG_HEX_LITERAL_TOK :
               oct ? UNSIGNED_LONG_OCT_LITERAL_TOK :
               bin ? UNSIGNED_LONG_BIN_LITERAL_TOK : 
                     UNSIGNED_LONG_LITERAL_TOK;
      case 2: 
        return hex ? UNSIGNED_LONG_LONG_HEX_LITERAL_TOK :
               oct ? UNSIGNED_LONG_LONG_OCT_LITERAL_TOK :
               bin ? UNSIGNED_LONG_LONG_BIN_LITERAL_TOK : 
                     UNSIGNED_LONG_LONG_LITERAL_TOK;
      default: error("Expected at most 2 long suffixes."); 
    }
  }

  if(d_suffix) {
    if(hex) error("Cannot have hexadecimal decimal floats.");
    if(oct) error("Cannot have octal decimal floats.");
    if(bin) error("Cannot have binary decimal floats.");
    if(f_suffix == 1 && d_suffix == 1 && !l_suffix) {
      return _DECIMAL32_LITERAL_TOK;
    } else if (l_suffix == 1 && d_suffix == 1 && !f_suffix) {
      return _DECIMAL64_LITERAL_TOK;
    } else if (d_suffix == 2 && !f_suffix && !l_suffix) {
      return _DECIMAL128_LITERAL_TOK;
    }
    error("Invalid suffix for decimal float.");
  }

  if(f_suffix == 1) {
    if(oct) error("Cannot have octal floats.");
    if(bin) error("Cannot have binary floats.");
    if(l_suffix) error("Cannot have 'f' and 'l' suffix together.");
    return hex ? FLOAT_HEX_LITERAL_TOK : FLOAT_LITERAL_TOK;
  } else if (f_suffix > 1) {
    error("Expected at most one 'f' suffix.");
  }

  if(dot || exp) {
    if(l_suffix == 1) 
      return hex ? LONG_DOUBLE_HEX_LITERAL_TOK : LONG_DOUBLE_LITERAL_TOK;
    else if(l_suffix)
      error("Expect at most 1 'l' suffix for floating point literal.");
    return hex ? DOUBLE_HEX_LITERAL_TOK : DOUBLE_LITERAL_TOK;
  }

  switch(l_suffix) {
    case 0: 
      return hex ? HEX_LITERAL_TOK :
             oct ? OCT_LITERAL_TOK :
             bin ? BIN_LITERAL_TOK : 
                   INT_LITERAL_TOK;
    case 1: 
      return hex ? LONG_HEX_LITERAL_TOK :
             oct ? LONG_OCT_LITERAL_TOK :
             bin ? LONG_BIN_LITERAL_TOK : 
                   LONG_LITERAL_TOK;
    case 2: 
      return hex ? LONG_LONG_HEX_LITERAL_TOK :
             oct ? LONG_LONG_OCT_LITERAL_TOK :
             bin ? LONG_LONG_BIN_LITERAL_TOK : 
                   LONG_LONG_LITERAL_TOK;
    default: error("Expected at most 2 long suffixes."); 
  }

  return UNKNOWN_TOK; // Should never reach this.
}

enum TType lex_identifier_or_keyword(struct string_view* value)
{
  while(matchAlNum() || match('_')) {
    value->length++;
  }

  struct string_view defined = preproc_table_get(prepTable, *value);
  if(defined.begin != NULL) {
    lexer_push_text(defined);
    longjmp(jbuf, 1);
  }
  struct Macro defined_macro = macro_table_get(macroTable, *value);
  if(defined_macro.text.begin != NULL) {
    while(matchSpace()) ;
    if(!match('(')) error("Expected '(' after macro name");
    Array(struct string_view) arguments = array_new();
    array_capacity(arguments) = 0;
    array_length(arguments) = 0;
    array_ensure(&arguments, 4);
    while(!match(')')) {
      struct string_view arg = { .begin = &lexer->buffer[lexer->buffer_loc],
                                 .length = 0};
      while(!match(',') && !match(')')) {
        advance();
        arg.length++;
      }
      array_sv_append(&arguments, arg);
      if(previous() == ')') break;
    }
    char* text = macro_expand(defined_macro, arguments);
    printf("%s", text);
    array_free(arguments);
    lexer_push_str(text);
    longjmp(jbuf, 1);
  }

  for(int i = 0; i < NUM_KEYWORD; i++) {
    if(!strviewstrcmp(*value, keywords[i].keyword)) 
      return keywords[i].token; 
  }

  return IDENTIFIER_TOK;
}

enum TType lex_operator(struct string_view* value)
{
  char c = advance();
  switch(c) {
  case '-':
    if(match('>')) {
      value->length++;
      return ARROW_TOK;
    }
    [[fallthrough]];
  case '+':
  case '&':
  case '|':
  case '<':
  case '>':
    if(match(c)) {
      value->length++;
      switch(c) {
      case '+': return PLUS_PLUS_TOK;
      case '-': return MINUS_MINUS_TOK;
      case '&': return AND_AND_TOK;
      case '|': return VERT_VERT_TOK;
      case '<':
      case '>':
        if(match('=')) {
          value->length++;
          return c == '<' ? LSHIFT_EQUAL_TOK : RSHIFT_EQUAL_TOK;
        }
        return c == '<' ? LSHIFT_TOK : RSHIFT_TOK;
      default: error("Should never reach this: lexer.c, line\n",__LINE__);
      }
    }
    [[fallthrough]];
  case '*':
  case '/':
  case '%':
  case '!':
  case '~':
  case '^':
  case '=':
    if(match('=')) {
      value->length++;
      return operator_tokens[(unsigned char)c].equals;
    }
    return operator_tokens[(unsigned char)c].single;
  case '.': return PERIOD_TOK;
  case '?': return QMARK_TOK;
  case ':': return COLON_TOK;
  case ';': return SEMICOLON_TOK;
  case '(': return LPAREN_TOK;
  case ')': return RPAREN_TOK;
  case '{': return LBRACE_TOK;
  case '}': return RBRACE_TOK;
  case '[':
  case ']':
    if(match(c)) {
      value->length++;
      return c == '[' ? LSQUARE_LSQUARE_TOK : RSQUARE_RSQUARE_TOK;
    }
    return c == '[' ? LSQUARE_TOK : RSQUARE_TOK;
  default: return UNKNOWN_TOK;
  }
}

void lex_macro(struct string_view to_define) {
  Array(struct string_view) arg_names = array_new();
  array_capacity(arg_names) = 0;
  array_length(arg_names) = 0;
  array_sv_ensure(&arg_names, 4);

  printf("Defining macro: ");
  print_strview(to_define);
  printf("\n");

  while(!match(')')) {
    while(matchSpace()) {
      if(previous() == '\n') {
        error("Expected expression to expand macro into");
      }
    }
    struct string_view arg = { .begin = &lexer->buffer[lexer->buffer_loc],
                               .length = 0 };
    while(!match(',') && !matchSpace() && !match(')')) {
      arg.length++;
      advance();
    }
    if(previous() == ')') {
      array_sv_append(&arg_names, arg);
      printf("Appending arg ");
      print_strview(arg);
      printf("\n");
      break;
    }
    if(previous() == '\n') {
      error("Expected ')' in macro");
    }
    if(isspace(previous())) {
      while(!match(',') && !match(')')) advance();
      if(previous() == ')') {
        array_sv_append(&arg_names, arg);
        printf("Appending arg ");
        print_strview(arg);
        printf("\n");
        break;
      }
    }
    printf("Array capacity: %zu\n", array_capacity(arg_names));
    printf("Array length: %zu\n", array_length(arg_names));
    array_sv_append(&arg_names, arg);
    printf("Appending arg ");
    print_strview(arg);
    printf("\n");
  }

  while(matchSpace()) {
    if(previous() == '\n') {
      error("Expected expression to expand macro into");
    }
  }

  struct string_view macro_exp = { .begin = &lexer->buffer[lexer->buffer_loc],
                                   .length = 0 };

  while(!match('\n')) {
    macro_exp.length++;
    advance();
  }
  struct Macro macro = { .text = macro_exp, .arg_names = arg_names };
  macro_table_set(&macroTable, to_define, macro);
  array_free(arg_names);
}

void preprocessor_lexer()
{
  while(matchSpace()) {
    if(previous() == '\n') {
      ++lexer->line;
      lexer->position = 1;
      return;
    }
  }

  struct string_view directive = (struct string_view){ .begin = &lexer->buffer[lexer->buffer_loc],
                                                       .length = 0};
  while(matchAlpha()) directive.length++;

  if(!strviewstrcmp(directive, "define")) {
    while(matchSpace()) {
      if(previous() == '\n') {
        ++lexer->line;
        lexer->position = 1;
        error("Preprocessor error: define with no term to define");
        return;
      }
    }
    struct string_view to_define = (struct string_view){ .begin = &lexer->buffer[lexer->buffer_loc],
                                                         .length = 0};
    while(!matchSpace()) {
      if(match('(')) {
        lex_macro(to_define);
        return;
      }
      advance();
      to_define.length++;
    }
    struct string_view value = {0};
    if(previous() != '\n') {
      while(matchSpace()) {
        if(previous() =='\n') {
          goto set_define;
        }
      }
      value.begin = &lexer->buffer[lexer->buffer_loc];
      while(!match('\n')) {
        advance();
        value.length++;
      }
    }
set_define:
    preproc_table_set(&prepTable, to_define, value);
    lexer->line++;
    lexer->position = 1;
  } else if(!strviewstrcmp(directive, "undef")) { 
    while(matchSpace()) {
      if(previous() == '\n') {
        ++lexer->line;
        lexer->position = 1;
        error("Preprocessor error: undef with no term to undef");
        return;
      }
    }
    struct string_view to_undef = (struct string_view){ .begin = &lexer->buffer[lexer->buffer_loc],
                                                        .length = 0};
    while(!matchSpace()) {
      advance();
      to_undef.length++;
    }
    bool found_end = false;
    if(previous() == '\n') found_end = true;
    preproc_table_delete(&prepTable, to_undef);
    if(!found_end)
      while(!match('\n')) advance();
    lexer->line++;
    lexer->position = 1;
  } else if(!strviewstrcmp(directive, "include")) { 
  } else if(!strviewstrcmp(directive, "if")) { 
  } else if(!strviewstrcmp(directive, "ifdef")) { 
  } else if(!strviewstrcmp(directive, "ifndef")) { 
  } else if(!strviewstrcmp(directive, "else")) { 
  } else if(!strviewstrcmp(directive, "elif")) { 
  } else if(!strviewstrcmp(directive, "elifdef")) { 
  } else if(!strviewstrcmp(directive, "elifndef")) { 
  } else if(!strviewstrcmp(directive, "endif")) { 
  } else if(!strviewstrcmp(directive, "line")) { 
  } else if(!strviewstrcmp(directive, "embed")) { 
  } else if(!strviewstrcmp(directive, "error")) { 
    while(matchSpace()) {
      if(previous() == '\n') {
        ++lexer->line;
        lexer->position = 1;
        error("Preprocessor error");
        return;
      }
    }
    struct string_view to_err = (struct string_view){ .begin = &lexer->buffer[lexer->buffer_loc],
                                                      .length = 0};
    while(!match('\n')) {
      advance();
      to_err.length++;
    }
    char* to_err_str = strviewtostr(to_err);
    error("Preprocessor error: %s", to_err_str);
    free(to_err_str);
    lexer->line++;
    lexer->position = 0;
  } else if(!strviewstrcmp(directive, "warning")) { 
    while(matchSpace()) {
      if(previous() == '\n') {
        ++lexer->line;
        lexer->position = 1;
        warning("Preprocessor warning");
        return;
      }
    }
    struct string_view to_warn = (struct string_view){ .begin = &lexer->buffer[lexer->buffer_loc],
                                                       .length = 0};
    while(!match('\n')) {
      advance();
      to_warn.length++;
    }
    char* to_warn_str = strviewtostr(to_warn);
    warning("Preprocessor warning: %s", to_warn_str);
    free(to_warn_str);
    lexer->line++;
    lexer->position = 1;
  } else if(!strviewstrcmp(directive, "pragma")) { 
    while(matchSpace()) {
      if(previous() == '\n') {
        ++lexer->line;
        lexer->position = 1;
        warning("pragma not supported.");
        return;
      }
    }
    struct string_view to_warn = (struct string_view){ .begin = &lexer->buffer[lexer->buffer_loc],
                                                       .length = 0};
    while(!match('\n')) {
      advance();
      to_warn.length++;
    }
    char* to_warn_str = strviewtostr(to_warn);
    warning("Pragma not supported at the moment.\n"
            "Pragma used: %s", to_warn_str);
    free(to_warn_str);
    lexer->line++;
  } else {
  }
}

bool get_next_token(struct Token* out)
{
  if(!out) return 0;

  setjmp(jbuf);

  out->type = UNKNOWN_TOK;

skip_whitespace:

  while(matchSpace()) {
    lexer->position++;
    if(previous() == '\n') {
      lexer->line++;
      lexer->position = 1;
    }
  }

  if(match('#')) {
    preprocessor_lexer();
    goto skip_whitespace;
  }
 
  out->line = lexer->line;
  out->position = lexer->position;
  struct string_view token_value = (struct string_view){.begin = &lexer->buffer[lexer->buffer_loc], .length = 1};
  
  char next = ' ';

  if(isAtEnd()) {
    if(lexer->next) {
      lexer_pop();
      longjmp(jbuf, 2);
    }
    out->type = EOF_TOK;
  } else if(match('\'')) { // lex char inlexer->line
    if(match('\\')) {
      token_value.length++;
      if(matchOne("\'\"\?\\abfnrtv")) {
        if(!match('\'')) {
          error("Expected \' after escape-sequence-char. File %s, line %i, position %i.\n", lexer->current_file, lexer->line, lexer->position);
        }
        token_value.length += 2;
      } else if(peek() == '0' && peekNext() == '\'') {
        advance();
        advance();
        token_value.length += 2;
      } else if(match('x')) {
        token_value.length++;
        while(matchXDigit()) token_value.length++;
        if(!match('\'')) error("Expected \' after hex value at line %i, position %i.\n", lexer->line, lexer->position);
        token_value.length++;
      } else {
        while(matchOne("01234567")) token_value.length++;
        if(!match('\'')) error("Expected \' after octal value at line %i, position %i.\n", lexer->line, lexer->position);
        token_value.length++;
      }
    } else if(peekNext() == '\'') {
      advance();
      advance();
      token_value.length = 3;
    } else {
      error("Char literal on line %i, position %i not a valid char.\n", lexer->line, lexer->position);
    }
    out->type = CHAR_LITERAL_TOK;
  } else if(next == 'L' && peekNext() == '\'') {
    out->type = WIDE_CHAR_LITERAL_TOK;
  } else if(next == 'u' && peek() == '8' && peekNext() == '\'') {
    out->type = U8_CHAR_LITERAL_TOK;
  } else if(next == 'u' && peek() == '\'') {
    out->type = U16_CHAR_LITERAL_TOK;
  } else if(next == 'U' && peek() == '\'') {
    out->type = U32_CHAR_LITERAL_TOK;
  } else if(match('"')) {
    lex_string(&token_value);
    out->type = STR_LITERAL_TOK;
  } else if(isdigit(peek()) || (peek() == '.' && isdigit(peekNext()))) {
    out->type = lex_number(&token_value);
  } else if(matchAlpha() || match('_')) {
    out->type = lex_identifier_or_keyword(&token_value);
  } else if(ispunct(peek())) {
    out->type = lex_operator(&token_value);
  } else error("Line %i, Location %i: Unreconized token.", lexer->line, lexer->position);

  lexer->position += (int)token_value.length;

  out->file = strdup(lexer->current_file);
  out->value = token_value;
#warning Test warning
  return out->type != EOF_TOK;
}
