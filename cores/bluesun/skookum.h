#ifndef __SKOOKUM_H__
#define __SKOOKUM_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SKOOKUM_ADC_GROUND 0xFFF

extern void skookumPWM_init(uint32_t pin);
extern void skookumPWM_update(uint32_t pin, uint32_t high, uint32_t period);
extern void skookumADC_init(uint32_t pin);
extern uint32_t skookumADC_sampleDifferential(uint32_t neg, uint32_t pos, uint8_t gain);

#ifdef __cplusplus
}
#endif

#endif
