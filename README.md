# Polymorphism through Typeclasses / Interface / Traits
Ideas, thoughts, and notes on an action based polymorphism pattern for good ol' C. Originally used in [c-iterators](https://github.com/TotallyNotChase/c-iterators), and explained in a [small document](https://github.com/TotallyNotChase/c-iterators/blob/master/Typeclass%20Pattern.md).

This is meant to be an extension to the aforementioned document. In reality, this pattern was supposed to be a major focus point of c-iterators. But I realized that I needed another repo to log my ideas and thoughts about this pattern.

You're free to use this pattern and the related ideas discussed in this repository. Although, there is a [LICENSE](./LICENSE) file, I don't expect people to have to include it everywhere. Attribution is all I ask for.

# A brief introduction
Before we move on to implementations, I expect you to be familar with *action based polymorphism*. For the OO programmer, this is an [Interface](https://docs.oracle.com/javase/tutorial/java/concepts/interface.html). For the Functional programmer, this is a [Type class](https://en.wikipedia.org/wiki/Type_class). Though in reality, the implementation is more akin to [Trait objects](https://doc.rust-lang.org/book/ch17-02-trait-objects.html) than real typeclasses.

# Goals
* Type safety - try not to make "user" interfaces (i.e concrete implementations) use `void*`.
* **Full**, and **strict** standard C<sup>[1]</sup> conformance - no hacky strict alias violating shenanigans.

  [1] Core idea supports C90; examples use compound literals (C99) for convenience (not required); further (highly optional) abstractions may require C99 or even C11
* Extensible and usable in libraries (unlike `_Generic`) - possible through dynamic dispatch.
* Open to being used with existing C libraries. Implementing typeclasses should not have special requirements.
* As transparent as possible, especially from a usage perspective.
* Base polymorphism around actions (abilities), not objects.

# Core Idea
*Reference code: [barebones.c](./barebones.c)*

A struct that contains 2 members-
* The concrete type itself, to use with the respective functions (abilities of the type) - `self`
* A vtable containing function pointers to the **exact implementations** of respective abilities for the specific type - `tc`

Feel free to name these members whatever you'd like. I used `self`, since it's widely used in this context, and `tc`, for "type class".

This struct **has** to be polymorphic. As in, the `self` member should be of type `void*`. The bonafide polymorphic type in C. This way, functions can simply ask for this struct to **constrain** types to ones that can *do certain actions*. The function can then call whatever function they need to call through the `tc` member, and pass in the `self` member.

This is what that'd look like in psuedocode-
```c
typedef struct typeclass_name_vtable
{
    ReturnType (*const func_name)(void* self, ...);
    ... /* More "abilities" */
} TypeclassName_vtable;

typedef struct typeclass_name
{
    void* self;
    TypeclassName_vtable const* tc;
} TypeclassName;
```
The latter struct is called a **Typeclass Instance**.

A polymorphic function could then look like-
```c
void poly_foo(TypeclassName x)
{
    /* Use x's abilities here */
    x.tc->func_name(self);
    ...
}
```

Coming back to the real world, you need to actually be able to turn concrete types into a typeclass. For that, you first need the concrete implementations for the abilities required by the typeclass. Once again, this is what that'd be like in psuedocode-
```c
/* Assume `T` is a concrete type */
typedef some_type T;

/* The `func_name` ability (shown above) impl for `T` */
ReturnType T_func_name(T* self, ...);
```

Assuming this is the only ability required by a certain typeclass, you can now make a function to convert `T` to that typeclass-
```c
TypeclassName T_to_TypeclassName(T* x)
{
    static TypeclassName_vtable const tc = {.func_name = (ReturnType (*const)(void*, ...))(T_func_name) };
    return (TypeclassName){.tc = &tc, .self = x};
}
```
This is called the **Implementation Function**. As expected, it accepts a pointer to that concrete type (since it has to be assignable to `void*`), wraps it around its typeclass instance, and returns it. The **lifetime** of the returned struct is the *same as the lifetime of the data pointed to by the given pointer*.

In general, neither this typeclass struct, nor the typeclass functions should take ownership of the concrete type. Though this isn't a forced requirement, just a suggested one.

That's all there is to it! *This* is the typeclass pattern (or interface pattern). These typeclass structs can be used in libraries to have polymorphic functions. The correct functions will be dynamically dispatched to.

**The code snippets above are all psuedocode for a general idea. This core idea is used to define and implement a [`Show`](https://hackage.haskell.org/package/base-4.15.0.0/docs/Text-Show.html#t:Show) typeclass for an `enum` in [barebones.c](./barebones.c).**

# Combining multiple typeclasses/interfaces
*Reference code: [barebones-combined.c](./barebones-combined.c)*

In real world code, you'll require types that implement multiple typeclasses/interfaces. You can encode that by having a struct containing the usual `self` member, and *multiple* vtables - each corresponding to a specific typeclass.

Consider 2 typeclasses- [`Show`](https://hackage.haskell.org/package/base-4.15.0.0/docs/Text-Show.html#t:Show) and [`Enum`](https://hackage.haskell.org/package/base-4.15.0.0/docs/GHC-Enum.html), their vtable structs looks like-
```c
typedef struct
{
    char* (*const show)(void* self);
} ShowTC;

typedef struct
{
    int (*const from_enum)(void* self);
} EnumTC;
```

You can combine these with-
```c
typedef struct
{
    void* self;
    ShowTC const* showtc;
    EnumTC const* enumtc;
} ShowEnum;
```
Now, you have a typeclass instance - that requires *multiple typeclass implementations*. You'd wrap a type into this *combined typeclass instance* by obtaining the `Show` and `Enum` typeclass implementations *for that type* (by calling the implementation functions), extracting the vtables from those instances and putting them into this struct.

If you implemented `Show` for `int`, and named the *implementation function* `int_to_show`, implemented `Enum` for `int`, and named the *implementation function* `int_to_enum`, this whole process would look like-
```c
int x = 42;
ShowEnum shen = { .self = &x, .showtc = int_to_show(&x).tc, .enumtc = int_to_enum(&x).tc };
```
Feel free to generalize this into a function. This concept is showcased in [barebones-combined.c](./barebones-combined.c).

You now have **full**, **type safe**, and **flexible** *polymorphism*. Usable *in any context* where you can use regular types. **Function arguments**, **container elements**, **polymorphic return values** etc. Many of these polymorphic types will be the combination of many typeclasses. Which may feel somewhat dry, and repetitive. However, the core idea is intentionally barebones. You can always design macros around these to make it less dry. Or, you may choose to simply have this completely transparent, your choice.

# For a few macros more
*Reference code: [barebones-macro.c](./barebones-macro.c)*

This pattern, as implemented with maximum transparency above, may seem rather unintuitive for the implementor. It's very easy for the implementor of a typeclass to make mistakes in the way showcased above. You can, instead, make a macro to generalize the implementation-
```c
#define impl_TypeclassName(T, Name, func_name_f)                                                                       \
    TypeclassName Name(T* x)                                                                                           \
    {                                                                                                                  \
        ReturnType (*const func_name_)(T* self, ...) = (func_name_f);                                                  \
        (void)func_name_;                                                                                              \
        static TypeclassName_vtable const tc = {.func_name = (ReturnType (*const)(void*, ...))(func_name_f) };         \
        return (TypeclassName){.tc = &tc, .self = x};                                                                  \
    }
```
This is very similar to the `T_to_TypeclassName` above. But with a touch more "features".
```c
ReturnType (*const func_name_)(T* self, ...) = (func_name_f);
(void)func_name_;
```
These 2 lines are a no-op, but they are very important to the goal of this pattern- *Type safety*. The implementation functions should take in **the exact concrete type**. Implementations are inherently "user targeted" - the user **should not** be using errant void pointers.

The first line ensures the function implementation is the exact type it needs to be, with `void* self` substituted for `T* self`. If you accidentally provided a function implementation with incorrect type, you'll know it.

The second line silences the "unused variable" warning and allows the 2 lines to be a no-op.

Otherwise, the macro is an exact analog of `T_to_TypeclassName`, it takes the concrete type the implementation is for, the name to define this wrapper function as, and all the required function implementations.

You can now simplify the implementation for `T` above-
```c
impl_TypeclassName(T, T_to_TypeclassName, T_func_name)
```

**The code snippets above are all psuedocode for a general idea. This concept is used to define a [`Show`](https://hackage.haskell.org/package/base-4.15.0.0/docs/Text-Show.html#t:Show) typeclass for an `enum` in [barebones-macro.c](./barebones-macro.c).**

# With more constraints
**NOTE**: The ideas and abstractions described in this section **are not** *integral* enough to the actual pattern. If you want maximum transparency, you may safely ignore this.

*Reference code: [name-constrained.c](./name-constrained.c)*

Constraints are a core part of abstractions. With more constraints, you can have *more predictability* - allowing for more "syntax sugar" macros.

If the user is disallowed from choosing their own names for the implementation functions - you get *predictability*. Which allows you to make a macro to wrap a user given type into the necessary typeclass. You'll need to abstract out the impl function naming in the `impl_` macro to have consistent naming-
```c
#define CONCAT_(x, y) x ## y
#define CONCAT(x, y) CONCAT_(x, y)

/* Consistently name the impl functions */
#define ImplName(T, TypeclassName) CONCAT(CONCAT(T, _to_), TypeclassName)

#define impl_TypeclassName(T, func_name_f)                                                                             \
    TypeclassName ImplName(T, TypeclassName)(T* x)                                                                     \
    {                                                                                                                  \
        ReturnType (*const func_name_)(T* self, ...) = (func_name_f);                                                  \
        (void)func_name_;                                                                                              \
        static TypeclassName_vtable const tc = {.func_name = (ReturnType (*const)(void*, ...))(func_name_f) };         \
        return (TypeclassName){.tc = &tc, .self = x};                                                                  \
    }
```

Now, if you implemented `TypeclassName` for your type `T`, the function used to turn `T*` into `TypeclassName` would just be `ImplName(T, TypeclassName)`. With this knowledge, you can abstract out the "wrapping `T` into `TypeclassName`" part-
```c
/* "Apply" a typeclass over a concrete type */
#define ap(x, T, TypeclassName) ImplName(T, TypeclassName)(x)
```

You can now simplify the implementation for `T` *as well as the wrapping*-
```c
impl_TypeclassName(T, T_func_name)

int main(void)
{
    /* Initiate the concrete value */
    T val = ...;
    TypeclassName x = ap(&val, T, TypeclassName);
}
```

**The code snippets above are all psuedocode for a general idea. This concept is used to define a [`Show`](https://hackage.haskell.org/package/base-4.15.0.0/docs/Text-Show.html#t:Show) typeclass for an `enum` in [name-constrained.c](./name-constrained.c).**

# Here be meta-macros
**NOTE**: The ideas and abstractions described in this section **are not** *integral* enough to the actual pattern. If you want maximum transparency, you may safely stop ignore this.

**NOTE**: If you're interested in meta macros fully implemented alongside a very similar pattern - you should check out [interface99](https://github.com/Hirrolot/interface99). Hirrolot does metaprogramming better than I can even imagine!

Ah, meta programming with macros. Remarkable projects like [metalang99](https://github.com/Hirrolot/metalang99), [C99-Lambda](https://github.com/Leushenko/C99-Lambda), [obj.h](https://github.com/small-c/obj.h), [clofn.h](https://github.com/yulon/clofn) and [many more](https://github.com/Hirrolot/awesome-c-preprocessor), really showcase how ridiculously strong (and evil) the C preprocessor can be in the right hands. Surely, meta macros can be used here too? To make some magical abstractions?

Well yes, not entirely sure how magical they would be though. I'm not going to go through the implementations of these - I'd rather just discuss the concepts. Following is a list of "abstractions", wrapping around the typeclass pattern, that you can implement with macros.

## Default implementations
In many cases, typeclasses or interfaces have certain functions that aren't required to be implemented - and instead some default implementation is used. You can do this by having a variadic argument (`...`)<sup>[1]</sup> on the `impl_` macro, representing the optional function implementations. You'll then need macros to *work* with these variadic macros, and figure out which optional functions were provided - and which weren't. This isn't a new concept and is already used in many of the meta macro projects mentioned above. You can also find an implementation of this on [Stack Overflow](https://stackoverflow.com/questions/3046889/optional-parameters-with-c-macros).

For implementations that were provided, you simply use them - as long as they pass typecheck. For ones that weren't provided, you use some default implementation you have defined already.

[1] In standard C, A variadic argument represents *1 or more* arguments. Not *0 or more*. This means that you may have to have a dummy argument to really achieve the "0 or more" arguments concept that you'll need for truly optional arguments.

## Less redundancy for the definer
In general, defining all the typeclasses and its respective `impl_` macro is very similar. By spamming enough meta macros, you should be able to abstract out the defining part completely.

In general, you could have a singular macro to define the vtable and the typeclass instance together - it just needs to know some information about the functions. Next, you need a general `impl` macro. It should be able to deduce information about the functions of a typeclass (possibly through an object like macro), and define a function similar to how the current `impl_` macro does. You'll definitely need [`mapping`](https://github.com/swansontec/map-macro/blob/master/map.h) for this.

# Limitations
1. Polymorphic return types, i.e when the return type is a typeclass instance, generally involve heap allocation. This is because the `self` member is of type- `void*`. You can only assign pointers to it. But you can't assign the address of a local variable since its lifetime ends after the function returns.
2. There's no way to have a function's **return type**, be the *exact same* as a **polymorphic input (argument) type**. This is because there's no way to know *the exact type* wrapped inside a typeclass. You can return the same polymorphic type. But in many cases, this isn't what you'd want.

   Consider addition- `(+) :: Num a => a -> a -> a` - 2 arguments and a return value, all of the same type. As long as the type implements `Num`. If you use `(+)` with `int`s, the return value is an `int`, with `float`s, the return value is a `float`.
  
   You simply can't do this in C, since there's no way to capture those types. You could return the `Num` typeclass instance itself. But the only thing you can (safely) do with that return value, is more `Num` operations.

3. As an extension to the point 2, [Return type polymorphism](https://eli.thegreenplace.net/2018/return-type-polymorphism-in-haskell/) (not to be confused with *polymorphic return types*) is simply not possible (safely). Which means functions like `Enum a => toEnum :: Int -> a` cannot be implemented.
4. It requires extra effort to pass [**combined typeclass instances**](#combining-multiple-typeclassesinterfaces) to functions expecting less typeclass implementations.
  
   Suppose you have the combined typeclass instance `Foo`. It contains the usual `self` member, and vtables for 3 other typeclasses `Atc`, `Btc`, `Ctc`. You want to use this with a function that just wants a type implementing `Atc`. You need to manually extract the `Atc` vtable from `Foo`, the `self` member, and then create soley the `Atc` typeclass instance to be able to use it with the aforementioned function.
5. Type safety, on functions *taking multiple typeclass instance arguments*, but **requiring** those arguments to be backed up by **the same concrete type**, cannot be guaranteed.

   Consider the compare function- `Ord a => compare :: a -> a -> Ordering` (assume `Ordering` is `typedef enum { LT = -1, EQ = 0, GT = 1 } Ordering;`) - 2 polymorphic arguments, *bounded by the `Ord` typeclass*, but **required** to be *the same concrete type*. It wouldn't make sense to compare an `int` with a `char*` - and yet both can be wrapped inside a `Ord` typeclass instance, as long as they implement it. So if you have 2 `Ord` typeclass instances, and you want to call `compare` from one of them, pass in `self` from both `Ord` instances - there's no guarantee that both instances are actually wrapping the same type.

   There's a solution to this though, but you must do it manually. Before calling `compare`, **verify equality** of the `tc` members of both `Ord` instances. If they're wrapping the same type - obtained from their respective implementation function - the typeclass address is the exact same.

# Motivation
It's pretty common for people to ask for polymorphism after they've written enough C. Thankfully, there's no shortage of demonstrations, helper headers, and crazy cool meta macros for implementing OOP polymorphism in C. But I was looking for an action oriented pattern with as much type safety as possible.

Specifically, I wanted something like **Haskell typeclasses**, or **Java/C# interfaces**, or **Rust/Scala traits**. A sort of "interface" that declares a bunch of functions with specific types, leaving in a polymorphic `self` or the like. The exact implementations, for which, a concrete type must fill. This allows you to make polymorphic actions that ask for a type implementing a certain "interface".

The end result, after about a week of experimenting and refining, is this pattern. Over many refinements and re-evaluations, I think the final result is an actually extensible and practical polymorphism pattern based around actions. It's not a perfect encoding of actual typeclasses (especially not the haskell ones - those are extremely high level). But it's probably as good as it gets.
