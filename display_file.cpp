#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

using namespace std;

// Use to display a prerendered scene
// Scene can be prerendered by directing the output of the display to a file
// Ex: ./display_3d > display_3d.out; ./display_file display_3d.out

// Reads all frames from the file, assuming each frame is separated by "\033[0m"
vector<string> read_frames(const string& filename) {
    ifstream infile(filename);
    string data((istreambuf_iterator<char>(infile)), istreambuf_iterator<char>());
    vector<string> frames;

    const string delimiter = "\033[0m";
    size_t start = 0, end;
    while ((end = data.find(delimiter, start)) != string::npos) {
        string frame = data.substr(start, end - start + delimiter.size());
        frames.push_back(frame);
        start = end + delimiter.size();
    }
    return frames;
}

void display_frame(const string& frame) {
    cout << frame << flush;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <frames_file.txt>\n";
        return 1;
    }

    auto frames = read_frames(argv[1]);
    if (frames.empty()) {
        cerr << "No frames found in file.\n";
        return 1;
    }

    for (const auto& frame : frames) {
        display_frame(frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
    }
    return 0;
}
