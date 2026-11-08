#define i_implement
#define i_import
#include <stc/cstr.h>

typedef struct Expr Expr;
typedef struct Env Env;
typedef struct vec_expr vec_expr;

// typedef struct Expr* (*LispProc)(vec_expr args, Env* env, const char** err);
typedef struct Expr* (*LispProc)(vec_expr* args, Env* env, cstr* err);

typedef struct {
  cstr sym;
} Symbol;

typedef struct {
  i64 num;
} Number;

typedef struct {
  f64 flo;
} Float;

typedef struct {
  cstr str;
} String;

typedef struct {
  Expr* car;
  Expr* cdr;
} Cons;

typedef struct {
  LispProc pro;
  bool is_special;
} Proc;

#define i_pro_key cstr
#include <stc/vec.h>

typedef struct {
  vec_cstr params;
  Expr* body;
  Env* closure;
} Lambda;

typedef enum {
  TYPE_SYMBOL,
  TYPE_NUMBER,
  TYPE_FLOAT,
  TYPE_STRING,
  TYPE_TRUE,
  TYPE_NIL,
  TYPE_CONS,
  TYPE_PROC,
  TYPE_LAMBDA,
} TypeTag;

struct Expr {
  TypeTag type;
  union {
    cstr sym;
    i64 num;
    f64 flo;
    cstr str;
    u8 tru_val;
    u8 nil_val;
    Cons cons;
    Proc proc;
    Lambda lambda;
  } data;
};

#define T vec_expr, Expr*
#include <stc/vec.h>

typedef struct {
  cstr input;
  u64 position;
  u64 read_position;
  u8 ch;
} Reader;

#define T hmap_proc, cstr, Expr*, (c_pro_key)
#define i_eq
#include <stc/hashmap.h>

struct Env {
  Env* outer;
  hmap_proc bindings;
};

// #define i_implement
// #define i_import
// #include <stc/cstr.h>
