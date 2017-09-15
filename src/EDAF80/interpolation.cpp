#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	//! \todo Implement this function
	glm::vec3 p_x = (1 - x) * p0 + x * p1;

	return p_x;
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{
	//! \todo Implement this function
	glm::vec3 p_x0 = p1;
	glm::vec3 p_x1 = (-p0 + p2) * t;
	glm::vec3 p_x2 = p0 * (2 * t) + p1 * (t - 3) + p2 * (3 - 2 * t) - p3 * t;
	glm::vec3 p_x3 = - p0 * t + p1 * (2 - t) + p2 * (t - 2) + p3 * t;

	return p_x0 + x * p_x1 + (float) pow(x, 2) * p_x2 + (float) pow(x, 3) * p_x3;
}
