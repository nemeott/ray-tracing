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
//Pixel p{ 64, int(255 / numPatties * row), int(255 / numSlices * col)};
// Pixel p{ 128, int(int(row) * 8.5), int(int(col) * 8.5) };

//#include <opencv2/opencv.hpp>

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace std;

constexpr size_t WIDTH = 30;
constexpr size_t HEIGHT = 30;

struct Pixel {
	int r, g, b;

	// Prints a pixel to the terminal
	void pixelerator() {
		cout << "\033[48;2;" // Set background color in truecolor mode
			<< r << ";"
			<< g << ";"
			<< b << "m"
			<< "  "; // Square with rgb background color (2 spaces for a square)
	}
};

struct Image {
	vector<vector<Pixel>> flattenedPixels;
	size_t numPatties;
	size_t numSlices;

	Image() : Image(WIDTH, HEIGHT) {};

	Image(size_t width, size_t height) {
		numPatties = height;
		numSlices = width;

		for (size_t row = 0; row < numPatties; row++) {
			vector<Pixel> hamburgerPatty;

			for (size_t col = 0; col < numSlices; col++) {
				Pixel p{ 0, 0, 0 };

				hamburgerPatty.push_back(p);
			}

			flattenedPixels.push_back(hamburgerPatty);
		}
	}

	size_t getNumRows() {
		return numPatties;
	}

	size_t getNumCols() {
		return numSlices;
	}

	Pixel get_pixel(size_t row, size_t col) {
		return flattenedPixels[row][col];
	}
};

struct Display {
	size_t width = WIDTH;
	size_t height = HEIGHT;

	Pixel average(Image image, size_t startRow, size_t endRow, size_t startCol, size_t endCol) {
		size_t rTotal = 0;
		size_t gTotal = 0;
		size_t bTotal = 0;

		for (size_t row = startRow; row < endRow; row++) {
			for (size_t col = startCol; col < endCol; col++) {
				const Pixel pix = image.get_pixel(row, col);

				rTotal += pix.r; // Pixar
				gTotal += pix.g;
				bTotal += pix.b;
			}
		}

		size_t numPixels = (endRow - startRow) * (endCol - startCol);
		// if (numPixels == 0)
		// 	return Pixel{0, 0, 0};

		return Pixel{
			int(rTotal / numPixels),
			int(bTotal / numPixels),
			int(gTotal / numPixels)
		};
	}

	void displayorater(Image image) {
		// Clear screen and move cursor to top-left
		//   \033 is the ESC character
		//   [2J clears screen
		//   [H Moves the cursor to the origin
		cout << "\033[2J\033[H";

		for (size_t row = 0; row < height; row++) {
			for (size_t col = 0; col < width; col++) {
				float heightScale = image.getNumRows() / float(height);
				float widthScale = image.getNumCols() / float(width);

				size_t startRow = row * heightScale;
				size_t endRow = (row + 1) * heightScale;
				size_t startCol = col * widthScale;
				size_t endCol = (col + 1) * widthScale;

				Pixel averaged_pixel = average(image, startRow, endRow, startCol, endCol);

				averaged_pixel.pixelerator();
			}

			cout << "\n"; // Print new line
			cout << "\033[0m"; // Reset attributes to default
		}
	}
};

int main() {
	Image dis; // dis has default width and height of 40 each
	Image dat(100, 200); // dat has width 30 and height 20

	Display display;
	display.displayorater(dat);
}
