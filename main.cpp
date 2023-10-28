#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

// напишите эту функцию
bool Preprocessor(ifstream& in, ofstream& out, const path& current_path, const vector<path>& include_directories) {

    int line_numb = 0;

    string hd;
    static regex num_reg1(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");  // сырой литерал #include "..."
    static regex num_reg2(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");  // сырой литерал #include <...>
    smatch m;
    
    while (getline(in, hd)) {

    ++line_numb;
    ifstream read_file;
    path file_path; 
    bool existance = false;

    if (regex_match(hd, m, num_reg1)) {
        path par_path = current_path.parent_path();
        
        read_file.open(par_path / string(m[1])); 

        if (read_file.is_open()) { 
        existance = true; 
        file_path = par_path / string(m[1]);
        }
        else 
        {
                for (const path& p : include_directories) {
                read_file.open(p / string(m[1]));
                if (read_file.is_open()) {
                file_path = p / string(m[1]);
                existance = true;
                } 
            }
            if (existance == false) {
            cout << "unknown include file " << m[1] << " at file " << current_path.string() << " at line " << line_numb << endl; 
            return false; }
        }
        }
    else if (regex_match(hd, m, num_reg2)) {
        path p2 = string(m[1]);

        for (const path& p : include_directories) {
            read_file.open(p / p2);
        if (read_file.is_open()) {
            file_path = p / p2;
            existance = true;
            break;
        }
        }
        if (existance == false) {
            cout << "unknown include file " << m[1] << " at file " << current_path.string() << " at line " << line_numb << endl;
            return false; 
        }
    }
    else {
        out << hd << endl;
    }
    if (existance == true) {
        Preprocessor(read_file, out, file_path, include_directories); 
        read_file.close(); 
    }
    }

return true;
}

bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories) {
 
 
    ifstream in(in_file, ios::in | ios::binary);
    if (!in) {
        return false;
    }

    ofstream out(out_file, ios::out | ios::app | ios::binary);
    if (!out) {
        return false;
    }
 
    return Preprocessor(in, out, in_file, include_directories);
}

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
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
