// TODO: Diffuse lighting and specular highlights

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

// Sleep
#include <chrono>
#include <thread>

// notcurses for terminal rendering and keyboard input (https://github.com/dankamongmen/notcurses)
#include <notcurses/notcurses.h>

constexpr float RGB_MAX_FLOAT = 255.0f;

constexpr float FOV = 90.0f; // The zoom
constexpr float SPECULAR_SHININESS = 32.0f; // Specular shininess factor (higher is smaller/brighter highlights)

using namespace std; // TODO: remove

constexpr float degToRad(const float degrees) {
	return degrees * M_PI / 180.0f;
}

struct Pixel {
	u_char r, g, b;

	Pixel() : Pixel{ 0, 0, 0 } {}
	Pixel(const u_char r, const u_char g, const u_char b) : r{ r }, g{ g }, b{ b } {}
};

// Struct that holds image data and renders the image
struct Display3D {
	vector<Pixel> flattenedPixels;
	size_t width;
	size_t height;
	struct ncplane* plane;

	// Width is multiplied by 2 since we are using 2:1 tall rectangular pixels
	Display3D(const size_t w, const size_t h, ncplane* p) : width{ w * 2 }, height{ h }, plane{ p } {
		// Initialize with black pixels
		flattenedPixels.resize(width * height, Pixel{ 0, 0, 0 });
	}

	void clear() {
		fill(flattenedPixels.begin(), flattenedPixels.end(), Pixel{ 0, 0, 0 });
		ncplane_erase(plane); // Clear the notcurses plane
	}

	size_t getNumRows() const {
		return height;
	}

	size_t getNumCols() const {
		return width;
	}

	// Return a reference to the pixel we can modify
	Pixel& pixelAt(const size_t row, const size_t col) {
		return flattenedPixels[row * width + col];
	}

	// Return a reference to the pixel we can't modify
	const Pixel& pixelAt(const size_t row, const size_t col) const {
		return flattenedPixels[row * width + col];
	}

	bool isWithinBounds(const size_t row, const size_t col) const {
		return row < height && col < width;
	}

	void displayorater() const {
		for (size_t row = 0; row < height; ++row) {
			for (size_t col = 0; col < width; ++col) {
				const Pixel& px = pixelAt(row, col);

				// Set foreground and background explicitly using RGB8
				ncplane_set_bg_rgb8(plane, px.r, px.g, px.b);

				// Draw space to represent pixel
				ncplane_putstr_yx(plane, row, col, " ");
			}
		}
		notcurses_render(ncplane_notcurses(plane));
	}
};

//
// 3d stuff
//

struct Vec3 {
	float x, y, z;
	Vec3() : Vec3{ 0, 0, 0 } {}
	Vec3(float x, float y, float z) : x{ x }, y{ y }, z{ z } {}

	// Overload basic operators
	Vec3 operator+(const Vec3& v) const {
		return Vec3{ x + v.x, y + v.y, z + v.z };
	}
	Vec3 operator-(const Vec3& v) const {
		return Vec3{ x - v.x, y - v.y, z - v.z };
	}
	Vec3 operator*(const float c) const {
		return Vec3{ x * c, y * c, z * c };
	}
	// Negative operator
	Vec3 operator-() const {
		return Vec3{ -x, -y, -z };
	}

	// Vector operators
	float dot(const Vec3& v) const {
		return (x * v.x) + (y * v.y) + (z * v.z);
	}
	float length() const {
		return sqrtf((x * x) + (y * y) + (z * z));
	}
	Vec3 norm() const {
		const float len = length();
		return Vec3{ x / len, y / len, z / len };
	}
	// Return a vector perpendicular to both (length is the area formed by both vectors)
	Vec3 cross(const Vec3& v) const {
		return Vec3{
			y * v.z - z * v.y,
			z * v.x - x * v.z,
			x * v.y - y * v.x
		};
	}
};

struct Ray {
	Vec3 origin, direction;
	Ray(const Vec3& o, const Vec3& d) : origin{ o }, direction{ d.norm() } {}
};

struct Light {
	Vec3 direction;
	Pixel color; // Light color/brightness

	Light(const Vec3& d, const Pixel& c) : direction{ d.norm() }, color{ c } {}
};

struct Camera {
	Vec3 position;
	float yaw_degrees; // Left/right
	float pitch_degrees; // Up/down

	Camera(const Vec3& v, const float y, const float p) : position{ v }, yaw_degrees{ y }, pitch_degrees{ p } {}
};

struct Plane {
	Vec3 point; // Any point on the plane
	Vec3 normal; // Normal vector
	Pixel color;

	Plane(const Vec3& p, const Vec3& n, const Pixel& c) : point{ p }, normal{ n.norm() }, color{ c } {}

	// Returns true if ray hits the plane, sets dist to intersection distance
	bool intersects(const Ray& ray, float& dist) const {
		float denominator = normal.dot(ray.direction);
		if (fabs(denominator) < 1e-6) return false; // Parallel, no hit
		dist = (point - ray.origin).dot(normal) / denominator;
		return dist > 0;
	}
};

struct Sphere {
	Vec3 center;
	float radius;
	Pixel color;

	Sphere(const Vec3 c, const float r, const Pixel p) : center{ c }, radius{ r }, color{ p } {}

	// Check if a ray intersects with the sphere (dist is updated when intersection dist found)
	bool intersects(const Ray& ray, float& dist) const {
		// dist is the length of the ray that hits the sphere (multiply by direction for the full ray)
		// a*dist^2 + b*dist + c = 0

		// Get the vector from the center of the sphere, to the ray's origin
		const Vec3 centerToOrigin = ray.origin - center;

		// Calculate a, b, c using the quadratic equation
		const float a = ray.direction.dot(ray.direction);
		const float b = 2.0f * centerToOrigin.dot(ray.direction);
		const float c = centerToOrigin.dot(centerToOrigin) - (radius * radius);

		// If the discriminant is negative, the ray does not intersect the sphere
		const float discriminant = (b * b) - (4 * a * c);
		if (discriminant < 0) return false;

		// The ray is not negative, so we can solve for the intersection distance
		dist = (-b - sqrtf(discriminant)) / (2.0f * a);
		return dist > 0;
	}
};

void get_camera_basis(const Camera& camera, Vec3& forward, Vec3& right, Vec3& up) {
	const float yaw = degToRad(camera.yaw_degrees);
	const float pitch = degToRad(camera.pitch_degrees);

	forward = Vec3{
		cosf(pitch) * sinf(yaw),
		sinf(pitch),
		cosf(pitch) * cosf(yaw)
	};
	up = Vec3{ 0, 1, 0 };
	right = forward.cross(up).norm();
	up = right.cross(forward).norm();
}

void render_scene(Display3D& image, const Camera& camera, const vector<Plane>& planes, const vector<Sphere>& spheres, const vector<Light>& lights) {
	const size_t width = image.getNumCols();
	const size_t height = image.getNumRows();

	// Calculate aspect ratio for proper scaling
	// Divide width by 2 since we are using 2:1 tall rectangular pixels
	const float aspect = (width / 2.0f) / static_cast<float>(height);

	// Distance from camera to image plane
	constexpr float camera_to_plane = 1.0f;

	// Calculate the size of the image plane based on FOV and aspect ratio
	const float plane_height = 2.0f * camera_to_plane * tanf(degToRad(FOV * 0.5f));
	const float plane_width = plane_height * aspect;

	// Get camera basis vectors
	Vec3 forward, right, up;
	get_camera_basis(camera, forward, right, up);

	// Cast rays for each pixel in the image
	for (size_t row = 0; row < height; ++row) {
		for (size_t col = 0; col < width; ++col) {
			// Map pixel to world coordinates on the image plane
			const float x = -((col + 0.5f) / width - 0.5f) * plane_width; // Negate for correct orientation (flip)
			const float y = ((row + 0.5f) / height - 0.5f) * plane_height;

			// Calculate pixel position in world space
			const Vec3 pixelPos = camera.position + (forward * camera_to_plane) + (right * x) + (up * y);

			// Create ray from camera to pixel
			const Ray ray{ camera.position, (pixelPos - camera.position).norm() };

			// TODO: Make generic object type
			// Find closest sphere
			float closest_dist = INFINITY;
			const Sphere* closest_sphere = nullptr;
			for (const auto& sphere : spheres) {
				float dist;
				if (sphere.intersects(ray, dist) && dist < closest_dist) {
					closest_dist = dist;
					closest_sphere = &sphere;
				}
			}

			// Find closest plane
			float closest_plane_dist = INFINITY;
			const Plane* closest_plane = nullptr;
			for (const auto& plane : planes) {
				float dist;
				if (plane.intersects(ray, dist) && dist < closest_plane_dist) {
					closest_plane_dist = dist;
					closest_plane = &plane;
				}
			}

			// Closest sphere exists
			if (closest_sphere && closest_dist < closest_plane_dist) {
				// Calculate the hit point and normal at the intersection
				const Vec3 hit_point = camera.position + ray.direction * closest_dist;
				const Vec3 normal = (hit_point - closest_sphere->center).norm();

				// View direction (from hit point to camera)
				const Vec3 viewDir = (camera.position - hit_point).norm();

				// Accumulate the light sources onto the sphere
				float rTotal = 0, gTotal = 0, bTotal = 0;
				for (const auto& light : lights) {
					// Diffuse shading ( Lambertian reflectance)
					const float diffuse = max(0.0f, normal.dot(light.direction));

					// Specular shading (Blinn-Phong)
					const Vec3 halfway = (light.direction + viewDir).norm();
					const float specAngle = max(0.0f, normal.dot(halfway));
					const float specular = powf(specAngle, SPECULAR_SHININESS);

					// Diffuse color
					rTotal += closest_sphere->color.r * diffuse * (light.color.r / RGB_MAX_FLOAT);
					gTotal += closest_sphere->color.g * diffuse * (light.color.g / RGB_MAX_FLOAT);
					bTotal += closest_sphere->color.b * diffuse * (light.color.b / RGB_MAX_FLOAT);

					// Specular highlight (white, or use light color)
					rTotal += RGB_MAX_FLOAT * specular * (light.color.r / RGB_MAX_FLOAT);
					gTotal += RGB_MAX_FLOAT * specular * (light.color.g / RGB_MAX_FLOAT);
					bTotal += RGB_MAX_FLOAT * specular * (light.color.b / RGB_MAX_FLOAT);
				}

				Pixel& pix = image.pixelAt(row, col);
				pix.r = static_cast<u_char>(min(rTotal, RGB_MAX_FLOAT));
				pix.g = static_cast<u_char>(min(gTotal, RGB_MAX_FLOAT));
				pix.b = static_cast<u_char>(min(bTotal, RGB_MAX_FLOAT));
			}
			else if (closest_plane) {
				const Vec3 hit_point = camera.position + ray.direction * closest_plane_dist;
				const Vec3 normal = closest_plane->normal;
				const Vec3 viewDir = (camera.position - hit_point).norm();

				float rTotal = 0, gTotal = 0, bTotal = 0;
				for (const auto& light : lights) {
					const float diffuse = max(0.0f, normal.dot(light.direction));
					const Vec3 halfway = (light.direction + viewDir).norm();
					const float specAngle = max(0.0f, normal.dot(halfway));
					const float specular = powf(specAngle, SPECULAR_SHININESS);

					rTotal += closest_plane->color.r * diffuse * (light.color.r / RGB_MAX_FLOAT);
					gTotal += closest_plane->color.g * diffuse * (light.color.g / RGB_MAX_FLOAT);
					bTotal += closest_plane->color.b * diffuse * (light.color.b / RGB_MAX_FLOAT);

					rTotal += RGB_MAX_FLOAT * specular * (light.color.r / RGB_MAX_FLOAT);
					gTotal += RGB_MAX_FLOAT * specular * (light.color.g / RGB_MAX_FLOAT);
					bTotal += RGB_MAX_FLOAT * specular * (light.color.b / RGB_MAX_FLOAT);
				}

				Pixel& pix = image.pixelAt(row, col);
				pix.r = static_cast<u_char>(min(rTotal, RGB_MAX_FLOAT));
				pix.g = static_cast<u_char>(min(gTotal, RGB_MAX_FLOAT));
				pix.b = static_cast<u_char>(min(bTotal, RGB_MAX_FLOAT));
			}
		}
	}
}

// void draw_line_3d(Display3D& image, const Camera& camera, const Vec3& p0, const Vec3& p1, const Pixel& color) {
// 	Vec3 forward, right, up;
// 	get_camera_basis(camera, forward, right, up);
// 	right = -right; // Invert right vector to match typical camera controls
// 	up = -up; // Invert up vector to match typical camera controls

// 	const size_t width = image.getNumCols();
// 	const size_t height = image.getNumRows();
// 	const float aspect = (width / 2.0f) / static_cast<float>(height);
// 	constexpr float camera_to_plane = 1.0f;
// 	const float plane_height = 2.0f * camera_to_plane * tanf(degToRad(FOV * 0.5f));
// 	const float plane_width = plane_height * aspect;

// 	auto project = [&](const Vec3& pt) -> pair<int, int> {
// 		Vec3 cam_to_pt = pt - camera.position;
// 		float z = forward.dot(cam_to_pt);
// 		if (z <= 0.1f) return { -1, -1 }; // Behind camera

// 		float x = right.dot(cam_to_pt);
// 		float y = up.dot(cam_to_pt);

// 		// Perspective divide
// 		float px = (x / z) * camera_to_plane;
// 		float py = (y / z) * camera_to_plane;

// 		int col = static_cast<int>((0.5f + px / plane_width) * width);
// 		int row = static_cast<int>((0.5f - py / plane_height) * height);
// 		return { row, col };
// 		};

// 	auto [row0, col0] = project(p0);
// 	auto [row1, col1] = project(p1);

// 	// Simple Bresenham's line algorithm
// 	if (row0 >= 0 && col0 >= 0 && row1 >= 0 && col1 >= 0 &&
// 		image.isWithinBounds(row0, col0) && image.isWithinBounds(row1, col1)) {
// 		int dx = abs(col1 - col0), sx = col0 < col1 ? 1 : -1;
// 		int dy = abs(row1 - row0), sy = row0 < row1 ? 1 : -1;
// 		int err = (dx > dy ? dx : -dy) / 2, e2;

// 		int x = col0, y = row0;
// 		while (true) {
// 			image.pixelAt(y, x) = color;
// 			if (x == col1 && y == row1) break;
// 			e2 = err;
// 			if (e2 > -dx) {
// 				err -= dy; x += sx;
// 			}
// 			if (e2 < dy) {
// 				err += dx; y += sy;
// 			}
// 		}
// 	}
// }

struct KeyState {
	// Pack key states into 11 bits
	uint16_t packed = 0;

	void clear() {
		packed = 0;
	}

	// Set a key to pressed
	void set(const int key) {
		uint16_t mask = 0;
		switch (key) {
			case 'q':         mask = 0x001; break;

			case 'w':         mask = 0x002; break;
			case 'a':         mask = 0x004; break;
			case 's':         mask = 0x008; break;
			case 'd':         mask = 0x010; break;

			case NCKEY_SPACE: mask = 0x020; break;
			case 'x':         mask = 0x040; break;

			case NCKEY_UP:    mask = 0x080; break;
			case NCKEY_DOWN:  mask = 0x100; break;
			case NCKEY_LEFT:  mask = 0x200; break;
			case NCKEY_RIGHT: mask = 0x400; break;

			default: return; // Ignore unsupported keys
		}

		packed |= mask;
	}

	// Functions to check if a key is pressed
	bool q() const {
		return packed & 0x001;
	}

	bool w() const {
		return packed & 0x002;
	}
	bool a() const {
		return packed & 0x004;
	}
	bool s() const {
		return packed & 0x008;
	}
	bool d() const {
		return packed & 0x010;
	}

	bool space() const {
		return packed & 0x020;
	}
	bool x() const {
		return packed & 0x040;
	}

	bool up() const {
		return packed & 0x080;
	}
	bool down() const {
		return packed & 0x100;
	}
	bool left() const {
		return packed & 0x200;
	}
	bool right() const {
		return packed & 0x400;
	}
};

int main() {
	ios_base::sync_with_stdio(false); // Disable IO synchronization for performance
	setenv("COLORTERM", "truecolor", 1); // Enable perfect rgb colors in the terminal

	// Initialize notcurses
	struct notcurses_options opts {};
	notcurses* nc = notcurses_init(&opts, nullptr);
	if (!nc) return 1;

	// Enable mouse input
	notcurses_mice_enable(nc, NCMICE_ALL_EVENTS);

	ncplane* stdplane = notcurses_stdplane(nc);
	Display3D display{ 80, 45, stdplane };

	// Camera position (want to have it behind the image plane)
	Camera camera{ Vec3{ 0, 0, -60 }, 0.0f, 0.0f }; // Straight camera

	vector<Plane> planes{
		Plane{ Vec3{ 0, 25, 0 }, Vec3{ 0, 1, 0 }, Pixel{ 230, 230, 230 } } // Green ground plane
	};

	vector<Sphere> spheres = {
		Sphere{Vec3{ 0, 0, 0 }, 25, Pixel{ 255, 255, 255 }}, // White sphere
		Sphere{Vec3{ 30, 20, -15 }, 10, Pixel{ 255, 255, 140 }} // Light yellow sphere front, up, and to the right of the first
	};

	vector<Light> lights = {
		Light{Vec3{5, -10, 1}, Pixel{182, 34, 228}}, // Back top right (magenta light)
		Light{Vec3{-10, 3, -1}, Pixel{24, 236, 238 }}, // Front bottom left (cyan light)
		Light{Vec3{1, 4, -1}, Pixel{100, 100, 100 }}, // Front bottom right (dim white)

		// Light{Vec3{1, -1, -1}, Pixel{ 255, 255, 255 }}, // Front top right (white)
	};

	bool running = true;
	
	KeyState keys;
	int last_mouse_x = -1, last_mouse_y = -1;
	while (running) {
		keys.clear(); // Clear previous key states

		// Collect all key presses and mouse movements this frame
		// ? Note: Key releases are not possible to detect with notcurses but pressing multiple keys within a frame is possible
		struct ncinput input;
		while (notcurses_get_nblock(nc, &input) > 0) {
			keys.set(input.id); // Set key as pressed

			// Mouse look
            if (nckey_mouse_p(input.id) && (input.x != last_mouse_x || input.y != last_mouse_y)) {
                if (last_mouse_x != -1 && last_mouse_y != -1) {
                    int dx = input.x - last_mouse_x;
                    int dy = input.y - last_mouse_y;

                    constexpr float mouse_sensitivity = 0.7f; // Adjust as needed
                    camera.yaw_degrees += dx * mouse_sensitivity;
                    camera.pitch_degrees -= dy * mouse_sensitivity;

					// Clamp pitch and wrap yaw
                    camera.pitch_degrees = std::clamp(camera.pitch_degrees, -89.0f, 89.0f);
                    if (camera.yaw_degrees < 0.0f) camera.yaw_degrees += 360.0f;
                    else if (camera.yaw_degrees >= 360.0f) camera.yaw_degrees -= 360.0f;
                }

                last_mouse_x = input.x;
                last_mouse_y = input.y;
            }
		}

		Vec3 forward, right, up;
		get_camera_basis(camera, forward, right, up);
		right = -right; // Invert right vector to match typical camera controls
		up = -up; // Invert up vector to match typical camera controls

		constexpr float move_step = 2.0f;
		constexpr float rotate_step = 3.0f;

		if (keys.q()) { // Quit
			running = false;
			continue;
		}

		if (keys.w()) camera.position = camera.position + forward * move_step;
		if (keys.s()) camera.position = camera.position - forward * move_step;
		if (keys.a()) camera.position = camera.position - right * move_step;
		if (keys.d()) camera.position = camera.position + right * move_step;

		if (keys.space()) camera.position = camera.position + up * move_step;
		if (keys.x()) camera.position = camera.position - up * move_step;

		if (keys.up()) {
			camera.pitch_degrees -= rotate_step;
			camera.pitch_degrees = std::clamp(camera.pitch_degrees, -89.0f, 89.0f); // Prevent turning camera upside down
		}
		if (keys.down()) {
			camera.pitch_degrees += rotate_step;
			camera.pitch_degrees = std::clamp(camera.pitch_degrees, -89.0f, 89.0f);
		}
		if (keys.left()) {
			camera.yaw_degrees -= rotate_step;
			if (camera.yaw_degrees < 0.0f) camera.yaw_degrees += 360.0f;
			else if (camera.yaw_degrees >= 360.0f) camera.yaw_degrees -= 360.0f;
		}
		if (keys.right()) {
			camera.yaw_degrees += rotate_step;
			if (camera.yaw_degrees < 0.0f) camera.yaw_degrees += 360.0f;
			else if (camera.yaw_degrees >= 360.0f) camera.yaw_degrees -= 360.0f;
		}

		display.clear();
		render_scene(display, camera, planes, spheres, lights);
		display.displayorater();

		// Optional small sleep to avoid max CPU usage
		std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 100 fps
	}

	notcurses_stop(nc);
	return 0;
}
