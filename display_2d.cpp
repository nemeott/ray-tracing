// TODO: Image struct with 2d array of pixels
//   Print function to display

// TODO: Vec3/3d point (3d vector) struct with basic vector operations (overload basic operators)

// TODO: Ray struct (2 Vec3s: origin point, and direction vector)

// TODO: Sphere struct (position, radius, and color)
//   Intersect function calculates whether a ray intersects with the sphere

// TODO: Trace function to trace a ray through the scene and calculate color
//   Find closest sphere
//   Diffuse lighting and specular highlights


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

using namespace std; // TODO: remove

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

// Struct that holds image pixel data
struct Image {
	vector<Pixel> flattenedPixels;
	size_t numRows;
	size_t numCols;

	Image() : Image{ WIDTH, HEIGHT } {};
	Image(const size_t width, const size_t height) : numRows{ height }, numCols{ width } {
		for (size_t row = 0; row < numRows; ++row) {
			for (size_t col = 0; col < numCols; ++col) {
				// Initialize with black pixels
				flattenedPixels.emplace_back(0, 0, 0);
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
	Pixel& pixelAt(const size_t row, const size_t col) {
		return flattenedPixels[row * getNumCols() + col];
	}

	// Return a reference to the pixel we can't modify
	const Pixel& pixelAt(const size_t row, const size_t col) const {
		return flattenedPixels[row * getNumCols() + col];
	}

	bool isWithinBounds(const size_t row, const size_t col) const {
		return row < getNumRows() && col < getNumCols();
	}

	void addRectangle(const size_t x, const size_t y, const size_t width, const size_t height, const Pixel& color) {
		// Calculate bounds
		size_t x_end = x + width;
		if (x_end >= getNumCols()) x_end = getNumCols() - 1;
		size_t y_end = y + height;
		if (y_end >= getNumRows()) x_end = getNumRows() - 1;

		for (size_t row = y; row < y_end; ++row) {
			for (size_t col = x; col < x_end; ++col) {
				pixelAt(row, col) = color;
			}
		}
	}

	void addCircle(const size_t x, const size_t y, const size_t radius, const Pixel& color) {
		for (size_t row = y - radius; row <= y + radius; ++row) {
			for (size_t col = x - radius; col <= x + radius; ++col) {
				if (isWithinBounds(row, col)) {
					const size_t dx = col - x;
					const size_t dy = row - y;

					// x^2 + y^2 <= r^2
					if ((dx * dx) + (dy * dy) <= (radius * radius)) {
						pixelAt(row, col) = color;
					}
				}
			}
		}
	}

	static Image loadPng(const char* filename) {
		// Get the image data
		int width, height, channels;
		unsigned char* data = stbi_load(filename, &width, &height, &channels, 3); // force RGB
		const size_t sWidth = width, sHeight = height;

		// Create image and populate with the png data
		Image img{ sWidth, sHeight };
		for (size_t row = 0; row < sHeight; ++row) {
			for (size_t col = 0; col < sWidth; ++col) {
				size_t idx = (row * sWidth + col) * 3;
				Pixel p{ data[idx], data[idx + 1], data[idx + 2] };
				img.pixelAt(row, col) = p;
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

	// Width is multiplied by 2 since we are using 2:1 tall rectangular pixels
	Display() : Display(WIDTH, HEIGHT) {}
	Display(const size_t w, const size_t h) : width{ w * 2 }, height{ h } {}

	// Get the average color of a box of pixels
	Pixel average(const Image& image, const size_t startRow, const size_t endRow, const size_t startCol, const size_t endCol) const {
		u_int rTotal = 0, gTotal = 0, bTotal = 0;

		// Sum each value up
		for (size_t row = startRow; row < endRow; ++row) {
			for (size_t col = startCol; col < endCol; ++col) {
				const Pixel& pix = image.pixelAt(row, col);

				rTotal += pix.r; // Pixar
				gTotal += pix.g;
				bTotal += pix.b;
			}
		}

		const u_int numPixels = (endRow - startRow) * (endCol - startCol);

		// Divide each value by the total
		return Pixel{
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

		for (size_t row = 0; row < height; ++row) {
			for (size_t col = 0; col < width; ++col) {
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
	// Image dis{100, 200};
	// dis.addRectangle(15, 47, 30, 60, Pixel{ 34, 150, 228 });
	// dis.addCircle(63, 153, 16, Pixel{ 182, 34, 228 });

	// Display display;
	// display.displayorater(dis);


	// Image dat;
	// dat.addCircle(15, 12, 8, Pixel{ 182, 34, 228 });

	// display.displayorater(dat);

	Image img = Image::loadPng("lions.png");
	Display display;
	display.displayorater(img);
}
