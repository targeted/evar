#ifndef EVAR_PREPROC_H
#define EVAR_PREPROC_H

#define _EVAR_QUOTE(A) #A
#define EVAR_QUOTE(A) _EVAR_QUOTE(A)

#define _EVAR_CONCAT(A, B) A ## B
#define EVAR_CONCAT(A, B) _EVAR_CONCAT(A, B)

/*
 * Compile-time assert.
 */
#define EVAR_ASSERT(cond, tag) \
    extern unsigned char EVAR_CONCAT(__EVAR_ASSERT__, tag)[1]; \
    extern unsigned char EVAR_CONCAT(__EVAR_ASSERT__, tag)[(cond) ? 1 : -1]

/*
 * To silence "unused argument" warnings.
 */
#define EVAR_UNUSED(arg) (void)arg

#endif
