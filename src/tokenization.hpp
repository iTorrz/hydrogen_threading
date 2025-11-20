#pragma once

#include <string>
#include <vector>

// hydrogen language tokens
enum class TokenType
{
    exit,
    int_lit,
    semi
};

struct Token
{
    TokenType type;
    std::optional<std::string> value{};
};

class Tokenizer {
public:
    inline explicit Tokenizer(std::string src)
        : m_src(std::move(src))
    {

    }

    inline std::vector<Token> tokenize()
    {
        using namespace std;
        vector<Token> tokens;
        string buf;
        while(peak().has_value())
        {
            if(isalpha(peak().value()))
            {
                buf.push_back(consume());
                while(peak().has_value() && isalnum(peak().value()))
                    buf.push_back(consume());

                if (buf == "exit")
                {
                    tokens.push_back({.type = TokenType::exit});
                    buf.clear();
                    continue;
                }
                else
                {
                    cerr << "You messed up! ALPHANUMERIC" << endl;
                    exit(EXIT_FAILURE);
                }
            }
            else if (isdigit(peak().has_value()))
            {
                buf.push_back(consume());
                while(peak().has_value() && isdigit(peak().has_value()))
                    buf.push_back(consume());
                tokens.push_back({ .type = TokenType::int_lit, .value = buf });
                buf.clear();
                continue;
            }
            else if (peak().value() == ';')
            {
                consume();
                tokens.push_back({ .type = TokenType::semi});
                continue;
            }
            else if(isspace(peak().value()))
            {
                consume();
                continue;
            }
            else
            {
                cerr << "You messed up! CHARACTERS CANNOT BE TOKENIZED" << endl;
                exit(EXIT_FAILURE);
            }
        }

        // WIP, I believe we still have to get the col number
        m_index = 0;
        return tokens;
    }

private:
    // peeking used to match the largest matching tokens.
    // Useful to differ tokens with similar values or chars
    [[nodiscard]] std::optional<char> peak(int ahead = 1) const
    {
        if (m_index + ahead > m_src.length())
        {
            return {};
        }
        else
        {
            return m_src.at(m_index);
        }
    }

    char consume()
    {
        return m_src.at(m_index++);
    }

    // program source
    const std::string m_src;
    // m_src current index
    int m_index = 0;
};