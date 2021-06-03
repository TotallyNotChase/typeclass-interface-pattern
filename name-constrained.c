#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONCAT_(x, y) x ## y
#define CONCAT(x, y) CONCAT_(x, y)

/* Consistently name the impl functions */
#define ImplName(T, TypeclassName) CONCAT(CONCAT(T, _to_), TypeclassName)

/* "Apply" a typeclass over a concrete type */
#define ap(x, T, TypeclassName) ImplName(T, TypeclassName)(x)

/* The `Show` typeclass allows types to be turned into their string representation */
typedef struct
{
    char* (*const show)(void* self);
} ShowTC;

typedef struct
{
    void* self;
    ShowTC const* tc;
} Show;

#define impl_show(T, show_f)                                                                                           \
    Show ImplName(T, Show)(T* x)                                                                                       \
    {                                                                                                                  \
        char* (*const show_)(T* self) = (show_f);                                                                      \
        (void)show_;                                                                                                   \
        static ShowTC const tc = {.show = (char* (*const)(void*))(show_f) };                                           \
        return (Show){.tc = &tc, .self = x};                                                                           \
    }

/* Polymorphic printing function */
void print(Show showable)
{
    char* s = showable.tc->show(showable.self);
    puts(s);
    free(s);
}


/* A very holy enum */
typedef enum
{
    holy,
    hand,
    grenade
} Antioch;

static inline char* strdup_(char const* x)
{
    char* s = malloc((strlen(x) + 1) * sizeof(*s));
    strcpy(s, x);
    return s;
}

/* The `show` function implementation for `Antioch*` */
static char* antioch_show(Antioch* x)
{
    /*
    Note: The `show` function of a `Show` typeclass is expected to return a malloc'ed value
    The users of a generic `Show` are expected to `free` the returned pointer from the function `show`.
    */
    switch (*x)
    {
        case holy:
            return strdup_("holy");
        case hand:
            return strdup_("hand");
        case grenade:
            return strdup_("grenade");
        default:
            return strdup_("breakfast cereal");
    }
}

/* Make function to build a generic `Show` out of a concrete type- `Antioch` */
impl_show(Antioch, antioch_show)

int main(void)
{
    print(ap(&(Antioch){ holy }, Antioch, Show));
    return 0;
}
