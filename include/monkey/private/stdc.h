#ifndef MONKEY_PRIVATE_STDC_H_
#define MONKEY_PRIVATE_STDC_H_

#if __STDC_VERSION__ >= 202000L
#define MONKEY_C23(x) x
#define MONKEY_NOT_C23(x)
#else
#define MONKEY_C23(x)
#define MONKEY_NOT_C23(x) x
#endif

#define MONKEY_UNUSED MONKEY_C23([[maybe_unused]]) MONKEY_NOT_C23(__attribute__((unused)))

#ifdef __has_feature
#if __has_feature(undefined_behavior_sanitizer)
#define ALLOW_UINT_OVERFLOW __attribute__((no_sanitize("unsigned-integer-overflow")))
#endif
#endif

#ifndef ALLOW_UINT_OVERFLOW
#define ALLOW_UINT_OVERFLOW
#endif

#define auto __auto_type

#endif  // MONKEY_PRIVATE_STDC_H_
