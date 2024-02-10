/** @file ptmacosx.h portable timer implementation for mac os x. */

#include <stdint.h> // needed for uint64_t

#ifdef __cplusplus
extern "C" {
#endif

uint64_t Pt_CurrentHostTime(void);

uint64_t Pt_NanosToHostTime(uint64_t nanos);

uint64_t Pt_HostTimeToNanos(uint64_t host_time);

uint64_t Pt_MicrosPerHostTick(void);

/** @} */

#ifdef __cplusplus
}
#endif
