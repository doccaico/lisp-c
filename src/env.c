bool conv_float(Expr* expr, f64* val)
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

Expr* new_numeric_expr(f64 flo, bool is_all_int)
{
  if (is_all_int)
  {
    return new_int((i64)flo);
  }
  return new_float(flo);
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

  for (c_each(v, vec_expr, *args)) {
    Expr* item = *v.ref;

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

  REGISTER_PROC(b, "+", add_proc, false);

  return env;
}

// REGISTER_PROC(&env->bindings, "+", add_proc, false);
