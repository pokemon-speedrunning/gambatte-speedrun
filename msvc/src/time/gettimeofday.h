#ifndef _GETTIMEOFDAY_H_
#define _GETTIMEOFDAY_H_

#ifdef _MSC_VER

#include <time.h>
#define NOMINMAX
#include <winsock2.h>
#undef NOMINMAX

#ifdef __cplusplus
extern "C" {
#endif
int gettimeofday(struct timeval * tp, struct timezone * tzp);
#ifdef __cplusplus
}
#endif

#endif /* _MSC_VER */

#endif /* _GETTIMEOFDAY_H_ */
