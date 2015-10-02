
#pragma once

/** Threshold for detecting half revolution using ADC */
#define ERG_ADC_HIGH_THRES (150)
#define ERG_ADC_LOW_THRES (25)

struct erg_status_s
{
    float elapsed_time; //< sec
    float distance; //< m
    float strokes_per_min; //< s/min
    float drag_factor; //< 1E-6 Nms^2
    float split; //< sec (time for 500m)
    float average_split; //< sec (time for 500m)
    //float power; //< W
    //float energy; //< kCal
};

void erg_init(struct erg_status_s * status);
void erg_update_status(struct erg_status_s * status);
