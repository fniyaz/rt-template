#include "ray_generation.h"

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

RayGenerationApp::RayGenerationApp(short width, short height) :
	width(width),
	height(height)
{
}

RayGenerationApp::~RayGenerationApp()
{
}

void RayGenerationApp::SetCamera(float3 position, float3 direction, float3 approx_up)
{
    camera.SetPosition(position);
    camera.SetDirection(direction);
    camera.SetUp(approx_up);
    camera.SetRenderTargetSize(width, height);
}

void RayGenerationApp::Clear()
{
    frame_buffer.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
}

void RayGenerationApp::DrawScene() {
    for (short x = 0; x < width; x++) {
#pragma omp parallel for
        for (short y = 0; y < height; y++) {
            auto payload = TraceRay(camera.GetCameraRay(x, y), raytracing_depth);
            SetPixel(x, y, payload.color);
        }
    }
}

int RayGenerationApp::Save(std::string filename) const
{
    int result = stbi_write_png(filename.c_str(), width, height, 3, frame_buffer.data(), width * 3);

//    std::system(("open "+filename).c_str());

    return 1 - result;
}

Payload RayGenerationApp::TraceRay(const Ray& ray, const unsigned int max_raytrace_depth) const
{
    return Miss(ray);
}

Payload RayGenerationApp::Miss(const Ray &ray) const {
    auto t = 0.5f * (ray.direction.y + 1);
    float3 color{0.0f, 0.2f, 0.7f + 0.3f * t};
    return Payload{color};
}

void RayGenerationApp::SetPixel(unsigned short x, unsigned short y, float3 color)
{
    frame_buffer[y * width + x] = byte3{255 * color};
}

Camera::Camera()
{
}

Camera::~Camera()
{
}

void Camera::SetPosition(float3 position)
{
    this->position = position;
}

void Camera::SetDirection(float3 direction)
{
    this->direction = linalg::normalize(direction - position);
}

void Camera::SetUp(float3 approx_up) {
    right = linalg::normalize(linalg::cross(direction, approx_up));
    up = linalg::normalize(linalg::cross(direction, right));
}

void Camera::SetRenderTargetSize(short width, short height) {
    this->height = height;
    this->width = width;
}

Ray Camera::GetCameraRay(short x, short y) const {
    float3 nd = direction;
    float3 dx = (2 * (static_cast<float>(x) + 0.5f) / static_cast<float>(width) - 1) * right *
            static_cast<float>(width) / static_cast<float>(height);
    float3 dy = (2 * (static_cast<float>(y) + 0.5f) / static_cast<float>(height) - 1) * up;

    return Ray(position, nd + dx + dy);
}

Ray Camera::GetCameraRay(short x, short y, float3 jitter) const {
    // todo deal with it
    return GetCameraRay(x, y);
}
