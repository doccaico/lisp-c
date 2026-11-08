Expr* invoke(Expr* procedure, vec_expr* args, Env* env, cstr* err);

Expr* eval(Expr* expr, Env* env, cstr* err)
{
  switch (expr->type)
  {
    case TYPE_SYMBOL:
      Env* target_env = find_env(env, &expr->data.sym);
      if (target_env != NULL)
      {
        Expr* e = *hmap_proc_at_mut(&target_env->bindings, cstr_str(&expr->data.sym));
        return expr_clone(e);
      }
      else
      {
        // cstr expr_str = to_string(expr);
        // cstr msg = cstr_from_fmt("undefined symbol: %s", cstr_str(&expr_str));
        // cstr_assign(err, cstr_str(&msg));
        // cstr_drop(&msg);
        // cstr_drop(&expr_str);
        cstr s = to_string(expr);
        assign_err(err, "undefined symbol: %s", cstr_str(&s));
        cstr_drop(&s);
        return NULL;
      }
    case TYPE_NUMBER:
    case TYPE_FLOAT:
    case TYPE_STRING:
    case TYPE_PROC:
    case TYPE_LAMBDA:
    case TYPE_TRUE:
    case TYPE_NIL:
      return expr_clone(expr);
    case TYPE_CONS:
      Expr* first = eval(expr->data.cons.car, env, err);
      if (!cstr_is_empty(err))
      {
        return NULL;
      }

      // Expr* pro = eval(first, env, err);
      // if (!cstr_is_empty(err))
      // {
      //   return NULL;
      // }
      Expr* pro = first;

      bool list_ok;
      vec_expr list = flatten_list(expr->data.cons.cdr, &list_ok);
      if (!list_ok)
      {
        assign_err(err, "improper list (dot notation) cannot be evaluated as function arguments");
        free_all(pro);
        vec_expr_drop(&list);
        return NULL;
      }

      vec_expr args = vec_expr_init();

      bool is_special_form = (pro->type == TYPE_PROC && pro->data.proc.is_special) ? true : false;

      if (is_special_form)
      {
        for (isize_t i = 0; i < vec_expr_size(&list); i++)
        {
          Expr** elem = vec_expr_at_mut(&list, i);
          vec_expr_push_back(&args, *elem);
        }
      }
      else
      {
        for (isize_t i = 0; i < vec_expr_size(&list); i++)
        {
          Expr** elem = vec_expr_at_mut(&list, i);
          Expr* e = eval(*elem, env, err);
          if (!cstr_is_empty(err))
          {
            for (isize_t j = 0; j < vec_expr_size(&args); j++) {
              free_all(*vec_expr_at_mut(&args, j));
            }
            free_all(pro);
            vec_expr_drop(&list);
            vec_expr_drop(&args);
            return NULL;
          }
          else
          {
            vec_expr_push_back(&args, e);
          }
        }
      }

      Expr* final_result = invoke(pro, &args, env, err);

      free_all(pro);
      vec_expr_drop(&list);
      // invoke 内部で args の中身を流用していないことが前提です。
      // もし invoke が args を破壊的に吸い取るなら、中身の要素の free_all 扱いに注意してください。
      // 通常、ベクトルの器の drop は必須です。
      vec_expr_drop(&args);

      return final_result;
  }

  return NULL;
}


Expr* invoke(Expr* procedure, vec_expr* args, Env* env, cstr* err)
{
  switch (procedure->type)
  {
    case TYPE_PROC:
      Expr* res = procedure->data.proc.pro(args, env, err);
      if (!cstr_is_empty(err))
      {
        return NULL;
      }
      else
      {
        return res;
      }
    case TYPE_LAMBDA:
      if (vec_expr_size(args) != vec_cstr_size(&procedure->data.lambda.params))
      {
        const cstr s = cstr_from_fmt(
            "wrong number of arguments: expected %d, got %d",
            vec_cstr_size(&procedure->data.lambda.params),
            vec_expr_size(args));
        // cstr_assign(err, cstr_str(&msg));
        assign_err(err, "not a procedure or lambda: %s", cstr_str(&s));
        cstr_drop(&s);
        return NULL;
      }
      Env* new_env = malloc(sizeof(Env));
      new_env->outer = procedure->data.lambda.closure;
      new_env->bindings = hmap_proc_init();
      for (isize_t i = 0; i < vec_cstr_size(&procedure->data.lambda.params); i++)
      {
        const cstr* param_name_ptr = vec_cstr_at(&procedure->data.lambda.params, i);
        cstr key_clone = cstr_clone(*param_name_ptr);
        Expr* value_expr = *vec_expr_at(args, i);
        hmap_proc_value pair = {
          .first = key_clone,
          .second = value_expr,
        };
        hmap_proc_push(&new_env->bindings, pair);
      }
      return eval(procedure->data.lambda.body, new_env, err);
    default:
      cstr s = to_string(procedure);
      // cstr msg = cstr_from_fmt("not a procedure or lambda: %s", cstr_str(&s));
      // cstr_assign(err, cstr_str(&msg));
      // cstr_drop(&msg);
      // cstr_drop(&s);
      assign_err(err, "not a procedure or lambda: %s", cstr_str(&s));
      cstr_drop(&s);
      return NULL;
  }
}
