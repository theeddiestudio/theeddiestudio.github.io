#include <iostream>     // For input/output operations (cin, cout, cerr)
#include <string>       // For string manipulation
#include <vector>       // For dynamic array (vector) of FileInfo
#include <filesystem>   // For file system operations (listing, renaming) - C++17 feature
#include <algorithm>    // For sorting (std::sort)
#include <regex>        // For regular expressions (matching filenames)
#include <tuple>        // Not strictly needed here as FileInfo struct is used, but useful for generic tuples.

// Alias for std::filesystem for brevity
namespace fs = std::filesystem;

// Structure to hold information about each file found that matches the pattern
struct FileInfo {
    int number;             // The numerical part extracted from the filename (e.g., 5 from 5.txt)
    fs::path original_path; // The full original path to the file
    std::string extension;  // The file extension (e.g., "txt" from 5.txt)
};

// Comparison function for sorting FileInfo objects in descending order by their number
// Used when 'a' is positive to rename highest numbers first, preventing conflicts.
bool compareFilesDesc(const FileInfo& a, const FileInfo& b) {
    return a.number > b.number;
}

// Comparison function for sorting FileInfo objects in ascending order by their number
// Used when 'a' is negative to rename lowest numbers first, preventing conflicts.
bool compareFilesAsc(const FileInfo& a, const FileInfo& b) {
    return a.number < b.number;
}

int main() {
    // Provide a brief introduction to the user about what the program does.
    std::cout << "This program renames files in the current directory." << std::endl;
    std::cout << "It targets files named like 'NUMBER.EXTENSION' (e.g., 5.txt, 33.jpg)." << std::endl;
    std::cout << "It will add your input number 'a' to the numeric part of these filenames." << std::endl;
    std::cout << "For example, if 'a' is 2, 5.txt becomes 7.txt." << std::endl;
    std::cout << "If 'a' is -2, 5.txt becomes 3.txt (files with new negative numbers will be skipped)." << std::endl;
    std::cout << "You will also enter a number 'b'. Only files with an original number >= 'b' will be renamed." << std::endl;
    std::cout << std::endl;

    int a; // Variable to store the integer input from the user (the offset)
    std::cout << "Enter an integer 'a' (the number to add for renaming): ";
    std::cin >> a; // Read the integer 'a' from the console

    // Check if the input for 'a' was successful. If not, print an error and exit.
    if (std::cin.fail()) {
        std::cerr << "Invalid input for 'a'. Please enter an integer." << std::endl;
        return 1; // Return with an error code
    }

    int b; // Variable to store the integer input from the user (the lower bound)
    std::cout << "Enter an integer 'b' (the minimum original number to rename): ";
    std::cin >> b; // Read the integer 'b' from the console

    // Check if the input for 'b' was successful. If not, print an error and exit.
    if (std::cin.fail()) {
        std::cerr << "Invalid input for 'b'. Please enter an integer." << std::endl;
        return 1; // Return with an error code
    }

    // Get the current working directory.
    fs::path current_dir = fs::current_path();
    std::cout << "Searching for files in: " << current_dir << std::endl;

    // Define the regular expression to find files named "NUMBER.EXTENSION".
    // ^        - Asserts position at the start of the string.
    // (\d+)    - Captures one or more digits (the number part). This is the first capturing group.
    // \.       - Matches a literal dot (escaped because '.' is a special regex character).
    // (.+)     - Captures one or more of any characters (the extension part). This is the second capturing group.
    // $        - Asserts position at the end of the string.
    std::regex filename_regex("^(\\d+)\\.(.+)$");
    std::smatch matches; // Object to store the results of the regex match

    std::vector<FileInfo> files_to_rename; // Vector to store information about files that match our pattern

    try {
        // Iterate through all entries (files and directories) in the current directory.
        for (const auto& entry : fs::directory_iterator(current_dir)) {
            // Check if the current entry is a regular file (not a directory, symlink, etc.).
            if (entry.is_regular_file()) {
                // Get the filename as a string (e.g., "5.txt")
                std::string filename = entry.path().filename().string();

                // Attempt to match the filename against our regex pattern.
                if (std::regex_match(filename, matches, filename_regex)) {
                    // If a match is found:
                    try {
                        // Extract the number part (first capturing group) and convert to int.
                        int number = std::stoi(matches[1].str());
                        // Extract the extension part (second capturing group).
                        std::string extension = matches[2].str();
                        // Add the file's information to our vector.
                        files_to_rename.push_back({number, entry.path(), extension});
                    } catch (const std::invalid_argument& e) {
                        // Handle error if the captured number string cannot be converted to an integer.
                        std::cerr << "Warning: Could not convert number part of '" << filename << "': " << e.what() << std::endl;
                    } catch (const std::out_of_range& e) {
                        // Handle error if the number is too large to fit in an int.
                        std::cerr << "Warning: Number part of '" << filename << "' is out of range: " << e.what() << std::endl;
                    }
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        // Catch any errors that occur during directory iteration (e.g., permissions issues).
        std::cerr << "Error accessing directory: " << e.what() << std::endl;
        return 1; // Return with an error code
    }

    // If no matching files were found, inform the user and exit.
    if (files_to_rename.empty()) {
        std::cout << "No files matching 'NUMBER.EXTENSION' found in the current directory." << std::endl;
        return 0; // Exit successfully
    }

    // Sort the list of files based on the value of 'a'.
    // This is a crucial step to prevent renaming conflicts.
    if (a >= 0) {
        // If 'a' is positive or zero, we want to rename files from the highest original number
        // down to the lowest. This ensures that a file like '5.txt' changing to '7.txt'
        // doesn't conflict with an existing '7.txt' that itself needs to be renamed (e.g., to '9.txt').
        std::sort(files_to_rename.begin(), files_to_rename.end(), compareFilesDesc);
        std::cout << "Sorting files from highest original number to lowest for renaming..." << std::endl;
    } else {
        // If 'a' is negative, we want to rename files from the lowest original number
        // up to the highest. This ensures that a file like '7.txt' changing to '5.txt'
        // doesn't conflict with an existing '5.txt' that itself needs to be renamed (e.g., to '3.txt').
        std::sort(files_to_rename.begin(), files_to_rename.end(), compareFilesAsc);
        std::cout << "Sorting files from lowest original number to highest for renaming..." << std::endl;
    }

    std::cout << "\nAttempting to rename files:\n";
    // Iterate through the sorted list of files and perform the renaming.
    for (const auto& file_info : files_to_rename) {
        int old_number = file_info.number; // Original number of the file
        int new_number = old_number + a;   // Calculate the new number

        // Get the string representation of the original filename for messages.
        std::string original_filename_str = file_info.original_path.filename().string();

        // New feature: Skip if the original number is less than 'b'
        if (old_number < b) {
            std::cout << "Skipping '" << original_filename_str
                      << "': Original number (" << old_number
                      << ") is less than 'b' (" << b << ")." << std::endl;
            continue; // Move to the next file
        }

        // If the new number would be negative, skip renaming this file and inform the user.
        // Filenames typically do not start with negative numbers.
        if (new_number < 0) {
            std::cout << "Skipping '" << original_filename_str
                      << "': New number (" << new_number << ") would be negative. "
                      << "New filenames must be non-negative." << std::endl;
            continue; // Move to the next file
        }

        // Construct the new filename string (e.g., "7.txt")
        std::string new_filename_str = std::to_string(new_number) + "." + file_info.extension;
        
        // Check if the new filename is identical to the original filename.
        // This can happen if 'a' is 0. If so, there's no need to rename.
        if (original_filename_str == new_filename_str) {
            std::cout << "Skipping '" << original_filename_str
                      << "': New filename is identical to original." << std::endl;
            continue; // Move to the next file
        }

        // Construct the full new path for the file. It's in the same directory as the original.
        fs::path new_path = file_info.original_path.parent_path() / new_filename_str;

        try {
            // Attempt to rename the file.
            fs::rename(file_info.original_path, new_path);
            std::cout << "Renamed '" << original_filename_str << "' to '" << new_filename_str << "'" << std::endl;
        } catch (const fs::filesystem_error& e) {
            // Catch and report any errors during the renaming process (e.g., permissions, file in use).
            std::cerr << "Error renaming '" << original_filename_str << "' to '" << new_filename_str << "': " << e.what() << std::endl;
        }
    }

    std::cout << "\nRenaming process complete." << std::endl;

    // Keep the console window open on Windows until the user presses Enter,
    // so they can see the output.
    std::cout << "Press Enter to exit.";
    std::cin.ignore(); // Consume any leftover newline character from previous input
    std::cin.get();    // Wait for the user to press Enter

    return 0; // Exit successfully
}
