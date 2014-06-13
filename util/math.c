#include "util/math.h"

float math_degrees_to_radians(float f)
{
	return f * (PI / 180.0);
}

void math_unit_vector(float yaw, float pitch, float *x, float *y, float *z)
{
	yaw = math_degrees_to_radians(yaw);
	pitch = math_degrees_to_radians(pitch);

	*x = -cos(pitch) * sin(yaw);
	*y = -sin(pitch);
	*z = cos(pitch) * cos(yaw);
}

