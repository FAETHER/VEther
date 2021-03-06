#include "window.h"
#include "control.h"
#include "shaders.h"
#include "textures.h"
#include "draw.h"
#include "flog.h"
#include "entity.h"
#include "cvar.h"

#include <mutex>
#include <condition_variable>

/* {
GVAR: framebufferCount -> render.cpp
GVAR: imageViewCount -> render.cpp
GVAR: imageViews -> render.cpp
GVAR: number_of_swapchain_images -> swapchain.cpp
GVAR: logical_device -> startup.cpp
GVAR: allocators -> startup.cpp
GVAR: pipeline_layout -> render.cpp
GVAR: pipelines -> render.h
GVAR: dyn_vertex_buffers -> control.cpp
GVAR: current_dyn_buffer_index -> control.cpp
GVAR: NUM_COMMAND_BUFFERS -> control.cpp
} */

//{
GLFWwindow* _window;
VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
VkSwapchainKHR old_swapchain = VK_NULL_HANDLE;
VkSemaphore AcquiredSemaphore;
VkSemaphore ReadySemaphore;
VkFence Fence_one;
VkQueue GraphicsQueue;
VkQueue ComputeQueue;
int window_width = 1280;
int window_height = 960;
uint32_t graphics_queue_family_index; // <- this is queue 1
uint32_t compute_queue_family_index; // <- this is queue 2
double time1 = 0;
double y_wheel = 0;
double xm_norm = 0;
double ym_norm = 0;
int32_t xm = 0;
int32_t ym = 0;
double frametime = 0;
double deltatime = 0;
double lastfps = 0;
double realtime = 0;
float aspectRatio;
mu_Context* ctx;
//}

static bool mfocus = false;
static VkClearValue clearColor[2] = {};
static VkClearColorValue color = {};
static VkImageSubresourceRange range = {};
static VkImageMemoryBarrier image_memory_barrier_before_draw = {};
static VkImageMemoryBarrier image_memory_barrier_before_present = {};
static VkSubmitInfo submit_info = {};
static VkPresentInfoKHR present_info = {};
static uint32_t image_index;
static VkPipelineStageFlags flags = {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
static std::mutex mtx[2]; //locking
static std::condition_variable cvx[2]; //signaling
static bool run_threads[2] = {};
static bool quit = false;
static double frameCpuAvg = 0;
static double frameGpuAvg = 0;

namespace window
{

void window_size_callback(GLFWwindow* _window, int width, int height)
{
	//handle minimization.
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(_window, &width, &height);
		glfwWaitEvents();
	}

	VK_CHECK(vkDeviceWaitIdle(logical_device));

	old_swapchain = _swapchain;
	_swapchain = VK_NULL_HANDLE;
	if(
	    !surface::CheckPresentationSurfaceCapabilities()||
	    !swapchain::SetSizeOfSwapchainImages(width, height)||
	    !swapchain::CreateSwapchain(_swapchain, old_swapchain)||
	    !swapchain::GetHandlesOfSwapchainImages(_swapchain)
	)
	{
		startup::debug_pause();
		exit(1);
	}

	window_height = height;
	window_width = width;

	for(uint32_t i = 0; i<number_of_swapchain_images; i++)
	{
		vkDestroyImageView(logical_device, imageViews[i], nullptr);
	}
	uint32_t tmpc = imageViewCount;
	imageViewCount = 0;
	render::CreateSwapchainImageViews();

	render::DestroyFramebuffers();
	framebufferCount = 0;

	render::CreateDepthBuffer();
	imageViewCount = tmpc;

	for(uint16_t i = 0; i<number_of_swapchain_images; i++)
	{
		render::CreateFramebuffer(&imageViews[i], &imageViews[number_of_swapchain_images], 0, window_width, window_height);
	}
}

void keyCallback(GLFWwindow* _window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch(key)
		{
		case GLFW_KEY_ESCAPE:
			trace("Quiting cleanly");
			glfwSetWindowShouldClose(_window, true);
			break;
		case GLFW_KEY_BACKSPACE:
			mu_input_keydown(ctx, 8);
			break;
		case GLFW_KEY_ENTER:
			mu_input_keydown(ctx, 16);
			break;
		}
		if(!ctx->focus)
		{
			switch(key)
			{
			case GLFW_KEY_M:

				if(!mfocus)
				{
					info("Mouse focused.");
					glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
					mfocus = true;
				}
				else
				{
					info("Default mouse.");
					glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
					mfocus = false;
				}
				break;
			case GLFW_KEY_E:
				mesh_ent_t* target = entity::InstanceMesh("./res/kitty.obj");
				vec3_t pos = {cam.pos[0], cam.pos[1], cam.pos[2]};
				entity::SetPosition(target, pos);
				target->rigidBody->setLinearVelocity(btVector3(cam.front[0]*5, cam.front[1]*5, cam.front[2]*5));
				//spawn and shoot the model at index 0 for fun, test things ...
				break;

			}
		}
	}
}

void processInput()
{
	if(!ctx->focus)
	{
		float velocity = cam.speed * frametime;
		if (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS)
		{
			cam.pos[0] += cam.front[0] * velocity;
			cam.pos[1] += cam.front[1] * velocity;
			cam.pos[2] += cam.front[2] * velocity;
		}
		if (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS)
		{
			cam.pos[0] -= cam.front[0] * velocity;
			cam.pos[1] -= cam.front[1] * velocity;
			cam.pos[2] -= cam.front[2] * velocity;
		}
		if (glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS)
		{
			cam.pos[0] -= cam.right[0] * velocity;
			cam.pos[1] -= cam.right[1] * velocity;
			cam.pos[2] -= cam.right[2] * velocity;
		}
		if (glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS)
		{
			cam.pos[0] += cam.right[0] * velocity;
			cam.pos[1] += cam.right[1] * velocity;
			cam.pos[2] += cam.right[2] * velocity;
		}
	}
}
btVector3 rayFrom;
btVector3 rayTo;

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	static float lastX = window_height / 2.0f;
	static float lastY = window_width / 2.0f;
//	printf("Xpos: %.6f \n", xpos);
//	printf("Ypos: %.6f \n", ypos);
	//xm_norm = (xpos - ((window_width - 0.001) / 2)) / ((window_width - 0.001) / 2);
	//ym_norm = (ypos - ((window_height - 0.001) / 2)) / ((window_height - 0.001) / 2);
	xm_norm = ((float)xpos/(float)window_width  - 0.5f) * 2.0f;
	ym_norm = ((float)ypos/(float)window_height - 0.5f) * 2.0f;

	//p("%f", ((xm_norm / 2.0f) * window_width) + (0.5f * window_width));

	xm = (int32_t)xpos;
	ym = (int32_t)ypos;
//	p("Xpos: %d", xm);
//	p("Ypos: %d", ym);
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	xoffset *= cam.sensitivity;
	yoffset *= cam.sensitivity;

	cam.yaw   += xoffset;
	cam.pitch -= yoffset;

//p("%f", cam.pitch);
//p("%f", cam.yaw);

	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (cam.pitch > 89.0f)
		cam.pitch = 89.0f;
	if (cam.pitch < -89.0f)
		cam.pitch = -89.0f;
	entity::UpdateCamera();

	mu_input_mousemove(ctx, xm, ym);

	//btVector3 rayTo = entity::GetRayTo(int(xm), int(ym));
	//btVector3 rayFrom(cam.pos[0], cam.pos[1], cam.pos[2]);
	//entity::MovePickedBody(rayFrom, rayTo);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	y_wheel += yoffset;
	if (cam.zoom >= 1.0f && cam.zoom <= 45.0f)
		cam.zoom -= yoffset;
	if (cam.zoom <= 1.0f)
		cam.zoom = 1.0f;
	if (cam.zoom >= 45.0f)
		cam.zoom = 45.0f;

	mu_input_scroll(ctx, 0, (int)yoffset * -30);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	switch(action)
	{
	case GLFW_PRESS:
		if (button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			mu_input_mousedown(ctx, xm, ym, 2);
		}
		else
		{
			mu_input_mousedown(ctx, xm, ym, 1);
			mesh_ent_t* m = entity::GetMesh("./res/kitty.obj", nullptr);
			btTransform transform;
			m->rigidBody->getMotionState()->getWorldTransform(transform);
			btVector3 origin = transform.getOrigin();

			float invview[16];
			float invproj[16];
			float invmvp[16];
			float scrw_matrix[16];
			//	InvertMatrix(inv, cam.proj);
			//	vec4_t v;
			//	v[0] = xm_norm;
			//	v[1] = ym_norm;
			//	v[2] = -1.0f;
			//	v[3] = 1.0f;
			//	Vec4Xmat4(v, inv);
			//
			InvertMatrix(invview, cam.view);
			InvertMatrix(invproj, cam.proj);
			InvertMatrix(invmvp, cam.mvp);
			zone::Q_memcpy(scrw_matrix, invview, sizeof(float)*16);
			MatrixMultiply(scrw_matrix, invproj);




			//PrintMatrix(invview, "invview");
			//PrintMatrix(invproj, "invproj");
			//PrintMatrix(cam.view, "view");
			//PrintMatrix(cam.proj, "proj");
			//PrintMatrix(scrw_matrix, "comb");
			//PrintMatrix(invmvp, "invmvp");

//			vec4_t near;
//			near[0] = xm_norm;
//			near[1] = ym_norm;
//			near[2] = -1.0f;
//			near[3] = 1.0f;
//
//
//			vec4_t far;
//			far[0] = xm_norm;
//			far[1] = ym_norm;
//			far[2] = 0.0f;
//			far[3] = 1.0f;
//
//			Vec4Xmat4(near, invmvp);
//			Vec4Xmat4(far, invmvp);
//
//			PrintVec3(near);
//			PrintVec3(far);
//
//			vec3_t dir;
//			dir[0] = far[0] - near[0];
//			dir[1] = far[1] - near[1];
//			dir[2] = far[2] - near[2];
//
//			VectorNormalize(dir);

	//		vec4_t r;
	//		r[0] = clip[0] - cam.pos[0];
	//		r[1] = clip[1] - cam.pos[1];
	//		r[2] = clip[2] - cam.pos[2];

			rayFrom = btVector3(cam.pos[0], cam.pos[1], cam.pos[2]);
			//rayTo = btVector3(near[0]+(dir[0]*1000), near[1]+(dir[1]*1000), near[2]+(dir[2]*1000));
			//rayFrom = btVector3(clip[0]+cam.pos[0], clip[1]+cam.pos[1], clip[2]+cam.pos[2]);


vec4_t fo;

fo[0] = 0;
 fo[1] = 0;
 fo[2] = 0;
fo[3] = 1;

 WorldToScreen(fo);

 PrintVec3(fo);

			vec3_t f;
			if(cam.pitch < 0)
				{
					f[0] = cos(DEG2RAD(cam.yaw)) * cos(DEG2RAD(-89.0 - cam.pitch));
					f[1] = sin(DEG2RAD(-89.0f - cam.pitch));
					f[2] = sin(DEG2RAD(cam.yaw)) * cos(DEG2RAD(-89.0f - cam.pitch));
				}
			//rayTo = btVector3(cam.front[0]*100, cam.front[1]*100, cam.front[2]*100);
			//rayTo = btVector3(0,0,0);
			rayTo = btVector3(f[0]*100, f[1]*100, f[2]*100);
			//rayTo = entity::GetRayTo(int(xm), int(ym));
			entity::PickBody(rayFrom, rayTo);
		}
		break;
	case GLFW_RELEASE:
		if (button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			mu_input_mouseup(ctx, xm, ym, 2);
		}
		else
		{
			mu_input_mouseup(ctx, xm, ym, 1);
			entity::RemovePickingConstraint();
		}
		break;
	}
}

void character_callback(GLFWwindow* window, unsigned int codepoint)
{
	char k[1];
	k[0] = codepoint;
	mu_input_text(ctx, k);
}

void initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	_window = glfwCreateWindow(window_width, window_height, "VEther", nullptr, nullptr);
	glfwSetWindowSizeCallback(_window, window_size_callback);
	glfwSetKeyCallback(_window, keyCallback);
	glfwSetCursorPosCallback(_window, cursor_position_callback);
	glfwSetScrollCallback(_window, scroll_callback);
	glfwSetMouseButtonCallback(_window, mouse_button_callback);
	glfwSetCharCallback(_window, character_callback);
}

static  char logbuf[64000];
static   int logbuf_updated = 0;
static char input_buf[128];
static int submitted = 0;

static void write_log(char *text)
{
	if(text[0])
	{
		int len = zone::Q_sstrlen(text);
		text[len] = '\0';
		logbuf_updated = 1;
		if(!Cvar_Set(text, &text[len+1]))
		{
			len = zone::Q_strlen(text);
			char s[] = ": command not found.\n";
			zone::Q_memcpy(text+len, s, ARRAYSIZE(s));
			zone::Q_strcat(logbuf, text);
			return;
		}
		zone::Q_strcat(logbuf, text);
		zone::Q_strcat(logbuf, "\n");
	}
}

	void ConsoleCvarCheck()
	{
		if (submitted)
			{
				write_log(input_buf); //must be on main thread, because of callbacks.
				input_buf[0] = '\0';
			}
	}

static void console(mu_Context *ctx)
{
	static mu_Container window;

	/* init window manually so we can set its position and size */
	if (!window.inited)
	{
		mu_init_window(ctx, &window, 0);
		window.rect = mu_rect(350, 40, 300, 200);
	}

	if (mu_begin_window(ctx, &window, "Console"))
	{

		/* output text panel */
		static mu_Container panel;
		int width[] = {-1};
		mu_layout_row(ctx, 1, width, -28);
		mu_begin_panel(ctx, &panel);
		mu_layout_row(ctx, 1, width, -1);
		mu_text(ctx, logbuf);
		mu_end_panel(ctx);
		if (logbuf_updated)
		{
			panel.scroll.y = panel.content_size.y;
			logbuf_updated = 0;
		}

		/* input textbox */
		int _width[] = { -1, -1 };
		mu_layout_row(ctx, 2, _width, 0);
		if (mu_textbox(ctx, input_buf, sizeof(input_buf)) & MU_RES_SUBMIT)
		{
			mu_set_focus(ctx, ctx->last_id);
			submitted = 1;
		}

		mu_end_window(ctx);
	}
}


static int uint8_slider(mu_Context *ctx, unsigned char *value, int low, int high)
{
	static float tmp;
	mu_push_id(ctx, &value, sizeof(value));
	tmp = *value;
	int res = mu_slider_ex(ctx, &tmp, low, high, 0, "%.0f", MU_OPT_ALIGNCENTER);
	*value = tmp;
	mu_pop_id(ctx);
	return res;
}

static void style_window(mu_Context *ctx)
{
	static mu_Container window;

	/* init window manually so we can set its position and size */
	if (!window.inited)
	{
		mu_init_window(ctx, &window, 0);
		window.rect = mu_rect(350, 250, 300, 240);
	}

	static struct
	{
		const char *label;
		int idx;
	} colors[] =
	{
		{ "text:",         MU_COLOR_TEXT        },
		{ "border:",       MU_COLOR_BORDER      },
		{ "windowbg:",     MU_COLOR_WINDOWBG    },
		{ "titlebg:",      MU_COLOR_TITLEBG     },
		{ "titletext:",    MU_COLOR_TITLETEXT   },
		{ "panelbg:",      MU_COLOR_PANELBG     },
		{ "button:",       MU_COLOR_BUTTON      },
		{ "buttonhover:",  MU_COLOR_BUTTONHOVER },
		{ "buttonfocus:",  MU_COLOR_BUTTONFOCUS },
		{ "base:",         MU_COLOR_BASE        },
		{ "basehover:",    MU_COLOR_BASEHOVER   },
		{ "basefocus:",    MU_COLOR_BASEFOCUS   },
		{ "scrollbase:",   MU_COLOR_SCROLLBASE  },
		{ "scrollthumb:",  MU_COLOR_SCROLLTHUMB },
		{ NULL, 0 }
	};

	if (mu_begin_window(ctx, &window, "Style Editor"))
	{
		int sw = mu_get_container(ctx)->body.w * 0.14;
		int widths[] = { 80, sw, sw, sw, sw, -1 };
		mu_layout_row(ctx, 6, widths, 0);
		for (int i = 0; colors[i].label; i++)
		{
			mu_label(ctx, colors[i].label);
			uint8_slider(ctx, &ctx->style->colors[i].r, 0, 255);
			uint8_slider(ctx, &ctx->style->colors[i].g, 0, 255);
			uint8_slider(ctx, &ctx->style->colors[i].b, 0, 255);
			uint8_slider(ctx, &ctx->style->colors[i].a, 0, 255);
			mu_draw_rect(ctx, mu_layout_next(ctx), ctx->style->colors[i]);
		}
		mu_end_window(ctx);
	}
}

void UIThread()
{
	for(;;)
	{
		// Wait until Draw() signals
		std::unique_lock<std::mutex> lk(mtx[1]);
		cvx[1].wait(lk, [] {return run_threads[1];});

		if(quit)
		{
			return;
		}
		mu_Command* cmd = nullptr;
		mu_begin(ctx);
		console(ctx);
		style_window(ctx);
		mu_end(ctx);

		//record ui commands.
		while (mu_next_command(ctx, &cmd))
		{
			switch (cmd->type)
			{
			case MU_COMMAND_TEXT:
				draw::Text(cmd->text.str, cmd->text.pos, cmd->text.color);
				break;
			case MU_COMMAND_RECT:
				draw::Rect(cmd->rect.rect, cmd->rect.color);
				break;
			case MU_COMMAND_ICON:
				draw::Icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color);
				break;
			}
		}

		if(mfocus)
		{
			draw::Cursor();
		}
		draw::Stats();

		VkCommandBufferInheritanceInfo cbii;
		cbii.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		cbii.pNext = nullptr;
		cbii.renderPass = renderPasses[0];
		cbii.subpass = 0;
		cbii.framebuffer = framebuffers[image_index];
		cbii.occlusionQueryEnable = VK_FALSE;
		cbii.queryFlags = 0;
		cbii.pipelineStatistics = 0;

		command_buffer = scommand_buffers[1];
		control::BeginCommandBufferRecordingOperation(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &cbii);

		VkViewport viewport = {0, 0, float(window_width), float(window_height), 0, 1 };
		VkRect2D scissor = { {0, 0}, {uint32_t(window_width), uint32_t(window_height)} };

		vkCmdSetViewport(command_buffer, 0, 1, &viewport);
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);
		vkCmdPushConstants(command_buffer, pipeline_layout[0], VK_SHADER_STAGE_VERTEX_BIT,
		                   16 * sizeof(float), sizeof(uint32_t), &window_width);
		vkCmdPushConstants(command_buffer, pipeline_layout[0], VK_SHADER_STAGE_VERTEX_BIT,
		                   16 * sizeof(float) + sizeof(uint32_t), sizeof(uint32_t), &window_height);

		draw::PresentUI();

		VK_CHECK(vkEndCommandBuffer(command_buffer));
		run_threads[1] = false;
		lk.unlock();
		cvx[1].notify_one();
	}
}

void Main3DThread()
{
	for(;;)
	{

		std::unique_lock<std::mutex> lk(mtx[0]);
		cvx[0].wait(lk, [] {return run_threads[0];});
		if(quit)
		{
			return;
		}
		VkCommandBufferInheritanceInfo cbii;
		cbii.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		cbii.pNext = nullptr;
		cbii.renderPass = renderPasses[0];
		cbii.subpass = 0;
		cbii.framebuffer = framebuffers[image_index];
		cbii.occlusionQueryEnable = VK_FALSE;
		cbii.queryFlags = 0;
		cbii.pipelineStatistics = 0;

		command_buffer = scommand_buffers[0];
		control::BeginCommandBufferRecordingOperation(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &cbii);

		VkViewport viewport = {0, 0, float(window_width), float(window_height), 0, 1 };
		VkRect2D scissor = { {0, 0}, {uint32_t(window_width), uint32_t(window_height)} };

		vkCmdSetViewport(command_buffer, 0, 1, &viewport);
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);
		aspectRatio = float(window_width) / float(window_height),
		Perspective(cam.proj, DEG2RAD(cam.zoom), aspectRatio, 0.01f, 1000.0f); //projection matrix.
		entity::ViewMatrix(cam.view);
		zone::Q_memcpy(cam.mvp, cam.proj, 16*sizeof(float));
		MatrixMultiply(cam.mvp, cam.view);
		vkCmdPushConstants(command_buffer, pipeline_layout[0], VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), &cam.mvp);

		draw::CameraVectors();
		basic_ent_t line;
		line.pidx = 4;
		vec3_t to;
		vec3_t from;
		vec3_t col;
		col[0] = 1.0f;
		col[1] = 0.0f;
		col[2] = 0.0f;
		Btof(rayFrom, from);
		Btof(rayTo, to);
		draw::Line(from, to, col, line);
		//draw::Meshes();
		draw::SkyDome();

		VK_CHECK(vkEndCommandBuffer(command_buffer));

		run_threads[0] = false;
		lk.unlock();
		cvx[0].notify_one();
	}
}

void PreDraw()
{
	std::thread (UIThread).detach();
	std::thread (Main3DThread).detach();

	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
	render::CreateRenderPass(depthFormat, false);
	render::CreateSwapchainImageViews();

	render::CreateDepthBuffer();

	for(uint16_t i = 0; i<number_of_swapchain_images; i++)
	{
		render::CreateFramebuffer(&imageViews[i], &imageViews[number_of_swapchain_images], 0, window_width, window_height);
	}

	shaders::CompileShaders();
	draw::InitUI();
	draw::InitAtlasTexture();

	render::CreatePipelineLayout();

	shaders::CreatePipelineCache();
	shaders::LoadShaders();

	InitRandSeed(12345);

	//fov setup.
	entity::InitCamera();
	entity::InitPhysics();
	entity::InitMeshes();
	//entity::SetupWorldPlane(50.0f);

	for(int i = 0; i<10; i++)
	{
		entity::InstanceMesh("./res/kitty.obj");
	}

	/* init microui */
	ctx = (mu_Context*) zone::Hunk_AllocName(sizeof(mu_Context), "ctx");
	mu_init(ctx);
	ctx->text_width = draw::text_width;
	ctx->text_height = draw::text_height;

	draw::InitSkydome();

	color.float32[0] = 0;
	color.float32[1] = 0;
	color.float32[2] = 0;
	color.float32[3] = 1;
	clearColor[0].color = color;
	clearColor[1].depthStencil = {1.0f, 0};

	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = VK_REMAINING_MIP_LEVELS;
	range.baseArrayLayer = 0;
	range.layerCount = VK_REMAINING_ARRAY_LAYERS;

	image_memory_barrier_before_draw.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier_before_draw.pNext = nullptr;
	image_memory_barrier_before_draw.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	image_memory_barrier_before_draw.dstAccessMask = 0;
	image_memory_barrier_before_draw.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_memory_barrier_before_draw.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	image_memory_barrier_before_draw.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier_before_draw.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier_before_draw.subresourceRange = range;

	image_memory_barrier_before_present.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier_before_present.pNext = nullptr;
	image_memory_barrier_before_present.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	image_memory_barrier_before_present.dstAccessMask = 0;
	image_memory_barrier_before_present.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_memory_barrier_before_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	image_memory_barrier_before_present.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier_before_present.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier_before_present.subresourceRange = range;

	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &AcquiredSemaphore;
	submit_info.pWaitDstStageMask = &flags;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &ReadySemaphore;

	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = nullptr;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &ReadySemaphore;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &_swapchain;
	present_info.pImageIndices = &image_index;
	present_info.pResults = nullptr;

}

void AwakeWorkers()
{
	for(uint8_t i = 0; i<ARRAYSIZE(mtx); i++)
	{
		{
			std::lock_guard<std::mutex> lk(mtx[i]);
			run_threads[i] = true;
		}
		cvx[i].notify_one();
	}
}

void WaitForWorkers()
{
	for(uint8_t i = 0; i<ARRAYSIZE(mtx); i++)
	{
		std::unique_lock<std::mutex> post_lk(mtx[i]);
		cvx[i].wait(post_lk, [&] {return !run_threads[i];});
	}
}

void DebugTimingInTitle()
{
	uint64_t queryResults[2];
	VK_CHECK(vkGetQueryPoolResults(logical_device, queryPool, 0, ARRAYSIZE(queryResults),
	                               sizeof(queryResults), queryResults, sizeof(queryResults[0]), VK_QUERY_RESULT_64_BIT));

	double frameGpuBegin = double(queryResults[0]) * device_properties.limits.timestampPeriod * 1e-6;
	double frameGpuEnd = double(queryResults[1]) * device_properties.limits.timestampPeriod * 1e-6;

	double frameCpuEnd = glfwGetTime() * 1000;

	frameCpuAvg = frameCpuAvg * 0.95 + (frameCpuEnd - (time1*1000)) * 0.05;
	frameGpuAvg = frameGpuAvg * 0.95 + (frameGpuEnd - frameGpuBegin) * 0.05;

	char title[256];
	sprintf(title, "cpu: %.2f ms; gpu: %.2f ms; ", frameCpuAvg, frameGpuAvg);
	glfwSetWindowTitle(_window, title);
}

inline uint8_t Draw()
{
	VkResult result;

	vkResetFences(logical_device, 1, &Fence_one);
	uint8_t ret = swapchain::AcquireSwapchainImage(_swapchain, AcquiredSemaphore, VK_NULL_HANDLE, image_index);

	while(ret == 2)
	{
		ret = swapchain::AcquireSwapchainImage(_swapchain, AcquiredSemaphore, VK_NULL_HANDLE, image_index);
		glfwPollEvents();
	}
	if(!ret)
	{
		return 0;
	}

	processInput();
	entity::UpdateCamera();

	control::BeginCommandBufferRecordingOperation(0, nullptr);

	ConsoleCvarCheck();

#ifdef DEBUG
	vkCmdResetQueryPool(command_buffer, queryPool, 0, 128);
	vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 0);
#endif

	VkRect2D render_area = {};
	render_area.extent.width = window_width;
	render_area.extent.height = window_height;

	image_memory_barrier_before_draw.image = handle_array_of_swapchain_images[image_index];

	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                     0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier_before_draw);

	render::StartRenderPass(render_area, &clearColor[0], VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS, 0, image_index);

	AwakeWorkers();
	entity::StepPhysics();
	WaitForWorkers();

	control::SetCommandBuffer(current_cmd_buffer_index);
	vkCmdExecuteCommands(command_buffer, 2, &scommand_buffers[0]);
	vkCmdEndRenderPass(command_buffer);

	image_memory_barrier_before_present.image = handle_array_of_swapchain_images[image_index];

	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	                     0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier_before_present);
#ifdef DEBUG
	vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 1);
#endif
	VK_CHECK(vkEndCommandBuffer(command_buffer));

	VK_CHECK(vkQueueSubmit(GraphicsQueue, 1, &submit_info, Fence_one));

	control::ResetStagingBuffer();
	//textures::SampleTextureUpdate();

	result = vkQueuePresentKHR(GraphicsQueue, &present_info);

	vkWaitForFences(logical_device, 1, &Fence_one, VK_TRUE, UINT64_MAX);
#ifdef DEBUG
	DebugTimingInTitle();
#endif
	switch(result)
	{
	case VK_SUCCESS:
		return 1;
	case VK_ERROR_OUT_OF_DATE_KHR:
		glfwPollEvents();
		return 1;
	default:
		printf(startup::GetVulkanResultString(result));
		printf("\n");
		return 0;
	}
	return 1;
}

void mainLoop()
{
	double time2 = 0;
	double maxfps;
	double oldrealtime = 0;
	double oldtime = 0;
	double elapsedtime = 0;
	double stamp = 0;
	int framecount = 0;
	int oldframecount = 0;
	int frames = 0;

	while (!glfwWindowShouldClose(_window))
	{
		//char* a = new char[10000];
		glfwPollEvents();
		time1 = glfwGetTime();
		deltatime = time1 - time2;
		realtime += deltatime;
		maxfps = CLAMP (10.0, 60.0, 1000.0); //60 fps
		if(realtime - oldrealtime < 1.0/maxfps)
		{
			goto c; //framerate is too high
		}
		frametime = realtime - oldrealtime;
		elapsedtime = realtime - oldtime;
		frames = framecount - oldframecount;

		
		
		if (elapsedtime < 0 || frames < 0)
		{
			oldtime = realtime;
			oldframecount = framecount;
			goto wait;
		}

		if (elapsedtime > 0.75) // update value every 3/4 second
		{
			shaders::fileWatcher->update();
			lastfps = frames / elapsedtime;
			oldtime = realtime;
			oldframecount = framecount;
		}
#ifdef DEBUG
		if (realtime-stamp > 60.0)
		{
			std::thread (zone::MemPrint).detach();
			stamp = realtime;
		}
#endif
wait:
		oldrealtime = realtime;
		frametime = CLAMP (0.0001, frametime, 0.1);

		if(!Draw())
		{
			fatal("Critical Error! Abandon the ship.");
			break;
		}
		framecount++;
c:
		if(deltatime < 0.02f)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		time2 = time1;
	}

	//CLEANUP -----------------------------------
	quit = true;
	AwakeWorkers();
	VK_CHECK(vkDeviceWaitIdle(logical_device));
	vkDestroyQueryPool(logical_device, queryPool, allocators);
	control::FreeCommandBuffers(NUM_COMMAND_BUFFERS);
	render::DestroyDepthBuffer();
	control::DestroyCommandPool();
	control::DestroyDynBuffers();
	control::DestroyStagingBuffers();
	control::DestroyUniformBuffers();
	control::DestroyIndexBuffers();
	control::DestroyVramHeaps();
	textures::TexDeinit();
	vkDestroySemaphore(logical_device, AcquiredSemaphore, allocators);
	vkDestroySemaphore(logical_device, ReadySemaphore, allocators);
	vkDestroyFence(logical_device, Fence_one, allocators);
	render::DestroyImageViews();
	render::DestroyFramebuffers();
	vkDestroyPipelineLayout(logical_device, pipeline_layout[0], allocators);
	vkDestroyPipelineLayout(logical_device, pipeline_layout[1], allocators);
	render::DestroyPipeLines();
	render::DestroyRenderPasses();
	shaders::DestroyShaders();
	swapchain::DestroySwapchain(_swapchain);
	surface::DestroyPresentationSurface();
	glfwDestroyWindow(_window);
}

} //namespace window
