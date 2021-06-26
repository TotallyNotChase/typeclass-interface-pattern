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


/* The `Enum` typeclass allows types to be enumerable */
typedef struct
{
    int (*const from_enum)(void* self);
} EnumTC;

typedef struct
{
    void* self;
    EnumTC const* tc;
} Enum;


/* Typeclass that asks for both `Show` and `Enum` implementation */
typedef struct
{
    void* self;
    ShowTC const* showtc;
    EnumTC const* enumtc;
} ShowEnum;

void print_shen(ShowEnum shen)
{
    char* const s = shen.showtc->show(shen.self);
    int enm = shen.enumtc->from_enum(shen.self);
    printf("%s : %d\n", s, enm);
    free(s);
}

/* The `show` function implementation for `int` */
static char* int_show(int* x)
{
    /*
    Note: The `show` function of a `Show` typeclass is expected to return a malloc'ed value
    The users of a generic `Show` are expected to `free` the returned pointer from the function `show`.
    */
    size_t len = snprintf(NULL, 0, "%d", *x);
    char* const res = malloc((len + 1) * sizeof(*res));
    snprintf(res, len + 1, "%d", *x);
    return res;
}

/* The wrapper function around `int_show` */
static inline char* int_show__(void* self)
{
    return int_show(self);
}

/* Make function to build a generic `Show` out of a concrete type- `int` */
Show int_to_show(int* x)
{
    /* Build the vtable once and attach a pointer to it every time */
    static ShowTC const tc = { .show = int_show__ };
    return (Show){ .tc = &tc, .self = x };
}

/* The `from_enum` function implementation for `int` */
static int int_from_enum(int* x)
{
    return *x;
}

/* The wrapper function around `int_from_enum` */
static inline int int_from_enum__(void* self)
{
    return int_from_enum(self);
}

/* Make function to build a generic `Show` out of a concrete type- `int` */
Enum int_to_enum(int* x)
{
    /* Build the vtable once and attach a pointer to it every time */
    static EnumTC const tc = { .from_enum = int_from_enum__ };
    return (Enum){ .tc = &tc, .self = x };
}

int main(void)
{
    int x = 42;
    ShowEnum shen = { .self = &x, .showtc = int_to_show(&x).tc, .enumtc = int_to_enum(&x).tc };
    print_shen(shen);
    return 0;
}
