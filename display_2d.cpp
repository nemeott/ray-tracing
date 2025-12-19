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
			<< "  "; // Square with rgb background color (2 spaces for a square)
	}
};

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

struct Display {
	size_t width;
	size_t height;

	Display() : width(WIDTH), height(HEIGHT) {}
	Display(const size_t w, const size_t h) : width(w), height(h) {}

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
			static_cast<u_char>(rTotal / numPixels),
			static_cast<u_char>(gTotal / numPixels),
			static_cast<u_char>(bTotal / numPixels)
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

int main() {
	Image dis(100, 200);
	dis.addRectangle(15, 47, 30, 60, Pixel{ 34, 150, 228 });
	dis.addCircle(63, 153, 16, Pixel{ 182, 34, 228 });

	Display display;
	display.displayorater(dis);


	// Image dat;
	// dat.addCircle(15, 12, 8, Pixel{ 182, 34, 228 });

	// display.displayorater(dat);

	// Image img = Image::load_png("lions.png");
	// Display display;
	// display.displayorater(img);
}
