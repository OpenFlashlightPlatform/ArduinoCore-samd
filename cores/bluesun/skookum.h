#ifndef __SKOOKUM_H__
#define __SKOOKUM_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SKOOKUM_ADC_GROUND 0xFFF
#define SKOOKUM_GAIN_1X 0
#define SKOOKUM_GAIN_2X 1
#define SKOOKUM_GAIN_4X 2
#define SKOOKUM_GAIN_8X 3
#define SKOOKUM_GAIN_16X 4
#define SKOOKUM_GAIN_HALF 15
extern void skookumPWM_init(uint32_t pin);
extern void skookumPWM_update(uint32_t pin, uint32_t high, uint32_t period);
extern void skookumADC_init(uint32_t pin);
extern uint32_t skookumADC_sampleDifferential(uint32_t neg, uint32_t pos, uint8_t gain);

#ifdef __cplusplus
}
#endif

#endif
