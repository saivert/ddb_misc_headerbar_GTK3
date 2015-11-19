#if ENABLE_NLS

# include <libintl.h>
#define _(s) gettext(s)

#else

#define _(s) (s)

#endif

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
