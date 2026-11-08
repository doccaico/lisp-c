int repl(const char* prompt, Env* global_env)
{
  char input_buf[2048];

  printf("The standard environment has been initialized. "
      "Number of registrations: %Id\n",
      hmap_proc_size(&global_env->bindings));

  for (;;)
  {
    printf(prompt);
    fflush(stdout);

    if (fgets(input_buf, sizeof(input_buf), stdin) == NULL) {
      printf("Bye :-:\n");
      break;
    }

    Reader r = init_reader(input_buf);

    for (;;)
    {
      skip_whitespace(&r);

      // 行の末尾（EOF）に達したら、次の入力を促すために内側ループを抜ける
      if (r.ch == '\0') {
        break;
      }

      cstr err_msg = cstr_init();

      Expr* ast = read(&r, &err_msg);

      if (!cstr_is_empty(&err_msg)) {
        puts(cstr_str(&err_msg));
        free_all(ast);
        cstr_drop(&err_msg);
        break;
      }

      Expr* result = eval(ast, global_env, &err_msg);

      if (!cstr_is_empty(&err_msg)) {
        printf("Eval Error: %s\n", cstr_str(&err_msg));
        free_all(ast);
        free_all(result);
        cstr_drop(&err_msg);
        break;
      }

      if (result != NULL) {
        cstr out_str = to_string(result);
        printf("%s\n", cstr_str(&out_str));
        cstr_drop(&out_str);
      }

      // この式のメモリを解放して次の式へ
      free_all(ast);
      free_all(result);
      cstr_drop(&err_msg);
    }

    cstr_drop(&r.input);
  }

  for (c_each(n, hmap_proc, global_env->bindings))
  {
    hmap_proc_value* pair = n.ref;
    cstr_drop(&pair->first);
    free_all(pair->second);
  }

  hmap_proc_drop(&global_env->bindings);
  free(global_env);

  return 0;
}
