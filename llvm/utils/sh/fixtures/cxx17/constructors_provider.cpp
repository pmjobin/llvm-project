#include "constructors.hpp"
static int global_count = 0;
static int local_count = 0;
struct Counter { explicit Counter(int *p) : ptr(p) { ++*ptr; } int *ptr; };
static Counter global_initializer(&global_count);
int ctor_global_count() { return global_count; }
int ctor_local_access() { static Counter once(&local_count); return local_count; }
int ctor_local_count() { return local_count; }
