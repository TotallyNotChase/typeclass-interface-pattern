#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* Polymorphic printing function */
void print(Show showable)
{
    char* const s = showable.tc->show(showable.self);
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
    char* const s = malloc((strlen(x) + 1) * sizeof(*s));
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

/* The wrapper function around `antioch_show` */
static inline char* antioch_show__(void* self)
{
    return antioch_show(self);
}

/* Make function to build a generic `Show` out of a concrete type- `Antioch` */
Show prep_antioch_show(Antioch* x)
{
    /* Build the vtable once and attach a pointer to it every time */
    static ShowTC const tc = { .show = antioch_show__ };
    return (Show){ .tc = &tc, .self = x };
}

int main(void)
{
    Show const antsh = prep_antioch_show(&(Antioch){ grenade });
    print(antsh);
    return 0;
}
