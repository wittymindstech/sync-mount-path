/**
 * @file syncPathsThreads.cpp
 *
 * @brief Copy Files, Directories, Sub Directories , Soft Links , Hard Links to Destination Mount Path
 *
 *
 * @author Kumar Gaurav
 * Contact: gaurav.2897@gmail.com
 *
 */
#include <fstream>
#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>
#include <unistd.h>  // For the link() function on Unix-like systems
#include <cstdlib>   // For atoi

namespace fs = std::filesystem;

// Function to copy files
void cpFile(const fs::path &srcPath, const fs::path &dstPath) {
    std::cout << "Copying file: " << srcPath << " to " << dstPath << std::endl;
    std::ifstream srcFile(srcPath, std::ios::binary);
    std::ofstream dstFile(dstPath, std::ios::binary);

    if (!srcFile || !dstFile) {
        std::cout << "Failed to get the file." << std::endl;
        return;
    }

    dstFile << srcFile.rdbuf();

    srcFile.close();
    dstFile.close();
}

// Function to copy soft links
void cpSoftLink(const fs::path &srcPath, const fs::path &dstPath) {
    std::cout << "Copying symbolic link: " << srcPath << " to " << dstPath << std::endl;
    // If symbolic link already exists, don't create it again
    if (! (fs::exists(dstPath)) ) {
        fs::create_symlink(fs::read_symlink(srcPath), dstPath);
    }
}

// Function to detect and recreate hard links
void cpHardLink(const fs::path &srcPath, const fs::path &dstPath) {
    // Check if the destination file already exists
    if (fs::exists(dstPath)) {
        std::cout << "Hard link already exists at destination." << std::endl;
        return;
    }
    
    // Check if the source file has more than one hard link (i.e., it's a hard link)
    if (fs::hard_link_count(srcPath) > 1) {
        // Recreate the hard link in the destination directory
        if (link(srcPath.c_str(), dstPath.c_str()) == -1) {
            perror("Error creating hard link");
        }
    }
}

// Function to create a new directory and copy its contents
void cpDirectory(const fs::path &srcPath, const fs::path &dstPath, int numThreads) {
    std::cout << "Copying directory: " << srcPath << " to " << dstPath << " using " << numThreads << " threads." << std::endl;
    fs::create_directories(dstPath);
    std::vector<std::thread> threads;

    for (const auto &entry : fs::directory_iterator(srcPath)) {
        const fs::path &srcFilePath = entry.path();
        const fs::path &dstFilePath = dstPath / srcFilePath.filename();

        std::cout << "Copying: " << srcFilePath << " to " << dstFilePath << std::endl;

        if (fs::is_directory(srcFilePath)) {
            // Create a thread for copying directories
            threads.emplace_back(cpDirectory, srcFilePath, dstFilePath, numThreads);
        } else if (fs::is_symlink(srcFilePath)) {
            // Create a thread for copying symbolic links
            threads.emplace_back(cpSoftLink, srcFilePath, dstFilePath);
        } else if (fs::is_regular_file(srcFilePath)) {
            // Check if it's a hard link and create a thread for copying hard links
            if (fs::hard_link_count(srcFilePath) > 1) {
                threads.emplace_back(cpHardLink, srcFilePath, dstFilePath);
            } else {
                // Create a thread for copying regular files
                threads.emplace_back(cpFile, srcFilePath, dstFilePath);
            }
        }
    }

    // Wait for all threads to finish
    for (auto &thread : threads) {
        thread.join();
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cout << "This Program Sync Files, Folders, Subfolders, Links from One Mount path to Another Mount Path" << std::endl;
        std::cout << "Usage: " << argv[0] << " SourcePath DestinationPath NumThreads" << std::endl;
        return 1;
    }

    const fs::path srcPath = argv[1];
    const fs::path dstPath = argv[2];
    int numThreads = std::atoi(argv[3]); // Convert the third argument to an integer

    // Start Invoking  the Directory, File, Links  Copy
    cpDirectory(srcPath, dstPath, numThreads);

    std::cout << "Path Sync Completed Successfully!!" << std::endl;
    return 0;
}
