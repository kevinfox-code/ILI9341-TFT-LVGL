#ifndef DRV_POLICY_H_
#define DRV_POLICY_H_

/* LVGL adapter flush policy knobs. Overridable via compile definitions
 * (CMake cache vars DRV_DROP_FRAME_IF_BUSY / DRV_DMA_BUSY_WAIT_TIMEOUT_MS). */

#ifndef DRV_DROP_FRAME_IF_BUSY
#define DRV_DROP_FRAME_IF_BUSY 1
#endif

#ifndef DRV_DMA_BUSY_WAIT_TIMEOUT_MS
#define DRV_DMA_BUSY_WAIT_TIMEOUT_MS 200u
#endif

#endif /* DRV_POLICY_H_ */
