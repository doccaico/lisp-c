Expr* eval(Expr* expr, Env* env, cstr* err);

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
      cstr s = to_string(item);
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
    cstr s = to_string(first_arg);
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
      cstr s = to_string(item);
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
      cstr s = to_string(item);
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
      cstr s = to_string(first_arg);
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
      cstr s = to_string(first_arg);
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

Env* find_env(Env* env, cstr* name)
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
  Env* env = malloc(sizeof(Env));

  env->outer = NULL;
  env->bindings = hmap_proc_init();
  hmap_proc* b = &env->bindings;

  REGISTER_PROC(b, "if", if_proc, true);

  REGISTER_PROC(b, "+", add_proc, false);
  REGISTER_PROC(b, "-", sub_proc, false);
  REGISTER_PROC(b, "*", mul_proc, false);
  REGISTER_PROC(b, "/", div_proc, false);

  return env;
}
