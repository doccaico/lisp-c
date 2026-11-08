const char* HELP_MESSAGE = "Usage: odin-lisp FILENAME\n"
"  -r, --repl:       REPL\n"
"  -h, --help:       show a help message\n";

int main(int argc, char** argv)
{
#ifdef ENABLE_LEAK_CHECK
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    Env* global_env = NULL;

    if (argc == 1)
    {
      puts(HELP_MESSAGE);
      return 0;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
    {
      puts(HELP_MESSAGE);
      return 0;
    }
    else if (strcmp(argv[1], "-r") == 0 || strcmp(argv[1], "--repl") == 0)
    {
      global_env = standard_env();
      return repl("lisp-c>> ", global_env);
    }

    global_env = standard_env();

    const char* sc = "(+ 1 2)";
    Reader r = init_reader(sc);

    cstr err = cstr_init();
		Expr* expr = read(&r, &err);
    if (!cstr_is_empty(&err))
    {
      printf("%s\n", cstr_str(&err));
      return 0;
    }

    cstr res = to_string(expr);
    printf("%s\n", cstr_str(&res));

    free_all(expr);

    return 0;
}
