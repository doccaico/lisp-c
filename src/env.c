Expr* eval(Expr* expr, Env* env, cstr* err);
Env* find_env(Env* env, const cstr* name);

static bool conv_float(Expr* expr, f64* val)
{
  switch (expr->type)
  {
    case TYPE_NUMBER:
      *val = (f64)expr->data.num;
      return true;
    case TYPE_FLOAT:
      *val = expr->data.flo;
      return true;
  }
  return false;
}

static Expr* new_numeric_expr(f64 flo, bool is_all_int)
{
  if (is_all_int)
  {
    return new_int((i64)flo);
  }
  return new_float(flo);
}

Expr* if_proc(vec_expr *args, Env* env, cstr* err)
{
  // (if T 1 2) -> 1
  // (if NIL 1 2) -> 2

  if (vec_expr_size(args) != 3)
  {
    const cstr s = cstr_from_fmt(
        "if requires exactly 3 arguments (test conseq alt), got %d",
        vec_expr_size(args));
    assign_err(err, cstr_str(&s));
    cstr_drop(&s);
    return NULL;
  }

  Expr* test = *vec_expr_at(args, 0);
  Expr* conseq = *vec_expr_at(args, 1);
  Expr* alt = *vec_expr_at(args, 2);

  Expr* test_res = eval(test, env, err);
  if (!cstr_is_empty(err))
  {
    return NULL;
  }
  bool is_true = true;
  if (test_res->type == TYPE_NIL)
  {
    is_true = false;
  }
  if (is_true)
  {
    return eval(conseq, env, err);
  }
  else
  {
    return eval(alt, env, err);
  }
}

Expr* progn_proc(vec_expr *args, Env* env, cstr* err)
{
  // (progn T NIL T NIL 666) -> 666
  // (progn (define x 1) (define y 2) (+ x y)) -> 3

  if (vec_expr_size(args) == 0)
  {
    return new_nil();
  }

  Expr* final_result = NULL;

  for (isize_t i = 0; i < vec_expr_size(args); i++)
  {
    if (final_result != NULL)
    {
      free_all(final_result);
      final_result = NULL;
    }

    Expr* form = *vec_expr_at(args, i);

    final_result = eval(form, env, err);
    if (!cstr_is_empty(err))
    {
      if (final_result != NULL)
      {
        free_all(final_result);
      }
      return NULL;
    }
  }
  return final_result;
}

Expr* define_proc(vec_expr *args, Env* env, cstr* err)
{
	// (define x 10)
	// x -> 10

  if (vec_expr_size(args) != 2)
  {
    assign_err(err, "define requires exactly 2 argument");
    return NULL;
  }

  Expr* first_arg = *vec_expr_at(args, 0);
  if (first_arg->type != TYPE_SYMBOL)
  {
    const cstr s = to_string(first_arg);
    assign_err(err, "the argument to define, %s, is not a symbol", cstr_str(&s));
    cstr_drop(&s);
    return NULL;
  }

  const cstr define_sym = first_arg->data.sym;
  Expr* define_expr = *vec_expr_at(args, 1);

  Expr* value_expr = eval(define_expr, env, err);
  if (!cstr_is_empty(err))
  {
    return NULL;
  }

  hmap_proc_iter it = hmap_proc_find(&env->bindings, cstr_str(&define_sym));
  if (it.ref != NULL)
  {
    cstr_drop(&it.ref->first);
    free_all(it.ref->second);
    // ハッシュマップからそのエントリーを削除
    hmap_proc_erase_at(&env->bindings, it);
  }

  hmap_proc_value pair = {
    .first = cstr_clone(define_sym),
    .second = value_expr,
  };
  hmap_proc_push(&env->bindings, pair);

  return expr_clone(first_arg);
}

Expr* quote_proc(vec_expr *args, Env* env, cstr* err)
{
  // (quote x) -> x
  // (quote (list 1 2 3)) -> (list 1 2 3)

  (void) env;

  if (vec_expr_size(args) != 1)
  {
    assign_err(err, "quote requires exactly 1 argument");
    return NULL;
  }

  Expr* first_arg = *vec_expr_at(args, 0);
  return expr_clone(first_arg);
}

Expr* setf_proc(vec_expr *args, Env* env, cstr* err)
{
	// (setf x 1) -> undefined symbol for setf: x
	// (define x 1)
	// (setf x 10)
	// x -> 10

  if (vec_expr_size(args) != 2)
  {
    assign_err(err, "setf requires exactly 2 argument");
    return NULL;
  }

  Expr* first_arg = *vec_expr_at(args, 0);
  if (first_arg->type != TYPE_SYMBOL)
  {
    const cstr s = to_string(first_arg);
    assign_err(err, "the argument to setf, %s, is not a symbol", cstr_str(&s));
    cstr_drop(&s);
    return NULL;
  }

  const cstr setf_sym = first_arg->data.sym;
  Expr* setf_expr = *vec_expr_at(args, 1);

  Env* target_env = find_env(env, &setf_sym);
  if (target_env == NULL)
  {
    assign_err(err, "the argument to setf, %s, is not a symbol", cstr_str(&setf_sym));
    return NULL;
  }

  Expr* val = eval(setf_expr, env, err);
  if (!cstr_is_empty(err))
  {
    return NULL;
  }

  hmap_proc_iter it = hmap_proc_find(&target_env->bindings, cstr_str(&setf_sym));
  if (it.ref != NULL)
  {
    cstr_drop(&it.ref->first);
    free_all(it.ref->second);
    hmap_proc_erase_at(&target_env->bindings, it);
  }

  hmap_proc_value pair = {
    .first = cstr_clone(setf_sym),
    .second = val,
  };
  hmap_proc_push(&env->bindings, pair);

  return expr_clone(val);
}

Expr* lambda_proc(vec_expr *args, Env* env, cstr* err)
{
  // ((lambda (a) (+ a 10)) 1) -> 11

  if (vec_expr_size(args) < 2)
  {
    assign_err(err, "lambda requires at least a parameter list and a body");
    return NULL;
  }

  Expr* param_list_expr = *vec_expr_at(args, 0);
  bool param_list_ok = false;
  vec_expr param_list = flatten_list(param_list_expr,&param_list_ok);
  if (!param_list_ok)
  {
    vec_expr_drop(&param_list);
    assign_err(err, "lambda parameters must be a proper list");
    return NULL;
  }

  vec_cstr params = vec_cstr_init();
  for (isize_t i = 0; i < vec_expr_size(&param_list); i++)
  {
    Expr* param = *vec_expr_at(&param_list, i);

    if (param->type != TYPE_SYMBOL)
    {
      for (isize_t j = 0; j < vec_cstr_size(&params); j++)
      {
        cstr_drop(vec_cstr_at_mut(&params, j));
      }

      vec_cstr_drop(&params);
      vec_expr_drop(&param_list);
      assign_err(err, "lambda parameter must be a symbol");
      return NULL;
    }
    vec_cstr_push_back(&params, cstr_clone(param->data.sym));
  }

  // params の作成が終わったら、一時的な param_list はここで削除する
  vec_expr_drop(&param_list);

  // Implicit progn
  Expr* body = NULL;
  if (vec_expr_size(args) == 2)
  {
    body = expr_clone(*vec_expr_at(args, 1));
  }
  else
  {
    Expr* current_cdr = new_nil();

    for (isize_t i = vec_expr_size(args) - 1; i >= 1; i--)
    {
      current_cdr = new_cons(expr_clone(*vec_expr_at(args, i)), current_cdr);
    }

    Expr* progn_sym = new_symbol(cstr_lit("progn"));
    body = new_cons(progn_sym, current_cdr);
  }
  return new_lambda(params, body, env);
}

Expr* add_proc(vec_expr *args, Env* env, cstr* err)
{
  // (+) -> 0
  // (+ 10) -> 10
  // (+ 10 10 10) -> 30

  (void) env;

  if (vec_expr_size(args) == 0)
  {
    return new_int(0);
  }

  f64 sum = 0.0;
  bool all_int = true;

  for (isize_t i = 0; i < vec_expr_size(args); i++)
  {
    Expr* item = *vec_expr_at(args, i);

    f64 val;
    if (!conv_float(item, &val))
    {
      const cstr s = to_string(item);
      assign_err(err, "%s is not a number or float", cstr_str(&s));
      cstr_drop(&s);
      return NULL;
    }
    sum += val;

    if (!(item->type == TYPE_NUMBER))
    {
      all_int = false;
    }
  }

  return new_numeric_expr(sum, all_int);
}

Expr* sub_proc(vec_expr *args, Env* env, cstr* err)
{
  // (- 10) -> -10
  // (- 10 5 2) -> 3

  (void) env;

  if (vec_expr_size(args) == 0)
  {
    assign_err(err, "invalid number of arguments: 0");
    return NULL;
  }

  f64 first_val = 0.0;
  Expr* first_arg = *vec_expr_at(args, 0);
  if (!conv_float(first_arg, &first_val))
  {
    const cstr s = to_string(first_arg);
    assign_err(err, "%s is not a number or float", cstr_str(&s));
    cstr_drop(&s);
    return NULL;
  }

  bool all_int = (first_arg->type == TYPE_NUMBER) ? true : false;
  if (vec_expr_size(args) == 1)
  {
    return new_numeric_expr(-first_val, all_int);
  }

  f64 sum = first_val;

  for (isize_t i = 1; i < vec_expr_size(args); i++)
  {
    Expr* item = *vec_expr_at(args, i);

    f64 val;
    if (!conv_float(item, &val))
    {
      const cstr s = to_string(item);
      assign_err(err, "%s is not a number or float", cstr_str(&s));
      cstr_drop(&s);
      return NULL;
    }
    sum -= val;

    if (!(item->type == TYPE_NUMBER))
    {
      all_int = false;
    }
  }

  return new_numeric_expr(sum, all_int);
}

Expr* mul_proc(vec_expr *args, Env* env, cstr* err)
{
  // (*) -> 1
  // (* 10) -> 10
  // (* 2 5 10) -> 100

  (void) env;

  if (vec_expr_size(args) == 0)
  {
    return new_int(1);
  }

  f64 sum = 1.0;
  bool all_int = true;

  for (isize_t i = 0; i < vec_expr_size(args); i++)
  {
    Expr* item = *vec_expr_at(args, i);

    f64 val;
    if (!conv_float(item, &val))
    {
      const cstr s = to_string(item);
      assign_err(err, "%s is not a number or float", cstr_str(&s));
      cstr_drop(&s);
      return NULL;
    }
    sum *= val;

    if (!(item->type == TYPE_NUMBER))
    {
      all_int = false;
    }
  }

  return new_numeric_expr(sum, all_int);
}

Expr* div_proc(vec_expr *args, Env* env, cstr* err)
{
	// (/ 0) -> division by zero
	// (/ 10) -> 0.1
	// (/ 10.5) -> 0.09523809523809523
	// (/ 10 5) -> 2
	// (/ 5 2) -> 2.5

  (void) env;

  if (vec_expr_size(args) == 0)
  {
    assign_err(err, "invalid number of arguments: 0");
    return NULL;
  }

  f64 first_val;
  Expr* first_arg = *vec_expr_at(args, 0);
  if (!conv_float(first_arg, &first_val))
  {
      const cstr s = to_string(first_arg);
      assign_err(err, "%s is not a number or float", cstr_str(&s));
      cstr_drop(&s);
      return NULL;
  }

  if (vec_expr_size(args) == 1)
  {
    if (first_val == 0.0)
    {
      assign_err(err, "division by zero");
      return NULL;
    }
    return new_float(1.0 / first_val);
  }

  f64 sum = first_val;

  for (isize_t i = 1; i < vec_expr_size(args); i++)
  {
    Expr* item = *vec_expr_at(args, i);

    f64 val;
    if (!conv_float(item, &val))
    {
      const cstr s = to_string(first_arg);
      assign_err(err, "%s is not a number or float", cstr_str(&s));
      cstr_drop(&s);
      return NULL;
    }

    if (val == 0.0)
    {
      assign_err(err, "division by zero");
      return NULL;
    }

    sum /= val;
  }

  f64 int_part;
  f64 frac_part;
  frac_part = modf(sum, &int_part);
  if (frac_part == 0.0)
  {
    return new_numeric_expr(int_part, true);
  } else {
    return new_numeric_expr(sum, false);
  }
}

Env* find_env(Env* env, const cstr* name)
{
  Env* current = env;

  while (current != NULL)
  {
    if (hmap_proc_contains(&current->bindings, cstr_str(name)))
    {
      return current;
    }
    else
    {
      current = current->outer;
    }
  }
  return NULL;
}

#define REGISTER_PROC(bindings, name, proc, is_special) \
  do {                                                  \
    hmap_proc_value pair = {                            \
      .first = cstr_lit(name),                          \
      .second = new_proc((proc), (is_special))          \
    };                                                  \
    hmap_proc_push((bindings), pair);                   \
  } while (0)

Env* standard_env(void)
{
  Env* env = xmalloc(sizeof(Env));

  env->outer = NULL;
  env->bindings = hmap_proc_init();
  hmap_proc* b = &env->bindings;

  REGISTER_PROC(b, "if", if_proc, true);
  REGISTER_PROC(b, "progn", progn_proc, true);
  REGISTER_PROC(b, "define", define_proc, true);
  REGISTER_PROC(b, "quote", quote_proc, true);
  REGISTER_PROC(b, "setf", setf_proc, true);
  REGISTER_PROC(b, "lambda", lambda_proc, true);

  REGISTER_PROC(b, "+", add_proc, false);
  REGISTER_PROC(b, "-", sub_proc, false);
  REGISTER_PROC(b, "*", mul_proc, false);
  REGISTER_PROC(b, "/", div_proc, false);

  return env;
}
