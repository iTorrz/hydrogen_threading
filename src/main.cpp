#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

#include "./generation.hpp"

// main will consume characters from test.hy to create tokens
int main(int argc, char* argv[]) {
    // If we do not use std identifiers in function scopes, make this global.
    // If that causes weird behavior, revert to local namespace
    using namespace std;
    if (argc != 2)
    {
        cerr << "Incorrect usage. Correct usage is..." << endl;
        cerr << "hydro <input.hy>" << endl;
        return EXIT_FAILURE;
    }

    string contents;
    {
        stringstream contents_stream;
        fstream input(argv[1], ios::in);
        contents_stream << input.rdbuf();
        contents = contents_stream.str();
    }

    Tokenizer tokenizer(move(contents));
    vector<Token> tokens = tokenizer.tokenize();
    Parser parser(move(tokens));
    optional<NodeProg> prog = parser.parse_prog();

    if (!prog.has_value())
    {
        cerr << "Invalid Program" << endl;
        exit(EXIT_FAILURE);
    }

    Generator generator(prog.value());
    {
        fstream file("out.asm", ios::out);
        file << generator.gen_prog();
    }

    cout << "Code Generation Complete" << endl;

    system("nasm -felf64 out.asm");
    system("ld -o out out.o");

    return EXIT_SUCCESS;
}