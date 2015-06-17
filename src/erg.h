
#pragma once

/** Threshold for detecting half revolution using ADC */
#define ERG_ADC_HIGH_THRES (150)
#define ERG_ADC_LOW_THRES (25)

void erg_init(void);
void erg_update(void);
float erg_get_omega(void);
float erg_get_b(void);
float erg_get_power(void);
float erg_get_rpm(void);
uint32_t erg_get_time_diff(void);
