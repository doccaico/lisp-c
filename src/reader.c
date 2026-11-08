void next_char(Reader* r);
void skip_whitespace(Reader* r);
cstr read_string(Reader* r, cstr* err);
Expr* read_list(Reader* r, cstr* err);
bool is_delimiter(u8 ch);
Expr* read_atom(Reader* r, cstr* err);

Reader init_reader(const char* input)
{
  Reader r = {
    .input = cstr_from(input),
    .position = 0,
    .read_position = 0,
    .ch = '\0',
  };

  next_char(&r);

  return r;
}

Expr* read(Reader* r, cstr* err)
{
  skip_whitespace(r);

  if (r->ch == '"')
  {
    // Return a string object
    cstr s = read_string(r, err);
    if (!cstr_is_empty(err))
    {
      return NULL;
    }
    return new_str(s);
  }
  else if (r->ch == '(')
  {
    // Consume '('
    next_char(r);
    // Delegate list parsing to the specialized function
    Expr* expr = read_list(r, err);
    if (!cstr_is_empty(err))
    {
      return NULL;
    }
    else
    {
      return expr;
    }
  }
  else if (r->ch == ')')
  {
    assign_err(err, "unexpected closing parenthesis ')'");
    return NULL;
  }
  else if (r->ch == '\0')
  {
    // End of input reached
    return NULL;
  }
  // If it's not a quote or parenthesis, it's guaranteed to be an atom
  return read_atom(r, err);
}

cstr read_string(Reader* r, cstr* err)
{
  // Skip the starting double quote
  next_char(r);

  cstr sb = cstr_init();

  for (;;)
  {
    if (r->ch == '"')
    {
      // Consume the closing quote and advance
      next_char(r);
      break;
    }

    if (r->ch == '\0')
    {
      assign_err(err, "unclosed string literal");
      return cstr_init();
    }

    if (r->ch == '\\')
    {
      // Consume '\\'
      next_char(r);

      if (r->ch == '\x00')
      {
        assign_err(err, "unclosed string literal after escape");
        return cstr_init();
      }

      switch (r->ch) {
        case 'n':
          cstr_append(&sb, "\n");
          break;
        case 't':
          cstr_append(&sb, "\t");
          break;
        case 'r':
          cstr_append(&sb, "\r");
          break;
        case '\\':
          cstr_append(&sb, "\\");
          break;
        case '"':
          cstr_append(&sb, "\"");
          break;
        default:
          cstr_append_fmt(&sb, "%c", r->ch);
          break;
      }
    }
    else
    {
      cstr_append_fmt(&sb, "%c", r->ch);
    }
    next_char(r);
  }
  return sb;
}

Expr* read_list(Reader* r, cstr* err) // エラー文字列を書き換えるため cstr* に変更
{
    vec_expr elements = vec_expr_init();
    bool has_dot = false;
    Expr* var_cdr = NULL;
    Expr* head = NULL; // 戻り値用の一時変数

    for (;;)
    {
        skip_whitespace(r);

        if (r->ch == ')') {
            next_char(r);
            break;
        }

        if (r->ch == '\0') {
            assign_err(err, "expected ')', but reached EOF");
            goto cleanup_error; // エラー時は一括解放へジャンプ
        }

        if (r->ch == '.') {
            next_char(r);
            skip_whitespace(r);

            if (r->ch == ')') {
                assign_err(err, "unexpected ')' after '.'");
                goto cleanup_error;
            }

            Expr* final_expr = read(r, err);
            if (!cstr_is_empty(err)) {
                goto cleanup_error;
            }

            skip_whitespace(r);
            if (r->ch != ')') {
                assign_err(err, "expected ')' after dot notation value");
                // final_expr がリークしないよう、elementsに入れておくか個別に free_all する
                free_all(final_expr);
                goto cleanup_error;
            }

            has_dot = true;
            var_cdr = final_expr;
            next_char(r);
            break;
        }

        Expr* expr = read(r, err);

        if (!cstr_is_empty(err)) {
            goto cleanup_error;
        }

        vec_expr_push_back(&elements, expr);
    }

    // リストの構築
    if (has_dot) {
        head = var_cdr;
    } else {
        head = new_nil();
    }

    // 【ループバグ修正】符号なし整数(u64)の逆順ループを安全に
    for (size_t i = vec_expr_size(&elements); i > 0; i--) {
        Expr** car = vec_expr_at_mut(&elements, i - 1);
        head = new_cons(*car, head);
    }

    // 【リーク対策1】成功時：ベクトル自体の内部バッファを解放（中身のExpr*はLispリストに移動したので解放してはダメ）
    vec_expr_drop(&elements);
    return head;

cleanup_error:
    // 【リーク対策2】失敗時：これまでに elements に溜まった Expr をすべて解放
    for (c_each (it, vec_expr, elements)) {
        free_all(*it.ref);
    }
    if (has_dot && var_cdr) {
        free_all(var_cdr);
    }
    vec_expr_drop(&elements); // ベクトル自体のメモリも解放
    return NULL;
}

bool try_parse_int(const char* s, int* out_val)
{
    if (s == NULL || *s == '\0')
    {
      return false;
    }

    char* endptr;
    long val = strtol(s, &endptr, 10);

    // 文字列の末尾（'\0'）まで完全に数字としてパースできた場合のみ成功
    if (*endptr == '\0') {
        *out_val = (int)val;
        return true;
    }
    return false;
}

bool try_parse_float(const char* s, double* out_val)
{
    if (s == NULL || *s == '\0')
    {
      return false;
    }

    char* endptr;
    double val = strtod(s, &endptr);

    // 文字列の末尾まで完全に小数としてパースできた場合のみ成功
    if (*endptr == '\0') {
        *out_val = val;
        return true;
    }
    return false;
}

Expr* read_atom(Reader* r, cstr* err)
{
  (void) err;

  u64 start_pos = r->position;

  for (; !is_delimiter(r->ch);)
  {
    next_char(r);
  }

  csview sv = cstr_subview(&r->input, start_pos, r->position - start_pos);
  cstr token = cstr_from_sv(sv);

  cstr upper = cstr_toupper(cstr_str(&token));
  if (cstr_equals(&upper, "T"))
  {
    cstr_drop(&token);
    cstr_drop(&upper);
    return new_true();
  }
  else if (cstr_equals(&upper, "NIL"))
  {
    cstr_drop(&token);
    cstr_drop(&upper);
    return new_nil();
  }
  cstr_drop(&upper);

  const char* raw_token = cstr_str(&token);
  Expr* result = NULL;

  int int_val;
  if (try_parse_int(raw_token, &int_val))
  {
    result = new_int(int_val);
  }
  else
  {
    double float_val;
    if (try_parse_float(raw_token, &float_val))
    {
      result = new_float(float_val);
    }
    else
    {
      result = new_symbol(token);
    }
  }

  return result;
}

// Skip whitespaces and comments (lines starting with ';')
void skip_whitespace(Reader* r)
{
  for (;;)
  {
    // Skip standard whitespaces
    switch (r->ch) {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
        next_char(r);
        continue;
      default:
        break;
    }

    // Skip comment line if ';' is encountered
    if (r->ch == ';')
    {
      // Consume characters until we hit a newline (\n, \r) or End of Input (\0)
      for (; r->ch != '\n' && r->ch != '\r' && r->ch != '\0';)
      {
        next_char(r);
      }
      continue;
    }
    // If the current character is neither a whitespace nor a comment start, stop skipping
    break;
  }
}

bool is_delimiter(u8 ch)
{
  // Whitespace, parentheses, quotes, or EOF are delimiters
  switch (ch)
  {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
    case '(':
    case ')':
    case '"':
    case '\0':
      return true;
    default:
      return false;
  }
}

// Advance one character (consume input)
void next_char(Reader* r)
{
  if (r->read_position >= (u64)cstr_size(&r->input)) {
    // Nul character signifies end of input
    r->ch = '\0';
  }
  else
  {
    r->ch = cstr_str(&r->input)[r->read_position];
  }

  r->position = r->read_position;
  r->read_position += 1;
}

