#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	glm::vec2 x_vector{1.0f, x};
	glm::mat2 M{
		1.0f, -1.0f,
		0.0f,  1.0f
	};
	auto P = glm::transpose(glm::mat2x3{p0, p1});
	return x_vector * M * P;
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{
	glm::vec4 x_vector{1.0f, x, glm::pow(x, 2.0f), glm::pow(x, 3.0f)};
	glm::mat4 M{
		0.0f, -t,   2*t,  -t,
		1.0f, 0.0f, t-3,  2-t,
		0.0f, t,    3-2*t, t-2,
		0.0f, 0.0f, -t,    t
	};
	auto P = glm::transpose(glm::mat4x3{p0, p1, p2, p3});
	return x_vector * M * P;
}
