#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>

using namespace std;

struct SecretFinding {
    string filePath;
    int lineNumber;
    string matchType;
    string matchedValue;
};

const vector<pair<string, regex>> SECRET_PATTERNS = {
    {"AWS Access Key ID", regex(R"(AKIA[0-9A-Z]{16})")},
    {"GitHub Personal Access Token", regex(R"(ghp_[a-zA-Z0-9]{36})")},
    {"Slack Token", regex(R"(xox[baprs]-[0-9a-zA-Z]{10,48})")},
    {"Generic API Key / Secret", regex(R"((api[_-]?key|secret[_-]?key|auth[_-]?token)['"]?\s*[:=]\s*['"]([a-zA-Z0-9_.-]{16,64})['"])", regex_constants::icase)}
};

void scanFile(const string& filePath, vector<SecretFinding>& findings) {
    ifstream file(filePath);
    if (!file.is_open()) return;

    string line;
    int lineNumber = 0;

    while (getline(file, line)) {
        lineNumber++;

        if (line.length() > 500) continue;

        for (const auto& patternPair : SECRET_PATTERNS) {
            smatch match;
            if (regex_search(line, match, patternPair.second)) {
                findings.push_back({
                    filePath,
                    lineNumber,
                    patternPair.first,
                    match.str()
                });
            }
        }
    }
}

bool endsWith(const string& str, const string& suffix) {
    if (str.length() >= suffix.length()) {
        return (str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0);
    }
    return false;
}

void scanDirectory(const string& dirPath, vector<SecretFinding>& findings) {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(dirPath.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            string filename = ent->d_name;
            if (filename == "." || filename == "..") continue;
            
            if (filename == ".git" || filename == "node_modules" || filename == "__pycache__") {
                continue;
            }

            string fullPath = dirPath + "/" + filename;
            
            struct stat st;
            if (stat(fullPath.c_str(), &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    scanDirectory(fullPath, findings);
                } else if (S_ISREG(st.st_mode)) {
                    if (endsWith(filename, ".png") || endsWith(filename, ".jpg") || 
                        endsWith(filename, ".zip") || endsWith(filename, ".exe") || 
                        endsWith(filename, ".pdf")) {
                        continue;
                    }
                    scanFile(fullPath, findings);
                }
            }
        }
        closedir(dir);
    } else {
        cerr << "Error opening directory: " << dirPath << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: scanner <directory_path>" << endl;
        return 1;
    }

    string targetDir = argv[1];
    
    struct stat st;
    if (stat(targetDir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        cerr << "Error: Target directory does not exist." << endl;
        return 1;
    }

    vector<SecretFinding> findings;
    scanDirectory(targetDir, findings);

    if (findings.empty()) {
        cout << "No secrets found. Clean codebase!" << endl;
    } else {
        cout << "ALERT: Secrets found (" << findings.size() << " total):\n";
        for (const auto& f : findings) {
            cout << "----------------------------------------\n";
            cout << "File: " << f.filePath << "\n";
            cout << "Line: " << f.lineNumber << "\n";
            cout << "Type: " << f.matchType << "\n";
            cout << "Match: " << f.matchedValue << "\n";
        }
    }

    return 0;
}