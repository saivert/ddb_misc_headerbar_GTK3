
#ifdef DEBUG
#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#else
#define trace(fmt,...)
#endif
