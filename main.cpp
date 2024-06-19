  
#include <cassert> 
#include <filesystem> 
#include <fstream> 
#include <iostream> 
#include <regex> 
#include <sstream> 
#include <string> 
#include <vector> 

using namespace std;
using namespace std::filesystem;

path operator""_p(const char* data, size_t sz) {
    return path(data, data + sz);
}

bool FindInFolder(const path& folder, const path& file) {
    if (!exists(folder) || !is_directory(folder)) {
        return false;
    }

    // Check if the file exists directly in the folder
    path file_in_folder = folder / file;
    if (exists(file_in_folder) && is_regular_file(file_in_folder)) {
        return true;
    }

    // Recursively search in subdirectories
    for (const auto& dir_entry : directory_iterator(folder)) {
        if (dir_entry.is_directory() && FindInFolder(dir_entry.path(), file)) {
            return true;
        }
    }

    return false;
}

int FindInDirectory(const vector<path>& include_directories, const path& file_find) {
    int index = 0;
    for (const path& path_directory : include_directories) {
        if (FindInFolder(path_directory, file_find)) {
            return index;
        }
        index++;
    }
    return -1;
}

bool OutpUnknownInclude(const path& p, const path& par, int strn) {
    cout << "unknown include file " << p.string()
         << " at file " << par.string()
         << " at line " << strn << endl;
    return false;
}

bool ReadFile(ifstream& in, ofstream& out, const path& in_file, const vector<path>& include_directories) {
    path parent_folder = in_file.parent_path();
    int string_number = 1;

    string file_string;
    while (getline(in, file_string)) {
        static regex user_include(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
        static regex system_include(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");

        smatch m;
        path include_file_path;

        if (regex_match(file_string, m, user_include)) {
            string include_file_name = m[1].str();
            include_file_path = parent_folder / include_file_name;

            if (!exists(include_file_path)) {
                int index = FindInDirectory(include_directories, include_file_name);
                if (index >= 0) {
                    include_file_path = include_directories[index] / include_file_name;
                } else {
                    return OutpUnknownInclude(include_file_name, in_file, string_number);
                }
            }

            ifstream included_file(include_file_path.string(), ios::binary);
            if (!included_file) {
                return OutpUnknownInclude(include_file_name, in_file, string_number);
            }

            ReadFile(included_file, out, include_file_path, include_directories);
        } else if (regex_match(file_string, m, system_include)) {
            string include_file_name = m[1].str();
            int index = FindInDirectory(include_directories, include_file_name);

            if (index >= 0) {
                include_file_path = include_directories[index] / include_file_name;

                ifstream included_file(include_file_path.string(), ios::binary);
                if (!included_file) {
                    return OutpUnknownInclude(include_file_name, in_file, string_number);
                }

                ReadFile(included_file, out, include_file_path, include_directories);
            } else {
                return OutpUnknownInclude(include_file_name, in_file, string_number);
            }
        } else {
            out << file_string << '\n';
        }

        string_number++;
    }

    return true;
}
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories) {
    ifstream in(in_file.string(), ios::binary);
    if (!in) {
        cerr << "Failed to open input file: " << in_file << endl;
        return false;
    }

    ofstream out(out_file.string(), ios::binary);
    if (!out) {
        cerr << "Failed to open output file: " << out_file << endl;
        return false;
    }

    bool result = ReadFile(in, out, in_file, include_directories);

    in.close();
    out.close();

    return result;
}
string GetFileContents(string file) {
    ifstream stream(file);

    // Construct string from two iterators
    return {istreambuf_iterator<char>(stream), istreambuf_iterator<char>()};
}

void Test() { 
    error_code err; 
    std::filesystem::remove_all("sources"_p, err); 
    std::filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err); 
    std::filesystem::create_directories("sources"_p / "include1"_p, err); 
    std::filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err); 
 
    { 
        ofstream file("sources/a.cpp"); 
        file << "// this comment before include\n" 
                "#include \"dir1/b.h\"\n" 
                "// text between b.h and c.h\n" 
                "#include \"dir1/d.h\"\n" 
                "\n" 
                "int SayHello() {\n" 
                "    cout << \"hello,world!\" << endl;\n" 
                "#   include<dummy.txt>\n" 
                "}\n"s; 
    } 
    { 
        ofstream file("sources/dir1/b.h"); 
        file << "// text from b.h before include\n" 
                "#include \"subdir/c.h\"\n" 
                "// text from b.h after include"s; 
    } 
    { 
        ofstream file("sources/dir1/subdir/c.h"); 
        file << "// text from c.h before include\n" 
                "#include <std1.h>\n" 
                "// text from c.h after include\n"s; 
    } 
    { 
        ofstream file("sources/dir1/d.h"); 
        file << "// text from d.h before include\n" 
                "#include \"lib/std2.h\"\n" 
                "// text from d.h after include\n"s; 
    } 
    { 
        ofstream file("sources/include1/std1.h"); 
        file << "// std1\n"s; 
    } 
    { 
        ofstream file("sources/include2/lib/std2.h"); 
        file << "// std2\n"s; 
    } 
 
    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p, 
                        {"sources"_p / "include1"_p,"sources"_p / "include2"_p}))); 
 
    ostringstream test_out; 
    test_out << "// this comment before include\n" 
                "// text from b.h before include\n" 
                "// text from c.h before include\n" 
                "// std1\n" 
                "// text from c.h after include\n" 
                "// text from b.h after include\n" 
                "// text between b.h and c.h\n" 
                "// text from d.h before include\n" 
                "// std2\n" 
                "// text from d.h after include\n" 
                "\n" 
                "int SayHello() {\n" 
                "    cout << \"hello, world!\" << endl;\n"s; 
 
    assert(GetFileContents("sources/a.in"s) == test_out.str()); 
} 
 
int main() { 
    Test(); 
}
   
