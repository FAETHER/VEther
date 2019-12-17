#include "startup.h"
#include "mathlib.h"
#include "render.h"

typedef struct cam_ent_t
{
	vec3_t pos;
	vec3_t angles;
	vec3_t front;
	vec3_t up;
	vec3_t right;
	vec3_t worldup;

	float yaw      ;
	float pitch    ;
	float speed    ;
	float sensitivity ;
	float zoom        ;
	float fovx;
	float fovy;

} cam_ent_t;
extern cam_ent_t cam;

typedef struct sky_ent_t
{
	VkBuffer buffer[3];
	VkDeviceSize buffer_offset[2];
	VkDescriptorSet dset;
	uint32_t uniform_offset;
	unsigned char* vertex_data;
	uint32_t* index_data;
	uint32_t n_indices;
	uint32_t n_vertices;
	UniformSkydome* sky_uniform;
} sky_ent_t;

namespace entity
{
void UpdateCamera();
void InitCamera();
void ViewMatrix(float matrix[16]);

} //namespace entity
