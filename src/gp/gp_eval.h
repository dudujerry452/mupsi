#ifndef _GP_EVAL_H_
#define _GP_EVAL_H_

#include "kernel.h"

namespace mupsi {

extern uint32_t g_gpSeed;

float gp_eval(const Vector3f& pos, const SEKernel& kernel, int points_per_cell, float cell_size);

}

#endif 