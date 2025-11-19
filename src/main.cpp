#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

// hydrogen language tokens
enum class TokenType
{
    _return,
    int_lit,
    semi
};

struct Token
{
    TokenType type;
    std::optional<std::string> value{};
};

std::vector<Token> tokenize(const std::string& str)
{
    using namespace std;
    vector<Token> tokens;
    string buf;
    for (int i = 0 ; i < str.length(); i++)
    {
        char c = str.at(i);
        if(isalpha(c))
        {
            buf.push_back(c);
            i++;
            while (isalnum(str.at(i)))
            {
                buf.push_back(str.at(i));
                i++;
            }
            i--;
            if (buf == "return")
            {
                tokens.push_back({.type = TokenType::_return});
                buf.clear();
                continue;
            }
            else
            {
                cerr << "You messed up! ALHPANUMERIC" << endl;
                exit(EXIT_FAILURE);
            }
        }
        else if (isdigit(c))
        {
            buf.push_back(c);
            i++;
            while (isdigit(str.at(i)))
            {
                buf.push_back(str.at(i));
                i++;
            }
            i--;

            tokens.push_back({.type = TokenType::int_lit, .value = buf});
            buf.clear();
        }
        else if (c == ';')
        {
            tokens.push_back({.type = TokenType::semi});
        }
        else if (isspace(c))
        {
            continue;
        }
        else
        {
            cerr << "You messed up! CHARACTERS CANNOT BE TOKENIZED" << endl;
            exit(EXIT_FAILURE);
        }
    }

    return tokens;
}



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

    cout << contents << endl;

    vector<Token> tokens = tokenize(contents);

    return EXIT_SUCCESS;
}