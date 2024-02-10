/** @file ptmacosx.h portable timer implementation for mac os x. */

#include <MacTypes.h> // needed for UInt64

#ifdef __cplusplus
extern "C" {
#endif

UInt64 Pt_CurrentHostTime(void);

UInt64 Pt_NanosToHostTime(UInt64 nanos);

UInt64 Pt_HostTimeToNanos(UInt64 host_time);

UInt64 Pt_MicrosPerHostTick(void);

/** @} */

#ifdef __cplusplus
}
#endif
