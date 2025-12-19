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

// For png input (wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

constexpr size_t WIDTH = 30;
constexpr size_t HEIGHT = 30;

using namespace std;

struct Pixel {
	u_char r, g, b;

	Pixel() : r(0), g(0), b(0) {}
	Pixel(const u_char r, const u_char g, const u_char b) : r(r), g(g), b(b) {}

	// Prints a pixel to the terminal
	void pixelerator() const {
		cout << "\033[48;2;" // Set background color in truecolor mode
			<< static_cast<int>(r) << ";"
			<< static_cast<int>(g) << ";"
			<< static_cast<int>(b) << "m"
			<< " "; // Square with rgb background color (2 spaces for a square)

		// To use rectangular pixels (one space) for higher fidelity, multiply the display width by 2
	}
};

// Struct that holds image pixel data
struct Image {
	vector<Pixel> flattenedPixels;
	size_t numRows;
	size_t numCols;

	Image() : Image(WIDTH, HEIGHT) {};
	Image(const size_t width, const size_t height) {
		numRows = height;
		numCols = width;

		for (size_t row = 0; row < numRows; row++) {
			for (size_t col = 0; col < numCols; col++) {
				// Initialize with black pixels
				flattenedPixels.push_back({ 0, 0, 0 });
			}
		}
	}

	size_t getNumRows() const {
		return numRows;
	}

	size_t getNumCols() const {
		return numCols;
	}

	// Return a reference to the pixel we can modify
	Pixel& pixel_at(const size_t row, const size_t col) {
		return flattenedPixels[row * getNumCols() + col];
	}

	// Return a reference to the pixel we can't modify
	const Pixel& pixel_at(const size_t row, const size_t col) const {
		return flattenedPixels[row * getNumCols() + col];
	}

	bool is_within_bounds(const size_t row, const size_t col) const {
		return row < getNumRows() && col < getNumCols();
	}

	void addRectangle(const size_t x, const size_t y, const size_t width, const size_t height, const Pixel& color) {
		// Calculate bounds
		size_t x_end = x + width;
		if (x_end >= getNumCols()) x_end = getNumCols() - 1;
		size_t y_end = y + height;
		if (y_end >= getNumRows()) x_end = getNumRows() - 1;

		for (size_t row = y; row < y_end; row++) {
			for (size_t col = x; col < x_end; col++) {
				pixel_at(row, col) = color;
			}
		}
	}

	void addCircle(const size_t x, const size_t y, const size_t radius, const Pixel& color) {
		for (size_t row = y - radius; row <= y + radius; row++) {
			for (size_t col = x - radius; col <= x + radius; col++) {
				if (is_within_bounds(row, col)) {
					const size_t dx = col - x;
					const size_t dy = row - y;

					// x^2 + y^2 <= r^2
					if ((dx * dx) + (dy * dy) <= (radius * radius)) {
						pixel_at(row, col) = color;
					}
				}
			}
		}
	}

	static Image load_png(const char* filename) {
		// Get the image data
		int width, height, channels;
		unsigned char* data = stbi_load(filename, &width, &height, &channels, 3); // force RGB

		// Create image and populate with the png data
		Image img(width, height);
		for (int row = 0; row < height; row++) {
			for (int col = 0; col < width; col++) {
				size_t idx = (row * width + col) * 3;
				Pixel p{ data[idx], data[idx + 1], data[idx + 2] };
				img.pixel_at(row, col) = p;
			}
		}
		stbi_image_free(data);
		return img;
	}
};

// Struct that renders an image
struct Display {
	size_t width;
	size_t height;

	Display() : width(WIDTH * 2), height(HEIGHT) {}
	Display(const size_t w, const size_t h) : width(w * 2), height(h) {}

	Pixel average(const Image& image, const size_t startRow, const size_t endRow, const size_t startCol, const size_t endCol) const {
		u_int rTotal = 0;
		u_int gTotal = 0;
		u_int bTotal = 0;

		for (size_t row = startRow; row < endRow; row++) {
			for (size_t col = startCol; col < endCol; col++) {
				const Pixel& pix = image.pixel_at(row, col);

				rTotal += pix.r; // Pixar
				gTotal += pix.g;
				bTotal += pix.b;
			}
		}

		const u_int numPixels = (endRow - startRow) * (endCol - startCol);

		return {
			static_cast<u_char>((rTotal / numPixels)),
			static_cast<u_char>((gTotal / numPixels)),
			static_cast<u_char>((bTotal / numPixels))
		};
	}

	void displayorater(const Image& image) const {
		// Clear screen and move cursor to top-left
		//   \033 is the ESC character
		//   [2J clears screen
		//   [H Moves the cursor to the origin
		cout << "\033[2J\033[H";

		for (size_t row = 0; row < height; row++) {
			for (size_t col = 0; col < width; col++) {
				const float heightScale = image.getNumRows() / static_cast<float>(height);
				const float widthScale = image.getNumCols() / static_cast<float>(width);

				const size_t startRow = row * heightScale;
				const size_t endRow = (row + 1) * heightScale;
				const size_t startCol = col * widthScale;
				const size_t endCol = (col + 1) * widthScale;

				// Get averaged pixel and print
				const Pixel averaged_pixel = average(image, startRow, endRow, startCol, endCol);
				averaged_pixel.pixelerator();
			}

			cout << "\n"; // Print new line
			cout << "\033[0m"; // Reset attributes to default
		}
	}
};



struct Vec3 {
	float x, y, z;
	Vec3() : x(0), y(0), z(0) {}
	Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

	// Overload operators
	Vec3 operator+(const Vec3& v) const {
		return Vec3(x + v.x, y + v.y, z + v.z);
	}
	Vec3 operator-(const Vec3& v) const {
		return Vec3(x - v.x, y - v.y, z - v.z);
	}
	Vec3 operator*(const float s) const {
		return Vec3(x * s, y * s, z * s);
	}

	// Vector operators
	float dot(const Vec3& v) const {
		return (x * v.x) + (y * v.y) + (z * v.z);
	}
	float length() const {
		return sqrt((x * x) + (y * y) + (z * z));
	}
	Vec3 norm() const {
		const float len = length();
		return Vec3(x / len, y / len, z / len);
	}
};

struct Ray {
	Vec3 origin, direction;
	Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d.norm()) {}
};

struct Light {
	Vec3 direction;
	Pixel color; // Light color/brightness

	Light(const Vec3& d, const Pixel& c) : direction(d.norm()), color(c) {}
};

struct Sphere {
	Vec3 center;
	float radius;
	Pixel color;

	Sphere(const Vec3 c, const float r, const Pixel p) : center(c), radius(r), color(p) {}

	// Check if a ray intersects with the sphere (dist is updated when intersection dist found)
	bool intersects(const Ray& ray, float& dist) const {
		// dist is the length of the ray that hits the sphere (multiply by direction for the full ray)
		// a*dist^2 + b*dist + c = 0

		// Get the vector from the center of the sphere, to the ray's origin
		Vec3 centerToOrigin = ray.origin - center;

		// Calculate a, b, c from the quadratic equation
		float a = ray.direction.dot(ray.direction);
		float b = 2.0f * centerToOrigin.dot(ray.direction);
		float c = centerToOrigin.dot(centerToOrigin) - (radius * radius);

		// If the discriminant is negative, the ray does not intersect the sphere
		float discriminant = b * b - 4 * a * c;
		if (discriminant < 0) return false;

		// The ray is not negative, so we can solve for the intersection distance
		dist = (-b - sqrt(discriminant)) / (2.0f * a);
		return dist > 0;
	}
};

void render_scene(Image& image, const Vec3& camera, const vector<Sphere>& spheres, const vector<Light>& lights) {
	size_t width = image.getNumCols();
	size_t height = image.getNumRows();

	const float fov = 90.0f; // FOV in degrees
	const float aspect = width / static_cast<float>(height);

	// Distance from camera to image plane
	const float image_plane_z = 0.0f; // z-pos of the image in world space (where the screen is in 3d)
	const float camera_to_plane = abs(camera.z - image_plane_z);

	// Compute image plane size in world units
	float plane_height = 2.0f * camera_to_plane * tanf((fov * 0.5f) * M_PI / 180.0f);
	float plane_width = plane_height * aspect;

	for (size_t row = 0; row < height; row++) {
		for (size_t col = 0; col < width; col++) {
			// Map pixel to world coordinates on the image plane
			float x = ((col + 0.5f) / width - 0.5f) * plane_width;
			float y = ((row + 0.5f) / height - 0.5f) * plane_height;

			// Create 3d vector that represents position of the pixel in 3d coordinates
			Vec3 pixelPos(x, y, image_plane_z);

			// Create the ray for ray-tracing (Ray(origin, direction (length 1, aka unit vector)))
			Ray ray(camera, (pixelPos - camera).norm());

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
				Vec3 hit_point = camera + ray.direction * closest_dist;
				Vec3 normal = (hit_point - closest_sphere->center).norm();

				// Accumulate the light sources onto the sphere
				float r = 0, g = 0, b = 0;
				for (const auto& light : lights) {
					float brightness = max(0.0f, normal.dot(light.direction)); // Lambertian reflectance
					r += closest_sphere->color.r * brightness * (light.color.r / 255.0f);
					g += closest_sphere->color.g * brightness * (light.color.g / 255.0f);
					b += closest_sphere->color.b * brightness * (light.color.b / 255.0f);
				}

				image.pixel_at(row, col) = Pixel{
					static_cast<u_char>(min(255.0f, r)),
					static_cast<u_char>(min(255.0f, g)),
					static_cast<u_char>(min(255.0f, b))
				};
			}
		}
	}
}

int main() {
	// Image dis{100, 200};
	// dis.addRectangle(15, 47, 30, 60, Pixel{ 34, 150, 228 });
	// dis.addCircle(63, 153, 16, Pixel{ 182, 34, 228 });

	// Display display;
	// display.displayorater(dis);


	// Image dat;
	// dat.addCircle(15, 12, 8, Pixel{ 182, 34, 228 });

	// display.displayorater(dat);

	// Image img = Image::load_png("lions.png");
	// Display display;
	// display.displayorater(img);

	// x: negative is left, positive is right
	// y: negative is up, positive is down
	// z: negative is front, positive is back

	Image img{ 200, 200 };
	// Vec3 camera{ 50, 10, -100 }; // Camera position (want to have it behind the image plane)
	Vec3 camera{ 0, 0, -60 }; // Camera position (want to have it behind the image plane)

	vector<Sphere> spheres = {
		Sphere{Vec3(0, 0, 0), 25, Pixel{ 255, 255, 255 }}, // White sphere
		Sphere{Vec3(20, 10, -15), 10, Pixel{ 255, 255, 140 }} // Light yellow sphere front, up, and to the right of the first
	};

	vector<Light> lights = {
		// Light{Vec3{5, -10, 1}, Pixel{182, 34, 228}}, // Back top right (magenta light)
		// Light{Vec3{-10, 3, -1}, Pixel{24, 236, 238 }}, // Front bottom left (cyan light)
		// Light{Vec3{1, 4, -1}, Pixel{100, 100, 100 }} // Front bottom right (dim white)

		Light{Vec3{1, -1, -1}, Pixel{255, 255, 255 }} // Front top right (white)
	};

	render_scene(img, camera, spheres, lights);

	// Display display{ 40, 40 };
	Display display{ 80, 80 };
	display.displayorater(img);
}
