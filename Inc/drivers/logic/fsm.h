#include "angles.h"
#include <math.h>
#include <stdbool.h>


typedef enum { ZONE_FREE, ZONE_COMPLIANT, ZONE_CLAMPED } fsm_state;

typedef struct {
    fsm_state state;
    float  enter_compliant;
    float exit_compliant;   // e.g. 30, 25
    float  enter_clamped;
    float  exit_clamped;     // e.g. 50, 45
} axis_data;

void axis_fsm_update(axis_data *axis, uint16_t *position, float angle);