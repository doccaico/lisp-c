
Expr* new_symbol(cstr sym)
{
  Expr* e = malloc(sizeof(Expr));
  e->type = TYPE_SYMBOL;
  e->data.sym = sym;
  return e;
}

Expr* new_int(i64 num)
{
  Expr* e = malloc(sizeof(Expr));
  e->type = TYPE_NUMBER;
  e->data.num = num;
  return e;
}

Expr* new_float(f64 flo)
{
  Expr* e = malloc(sizeof(Expr));
  e->type = TYPE_FLOAT;
  e->data.flo = flo;
  return e;
}

Expr* new_str(cstr str)
{
  Expr* e = malloc(sizeof(Expr));
  e->type = TYPE_STRING;
  e->data.str = str;
  return e;
}

Expr* new_cons(Expr* car, Expr* cdr)
{
  Expr* e = malloc(sizeof(Expr));
  e->type = TYPE_CONS;
  e->data.cons.car = car;
  e->data.cons.cdr = cdr;
  return e;
}

Expr* new_proc(LispProc proc, bool is_special)
{
  Expr* e = malloc(sizeof(Expr));
  e->type = TYPE_PROC;
  e->data.proc.pro = proc;
  e->data.proc.is_special = is_special;
  return e;
}

Expr* new_lambda(vec_cstr params, Expr* body, Env* closure)
{
  Expr* e = malloc(sizeof(Expr));
  e->type = TYPE_LAMBDA;
  e->data.lambda.params = params;
  e->data.lambda.body = body;
  e->data.lambda.closure = closure;
  return e;
}

Expr* new_nil(void)
{
  Expr* e = malloc(sizeof(Expr));
  e->type = TYPE_NIL;
  e->data.nil_val = 0; // dummy value
  return e;
}

Expr* new_true(void)
{
  Expr* e = malloc(sizeof(Expr));
  e->type = TYPE_TRUE;
  e->data.tru_val = 0; // dummy value
  return e;
}

vec_expr flatten_list(Expr* list_expr, bool *ok)
{
  Expr* current = list_expr;
  vec_expr res = vec_expr_init();

  for (;;)
  {
    switch (current->type)
    {
      case TYPE_CONS:
        vec_expr_push_back(&res, current->data.cons.car);
        current = current->data.cons.cdr;
        break;
      case TYPE_NIL:
        *ok = true;
        return res;
      default:
        vec_expr_push_back(&res, current);
        *ok = false;
        return res;
    }
  }
}

cstr to_string(Expr* expr)
{
  cstr sb = cstr_init();

  switch (expr->type)
  {
    case TYPE_SYMBOL:
      // return expr->data.sym;
      return cstr_clone(expr->data.sym);
    case TYPE_NUMBER:
      return cstr_from_fmt("%d", expr->data.num);
    case TYPE_FLOAT:
      return cstr_from_fmt("%f", expr->data.flo);
    case TYPE_STRING:
      cstr_append_fmt(&sb, "\"%s\"", cstr_str(&expr->data.str));
      return sb;
    case TYPE_CONS:
      cstr_append(&sb, "(");
      Expr* current = expr;

      while (current->type == TYPE_CONS) {
        Cons c = current->data.cons;

        // 1. CAR (要素) の文字列化
        cstr s_car = to_string(c.car);
        cstr_append(&sb, cstr_str(&s_car));
        cstr_drop(&s_car); // メモリ解放

        // 2. CDR (次へのリンク) のチェック
        if (c.cdr->type == TYPE_NIL) {
          // リストの終端
          break;
        }
        else if (c.cdr->type == TYPE_CONS) {
          // まだリストが続くのでスペースを空けて次へ
          cstr_append(&sb, " ");
          current = c.cdr;
        }
        else {
          // 不完全リスト (ドット対)
          cstr_append(&sb, " . ");
          cstr s_cdr = to_string(c.cdr);
          cstr_append(&sb, cstr_str(&s_cdr));
          cstr_drop(&s_cdr);
          break;
        }
      }
      cstr_append(&sb, ")");
      return sb;
    case TYPE_TRUE:
      return cstr_lit("T");
    case TYPE_NIL:
      return cstr_lit("NIL");
    case TYPE_PROC:
      return cstr_lit("<PROC>");
    case TYPE_LAMBDA:
      return cstr_lit("<LAMBDA>");
  }

  return cstr_lit("reached the end of to_string()");
}

Expr* expr_clone(Expr* expr)
{
  Expr* new_expr = malloc(sizeof(Expr));

  *new_expr = *expr;

  switch (expr->type)
  {
    case TYPE_SYMBOL:
      new_expr->data.sym = cstr_clone(expr->data.sym);
      break;
    case TYPE_STRING:
      new_expr->data.str = cstr_clone(expr->data.str);
      break;
    case TYPE_CONS:
      new_expr->data.cons.car = expr_clone(expr->data.cons.car);
      new_expr->data.cons.cdr = expr_clone(expr->data.cons.cdr);
      break;
  }

  return new_expr;
}

void assign_err(cstr* err, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  cstr_vfmt(err, 0, fmt, args);
  va_end(args);
}

void free_all(Expr* expr)
{
  if (expr == NULL)
  {
    return;
  }

  switch (expr->type)
  {
    case TYPE_SYMBOL:
      cstr_drop(&expr->data.sym);
      break;
    case TYPE_STRING:
      cstr_drop(&expr->data.str);
      break;
    case TYPE_CONS:
      free_all(expr->data.cons.car);
      free_all(expr->data.cons.cdr);
      break;
  }

  free(expr);
}
