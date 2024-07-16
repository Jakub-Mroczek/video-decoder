#ifndef _PROFILE_H
#define _PROFILE_H

#include <stdint.h>
#include <stdio.h>

#include "xtime_l.h"

#include "config.h"

#ifdef CONFIG_ENABLE_PROFILE

#define PROFILE_UPDATE(profile_rhs, value_rhs)                        \
    do {                                                              \
        struct profile *profile = profile_rhs;                        \
        uint64_t val = (uint64_t)(value_rhs);                         \
        if (profile->cnt++ == 0) {                                    \
            profile->tot = val;                                       \
            profile->min = val;                                       \
            profile->max = val;                                       \
        } else {                                                      \
            profile->tot += val;                                      \
            profile->min = (val < profile->min) ? val : profile->min; \
            profile->max = (val > profile->max) ? val : profile->max; \
        }                                                             \
    } while(0)

#define PROFILE_US_START                \
    do {                                \
        XTime time1;                    \
        XTime time2;                    \
        XTime bias;                     \
        /* Get null time */             \
        XTime_GetTime((XTime *)&time1); \
        XTime_GetTime((XTime *)&time2); \
        bias = time2 - time1;           \
        /* Get code time */             \
        XTime_GetTime((XTime *)&time1); \
            {

#define PROFILE_US_STOP(profile_rhs)                               \
            }                                                      \
        XTime_GetTime((XTime *)&time2);                            \
        PROFILE_UPDATE(profile_rhs, (time2 - time1 - bias) / 325); \
    } while(0)

#define PROFILE_PRINT_RESULTS(title_str, units_str, profile_rhs) \
	do {                                                         \
		struct profile *profile = profile_rhs;                   \
		printf("\n%s\n", title_str);                             \
		if (profile->cnt) {                                      \
			printf("Tot: %llu", profile->tot);                   \
			printf(" %s\n", units_str);                          \
			printf("Cnt: %llu\n", profile->cnt);                 \
			printf("Avg: %llu", profile->tot / profile->cnt);    \
			printf(" %s\n", units_str);                          \
			printf("Min: %llu", profile->min);                   \
			printf(" %s\n", units_str);                          \
			printf("Max: %llu", profile->max);                   \
			printf(" %s\n", units_str);                          \
		}                                                        \
	} while(0)

#else

#define PROFILE_UPDATE(profile_rhs, value_rhs) ;
#define PROFILE_US_START() ;
#define PROFILE_US_STOP(profile_rhs) ;
#define PROFILE_PRINT_RESULTS(title_str, units_str, profile_rhs) ;

#endif /* CONFIG_ENABLE_PROFILE */

struct profile {
    uint64_t cnt;
    uint64_t tot; /* avg = tot / cnt */
    uint64_t min;
    uint64_t max;
};

#endif /* _PROFILE_H */
