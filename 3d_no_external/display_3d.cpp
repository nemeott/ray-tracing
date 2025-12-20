// TODO: Image struct with 2d array of pixels
//   Print function to display

// TODO: Vec3/3d point (3d vector) struct with basic vector operations (overload basic operators)

// TODO: Ray struct (2 Vec3s: origin point, and direction vector)

// TODO: Sphere struct (position, radius, and color)
//   Intersect function calculates whether a ray intersects with the sphere

// TODO: Trace function to trace a ray through the scene and calculate color
//   Find closest sphere
//   Diffuse lighting and specular highlights

//Pixel p{ 128, int(row) * 3, int(col) * 3};
//Pixel p{ 64, int(255 / numRows * row), int(255 / numCols * col)};
// Pixel p{ 128, int(int(row) * 8.5), int(int(col) * 8.5) };

// "const func()" means the return value is const
// "func() const" means the function does not modify the object

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

// Sleep
#include <chrono>
#include <thread>

constexpr size_t WIDTH = 30;
constexpr size_t HEIGHT = 30;

constexpr float FOV = 90.0f; // The zoom

using namespace std; // TODO: remove

constexpr float degToRad(const float degrees) {
	return degrees * M_PI / 180.0f;
}

struct Pixel {
	u_char r, g, b;

	Pixel() : Pixel{ 0, 0, 0 } {}
	Pixel(const u_char r, const u_char g, const u_char b) : r{ r }, g{ g }, b{ b } {}

	// Prints a pixel to the terminal
	void pixelerator() const {
		cout << "\033[48;2;" // Set background color in truecolor mode
			<< static_cast<int>(r) << ";"
			<< static_cast<int>(g) << ";"
			<< static_cast<int>(b) << "m"
			// To use rectangular pixels (one space) for higher fidelity, multiply the display width by 2
			<< " "; // Rectangular pixel (2:1 tall) with rgb background color

		// << "  "; // Square with rgb background color (2 spaces for a square)
	}
};

// Struct that holds image data and renders the image
struct Display3D {
	vector<Pixel> flattenedPixels;
	size_t width;
	size_t height;

	// Width is multiplied by 2 since we are using 2:1 tall rectangular pixels
	Display3D() : Display3D(WIDTH, HEIGHT) {}
	Display3D(const size_t w, const size_t h) : width{ w * 2 }, height{ h } {
		for (size_t row = 0; row < height; ++row) {
			for (size_t col = 0; col < width; ++col) {
				// Initialize with black pixels
				flattenedPixels.emplace_back(0, 0, 0);
			}
		}	
	}

	void clear() {
		for (auto& pixel : flattenedPixels) {
			pixel.r = 0;
			pixel.g = 0;
			pixel.b = 0;
		}
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

	// void addRectangle(const size_t x, const size_t y, const size_t width, const size_t height, const Pixel& color) {
	// 	// Calculate bounds
	// 	size_t x_end = x + width;
	// 	if (x_end >= getNumCols()) x_end = getNumCols() - 1;
	// 	size_t y_end = y + height;
	// 	if (y_end >= getNumRows()) x_end = getNumRows() - 1;

	// 	for (size_t row = y; row < y_end; ++row) {
	// 		for (size_t col = x; col < x_end; ++col) {
	// 			pixelAt(row, col) = color;
	// 		}
	// 	}
	// }

	// void addCircle(const size_t x, const size_t y, const size_t radius, const Pixel& color) {
	// 	for (size_t row = y - radius; row <= y + radius; ++row) {
	// 		for (size_t col = x - radius; col <= x + radius; ++col) {
	// 			if (isWithinBounds(row, col)) {
	// 				const size_t dx = col - x;
	// 				const size_t dy = row - y;

	// 				// x^2 + y^2 <= r^2
	// 				if ((dx * dx) + (dy * dy) <= (radius * radius)) {
	// 					pixelAt(row, col) = color;
	// 				}
	// 			}
	// 		}
	// 	}
	// }

	void displayorater() const {
		// Clear screen and move cursor to top-left
		//   \033 is the ESC character
		//   [2J clears screen
		//   [H Moves the cursor to the origin
		cout << "\033[2J\033[H";

		for (size_t row = 0; row < height; ++row) {
			for (size_t col = 0; col < width; ++col) {
				pixelAt(row, col).pixelerator();
			}

			cout << "\n"; // Print new line
		}

		cout << "\033[0m"; // Reset attributes to default
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

void render_scene(Display3D& image, const Camera& camera, const vector<Sphere>& spheres, const vector<Light>& lights) {
	const size_t width = image.getNumCols();
	const size_t height = image.getNumRows();

	// Divide width by 2 since we are using 2:1 tall rectangular pixels
	const float aspect = (width / 2.0f) / static_cast<float>(height);

	// Distance from camera to image plane
	const float image_plane_z = 0.0f; // z-pos of the image in world space (where the screen is in 3d)
	const float camera_to_plane = abs(camera.position.z - image_plane_z);
	const float epsilon = 1e-6f; // Add a small value to prevent division by 0 (would cause the camera to be close and not where we want)
	const float safe_camera_to_plane = camera_to_plane + epsilon;

	// Compute image plane size in world units
	const float plane_height = 2.0f * safe_camera_to_plane * tanf(degToRad(FOV * 0.5f));
	const float plane_width = plane_height * aspect;

	// Camera orientation
	const float yaw = degToRad(camera.yaw_degrees);
	const float pitch = degToRad(camera.pitch_degrees);

	// Calculate camera basis vectors
	const Vec3 forward{
		cosf(pitch) * sinf(yaw),
		sinf(pitch),
		cosf(pitch) * cosf(yaw)
	};
	Vec3 up{ 0, 1, 0 };
	const Vec3 right = forward.cross(up).norm();
	up = right.cross(forward).norm();

	for (size_t row = 0; row < height; ++row) {
		for (size_t col = 0; col < width; ++col) {
			// Map pixel to world coordinates on the image plane
			const float x = -((col + 0.5f) / width - 0.5f) * plane_width; // Negate for correct orientation (flip)
			const float y = ((row + 0.5f) / height - 0.5f) * plane_height;

			// Create 3d vector that represents position of the pixel in 3d coordinates
			const Vec3 pixelPos = camera.position + (forward * safe_camera_to_plane) + (right * x) + (up * y);

			// Create the ray for ray-tracing (Ray(origin, direction (normalized by constructor)))
			const Ray ray{ camera.position, (pixelPos - camera.position).norm() };

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

			// Closest sphere exists
			if (closest_sphere != nullptr) {
				const Vec3 hit_point = camera.position + ray.direction * closest_dist;
				const Vec3 normal = (hit_point - closest_sphere->center).norm();

				// Accumulate the light sources onto the sphere
				float rTotal = 0, gTotal = 0, bTotal = 0;
				for (const auto& light : lights) {
					const float brightness = max(0.0f, normal.dot(light.direction)); // Lambertian reflectance
					rTotal += closest_sphere->color.r * brightness * (light.color.r / 255.0f);
					gTotal += closest_sphere->color.g * brightness * (light.color.g / 255.0f);
					bTotal += closest_sphere->color.b * brightness * (light.color.b / 255.0f);
				}

				Pixel& pix = image.pixelAt(row, col);
				pix.r = static_cast<u_char>(min(rTotal, 255.0f));
				pix.g = static_cast<u_char>(min(gTotal, 255.0f));
				pix.b = static_cast<u_char>(min(bTotal, 255.0f));
			};
		}
	}
}

int main() {
	// Display3D display{ 20, 20 };
	Display3D display{ 30, 30 };
	// Display3D display{ 40, 40 };
	// Display3D display{ 80, 80 };

	// Camera position (want to have it behind the image plane)
	Camera camera{ Vec3{ 0, 0, -60 }, 0.0f, 0.0f }; // Straight camera
	// Camera camera{ Vec3{20, 40, -60}, -20.0f, -25.0f }; // Camera up, right, and back; looking right and down

	vector<Sphere> spheres = {
		Sphere{Vec3{ 0, 0, 0 }, 25, Pixel{ 255, 255, 255 }}, // White sphere
		Sphere{Vec3{ 20, 10, -15 }, 10, Pixel{ 255, 255, 140 }} // Light yellow sphere front, up, and to the right of the first
	};

	vector<Light> lights = {
		// Light{Vec3{5, -10, 1}, Pixel{182, 34, 228}}, // Back top right (magenta light)
		// Light{Vec3{-10, 3, -1}, Pixel{24, 236, 238 }}, // Front bottom left (cyan light)
		// Light{Vec3{1, 4, -1}, Pixel{100, 100, 100 }} // Front bottom right (dim white)

		Light{Vec3{1, -1, -1}, Pixel{ 255, 255, 255 }} // Front top right (white)
	};

	// render_scene(img, camera, spheres, lights);
	// display.displayorater(img);

	for (size_t frame = 0; frame < 200; ++frame) {
		display.clear();

		// camera.yaw_degrees = frame * 2.0f; // Turn right

		const float orbit_radius = 60.0f;
		const float orbit_speed = 2.0f * M_PI / 200.0f;
		const float angle = frame * orbit_speed;
	
		const Vec3 center = spheres[0].center;
	
		// Orbit in XZ plane, starting at (0, 0, -60)
		camera.position.x = center.x + orbit_radius * sinf(angle);
		camera.position.z = center.z - orbit_radius * cosf(angle); // negative to start at z = -60
		camera.position.y = center.y;
	
		// Calculate direction vector from camera to center
		Vec3 to_center = center - camera.position;
		camera.yaw_degrees = atan2f(to_center.x, to_center.z) * 180.0f / M_PI;
		camera.pitch_degrees = 0.0f;

		render_scene(display, camera, spheres, lights);
		display.displayorater();
		cout << flush;

		// ~33ms for 30 fps
		std::this_thread::sleep_for(std::chrono::milliseconds(33));
	}
}
