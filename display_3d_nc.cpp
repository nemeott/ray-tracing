#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <memory> // For smart pointers

// Sleep
#include <thread>
#include <chrono>

// notcurses for terminal rendering and keyboard input (https://github.com/dankamongmen/notcurses)
#include <notcurses/notcurses.h>

constexpr float RGB_MAX_FLOAT = 255.0f;

constexpr float SPECULAR_SHININESS = 32.0f; // Specular shininess factor (higher is smaller/brighter highlights)

constexpr float FOV = 90.0f; // The zoom
constexpr float MOUSE_SENSITIVITY = 0.7f;


using namespace std; // TODO: remove

constexpr float degToRad(const float degrees) {
	return degrees * M_PI / 180.0f;
}

constexpr float radToDeg(const float radians) {
	return radians * 180.0f / M_PI;
}

// Struct that holds RGB pixel data
struct Pixel {
	u_char r, g, b;

	Pixel() : Pixel{ 0, 0, 0 } {}
	Pixel(const u_char r, const u_char g, const u_char b) : r{ r }, g{ g }, b{ b } {}
};

// Struct that holds image data and renders the image
struct Display3D {
	vector<Pixel> flattenedPixels; // 2D array flattened into 1D array of pixels
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
	Vec3 norm(const float len) const {
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

//
// Lights, camera, action
//

struct Light {
	Vec3 direction;
	Pixel color; // Light color/brightness

	Light(const Vec3& d, const Pixel& c) : direction{ d.norm() }, color{ c } {}
};

struct Camera {
	Vec3 position;
	float yawDegrees; // Left/right
	float pitchDegrees; // Up/down

	Camera(const Vec3& v, const float y, const float p) : position{ v }, yawDegrees{ y }, pitchDegrees{ p } {}
	
	void fix() {
		// Clamp pitch and wrap yaw
		pitchDegrees = clamp(pitchDegrees, -89.9999f, 89.9999f);
		if (yawDegrees < 0.0f) yawDegrees += 360.0f;
		else if (yawDegrees >= 360.0f) yawDegrees -= 360.0f;
	}
	
	void get_basis(Vec3& forward, Vec3& right, Vec3& up) const {
	  const float yaw = degToRad(yawDegrees);
	  const float pitch = degToRad(pitchDegrees);

	  forward = Vec3{
		  cosf(pitch) * sinf(yaw),
		  sinf(pitch),
		  cosf(pitch) * cosf(yaw)
	  };
	  up = Vec3{ 0, 1, 0 };
	  right = forward.cross(up).norm();
	  up = right.cross(forward).norm();
  }
	
  void orbit(const size_t frame, const Vec3& focal, const float orbitRadius, const Vec3& direction, const float degreesPerFrame) {
		// direction: Vec3 that indicates the rotation direction using -1, 0, or 1
		
		// Move the camera acording to the direction and speed
		const float angle = frame * degToRad(degreesPerFrame);
		position.x = focal.x + (direction.x * orbitRadius * sinf(angle));
		position.y = focal.y + (direction.y * orbitRadius * sinf(angle));
		position.z = focal.z + (direction.z * orbitRadius * cosf(angle));
		
		// Look at the focal point
		const Vec3 toCenter = focal - position;
		
		yawDegrees = radToDeg(atan2f(toCenter.x, toCenter.z));
		const float horizontalDistance = sqrtf(toCenter.x * toCenter.x + toCenter.z * toCenter.z); // TODO: Replace with pythag
		pitchDegrees = radToDeg(atan2f(toCenter.y, horizontalDistance));
		fix();
  }
};

//
// Objects
//

struct Object {
	Vec3 center;
	Pixel color;

	virtual ~Object() = default; // Prevent children from not being destroyed properly
	Object(const Vec3& c, const Pixel& p) : center{ c }, color{ p } {}

	virtual const Vec3 getNormalAt(const Vec3& hit_point) const = 0;
	virtual bool intersects(const Ray& ray, float& dist) const = 0;
};

struct Plane : public Object {
	// Vec3 center: Any point on the plane
	Vec3 normal; // Normal vector

	Plane(const Vec3& c, const Vec3& n, const Pixel& p) : Object{ c, p }, normal{ n.norm() } {}

	const Vec3 getNormalAt(const Vec3&) const override {
		return normal;
	}

	// Returns true if ray hits the plane, sets dist to intersection distance
	bool intersects(const Ray& ray, float& dist) const override {
		float denominator = normal.dot(ray.direction);
		if (fabs(denominator) < 1e-6) return false; // Parallel, no hit
		dist = (center - ray.origin).dot(normal) / denominator;
		return dist > 0;
	}
};

struct Box : public Object {
	// Vec3 center: The center of the box
	Vec3 u, v, w; // Orthonormal vectors
	float hu, hv, hw; // Half-lengths of each vector

	Box(const Vec3& c, const Vec3& U, const Vec3& V, const Vec3& W, const Pixel& p) : Object{ c, p } {
		hu = U.length();
		hv = V.length();
		hw = W.length();
		
		// Avoid redundant computations by passing in the length to calculate normal
		u = U.norm(hu);
		v = V.norm(hv);
		w = W.norm(hw);
		
		hu /= 2.0f;
		hv /= 2.0f;
		hw /= 2.0f;
	}
	
	const Vec3 getNormalAt(const Vec3& hit_point) const override {
		const Vec3 direction = hit_point - center;
		
		// Projections onto each axis
		const float pU = direction.dot(u);
		const float pV = direction.dot(v);
		const float pW = direction.dot(w);
		//const float pU = direction.dot(u*hu);
		//const float pV = direction.dot(v*hv);
		//const float pW = direction.dot(w*hw);
		
		const float a = abs(pU / hu);
		const float b = abs(pV / hv);
		const float c = abs(pW / hw);
		
		//return Vec3{pU, pV, pW}.norm();
		return Vec3{pU*hu, pV*hv, pW*hw}.norm(); // Diag gradient
    return Vec3{pU*a, pV*b, pW*c}.norm(); // Curved gradient
		
    /*
		if (a >= b && a >= c )
		  return pU > 0 ? direction.cross(u*a).norm() : direction.cross(-u*a).norm();
		else if (b >= c)
		  return pV > 0 ? direction.cross(v*b).norm() : direction.cross(-v*b).norm(); 

		return pW > 0 ? direction.cross(w*c).norm() : direction.cross(-v*c).norm();
		*/
	}
	
	void calculateMinMax(const float h, const float o, const float d, float& minDist, float& maxDist) const {
		minDist = (-h - o) / d;
		maxDist = (h - o) / d;
		
		if (maxDist < minDist) swap(minDist, maxDist);
	}

  // Check if a ray intersects the box
	bool intersects(const Ray& ray, float& dist) const override {
		// Transform ray into local space
		const Vec3 O = ray.origin - center;
		
		float minX, minY, minZ, maxX, maxY, maxZ;
		calculateMinMax(hu, O.dot(u), ray.direction.dot(u), minX, maxX);
		calculateMinMax(hv, O.dot(v), ray.direction.dot(v), minY, maxY);
		calculateMinMax(hw, O.dot(w), ray.direction.dot(w), minZ, maxZ);
		
		// Slab intersection
		const float entryDist = max(max(minX, minY), minZ);
		const float exitDist = min(min(maxX, maxY), maxZ);
		
		// Calculate actual distance
		
		// Intersection conditions
		if (entryDist <= exitDist && exitDist > 0) {
			dist = entryDist >= 0 ? entryDist : exitDist;
			return true;
		}
		
		return false;
	}
};

struct Sphere : public Object {
	// Vec3 center: The center of the sphere
	float radius;

	Sphere(const Vec3 c, const float r, const Pixel p) : Object{ c, p }, radius{ r } {}

	const Vec3 getNormalAt(const Vec3& hit_point) const override {
		return (hit_point - center).norm();
	}

	// Check if a ray intersects with the sphere (dist is updated when intersection dist found)
	bool intersects(const Ray& ray, float& dist) const override {
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

void render_scene(Display3D& image, const Camera& camera, const vector<unique_ptr<Object>>& objects, const vector<Light>& lights) {
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
	Vec3 forward, right, up; // TODO: Return pair and unpack for const
	camera.get_basis(forward, right, up);

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

			// Find closest object
			float closest_dist = INFINITY;
			const Object* closest_object = nullptr;
			for (const auto& object : objects) {
				float dist;
				if (object->intersects(ray, dist) && dist < closest_dist) {
					closest_dist = dist;
					closest_object = object.get();
				}
			}

			// Closest object
			if (closest_object) {
				// Calculate the hit point and normal at the intersection
				const Vec3 hit_point = camera.position + ray.direction * closest_dist;
				const Vec3 normal = closest_object->getNormalAt(hit_point);

				// View direction (from hit point to camera)
				const Vec3 viewDir = (camera.position - hit_point).norm();

				// Accumulate the light sources onto the sphere
				float rTotal = 0, gTotal = 0, bTotal = 0;
				for (const auto& light : lights) {
					// Diffuse shading ( Lambertian reflectance)
					const float diffuse = max(0.0f, normal.dot(light.direction));

					// Specular shading (Blinn-Phong)
					const Vec3 halfway = (light.direction + viewDir).norm();
					const float specAngle = max(0.0f, normal.dot(halfway)); // FIX: Specular highlight for box 
					const float specular = powf(specAngle, SPECULAR_SHININESS);

					// Diffuse color
					rTotal += closest_object->color.r * diffuse * (light.color.r / RGB_MAX_FLOAT);
					gTotal += closest_object->color.g * diffuse * (light.color.g / RGB_MAX_FLOAT);
					bTotal += closest_object->color.b * diffuse * (light.color.b / RGB_MAX_FLOAT);

					// Specular highlight (white, or use light color)
					rTotal += specular * light.color.r;
					gTotal += specular * light.color.g;
					bTotal += specular * light.color.b;
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

	// Auto-detect terminal size (zoom terminal out really far for lots of pixels)
	u_int rows, cols;
	ncplane_dim_yx(stdplane, &rows, &cols);
	Display3D display{ cols / 2, rows, stdplane };

	// Display3D display{ 80, 45, stdplane };

	// Camera position (want to have it behind the image plane)
	Camera camera{ Vec3{ 0, 0, -60 }, 0.0f, 0.0f }; // Straight camera

	// Combine all objects into a vector of Object pointers
	vector<std::unique_ptr<Object>> objects;
	
	objects.emplace_back(std::make_unique<Plane>(Vec3{ 0, 25, 0 }, Vec3{ 0, 1, 0 }, Pixel{ 230, 230, 230 })); // Light gray ground plane

	objects.emplace_back(std::make_unique<Sphere>(Vec3{ 0, 0, 0 }, 25, Pixel{ 255, 255, 255 })); // White sphere
	objects.emplace_back(std::make_unique<Sphere>(Vec3{ 30, 20, -15 }, 10, Pixel{ 255, 255, 140 })); // Light yellow sphere front, up, right of the first

  objects.emplace_back(std::make_unique<Box>(Vec3{ 0, 10, 0 }, Vec3{ 20, 0, 0 }, Vec3{ 0, 40, 0 }, Vec3{ 0, 0, 30 }, Pixel{ 255, 255, 255 }));

	// Create light sources (directional lights for now)
	vector<Light> lights = {
		Light{Vec3{5, -10, 1}, Pixel{182, 34, 228}}, // Back top right (magenta light)
		Light{Vec3{-10, 3, -1}, Pixel{24, 236, 238 }}, // Front bottom left (cyan light)
		Light{Vec3{1, 4, -1}, Pixel{100, 100, 100 }}, // Front bottom right (dim white)

		//Light{Vec3{1, -1, -1}, Pixel{ 255, 255, 255 }}, // Front top right (white)
	};

	// Input loop and rendering
	bool running = true;
	size_t frame = 0;

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

					camera.yawDegrees += dx * MOUSE_SENSITIVITY;
					camera.pitchDegrees += dy * MOUSE_SENSITIVITY;

					// Clamp pitch and wrap yaw
					camera.pitchDegrees = std::clamp(camera.pitchDegrees, -89.0f, 89.0f);
					if (camera.yawDegrees < 0.0f) camera.yawDegrees += 360.0f;
					else if (camera.yawDegrees >= 360.0f) camera.yawDegrees -= 360.0f;
				}

				last_mouse_x = input.x;
				last_mouse_y = input.y;
			}
		}

		Vec3 forward, right, up;
		camera.get_basis(forward, right, up);
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
			camera.pitchDegrees -= rotate_step;
			camera.pitchDegrees = std::clamp(camera.pitchDegrees, -89.0f, 89.0f); // Prevent turning camera upside down
		}
		if (keys.down()) {
			camera.pitchDegrees += rotate_step;
			camera.pitchDegrees = std::clamp(camera.pitchDegrees, -89.0f, 89.0f);
		}
		if (keys.left()) {
			camera.yawDegrees -= rotate_step;
			if (camera.yawDegrees < 0.0f) camera.yawDegrees += 360.0f;
			else if (camera.yawDegrees >= 360.0f) camera.yawDegrees -= 360.0f;
		}
		if (keys.right()) {
			camera.yawDegrees += rotate_step;
			if (camera.yawDegrees < 0.0f) camera.yawDegrees += 360.0f;
			else if (camera.yawDegrees >= 360.0f) camera.yawDegrees -= 360.0f;
		}

    
		// Example: Move the first sphere
		auto* sphere = dynamic_cast<Sphere*>(objects[1].get());
		if (sphere) {
			sphere->center.y -= 0.1f;
			sphere->center.x -= 0.1f;
		}
		
		
		camera.orbit(frame, Vec3{ 0, 0, 0 }, 60.0f, Vec3{ 1, 1, -1 }, 2.0f);

		display.clear();
		render_scene(display, camera, objects, lights);
		display.displayorater();

		// Optional small sleep to avoid max CPU usage
		++frame;
		//std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 100 fps
		std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 fps
	}

	notcurses_stop(nc);
	return 0;
}
