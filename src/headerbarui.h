#if ENABLE_NLS

# include <libintl.h>
#define _(s) gettext(s)

#else

#define _(s) (s)

#endif

#ifdef DEBUG
#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#else
#define trace(fmt,...)
#endif
