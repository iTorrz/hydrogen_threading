#pragma once

#include <string>
#include <vector>

// hydrogen language tokens
enum class TokenType
{exit, open_paren, close_paren, eq, int_lit, ident, let, semi };

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
        while(peek().has_value())
        {
            if(isalpha(peek().value()))
            {
                buf.push_back(consume());
                while(peek().has_value() && isalnum(peek().value()))
                    buf.push_back(consume());

                if (buf == "exit")
                {
                    tokens.push_back({.type = TokenType::exit});
                    buf.clear();
                }
                else if (buf == "let")
                {
                    tokens.push_back({.type = TokenType::let});
                    buf.clear();
                }
                else
                {
                    tokens.push_back({.type = TokenType::ident, .value = buf});
                    buf.clear();
                }
            }
            else if (isdigit(peek().value()))
            {
                buf.push_back(consume());
                while(peek().has_value() && isdigit(peek().value()))
                    buf.push_back(consume());
                tokens.push_back({.type = TokenType::int_lit, .value = buf});
                buf.clear();
            }
            else if (peek().value() == '=')
            {
                consume();
                tokens.push_back({.type = TokenType::eq});
            }
            else if (peek().value() == '(')
            {
                consume();
                tokens.push_back({.type = TokenType::open_paren});
            }
            else if (peek().value() == ')')
            {
                consume();
                tokens.push_back({.type = TokenType::close_paren});
            }
            else if (peek().value() == ';')
            {
                consume();
                tokens.push_back({.type = TokenType::semi});
            }
            else if(isspace(peek().value()))
            {
                consume();
            }
            else
            {
                cerr << "You messed up! CHARACTERS CANNOT BE TOKENIZED" << endl;
                exit(EXIT_FAILURE);
            }
        }

        // WIP, I believe we still have to get the col number
        // at least for debugging purposes
        m_index = 0;
        return tokens;
    }

private:
    // peeking used to match the largest matching tokens.
    // Useful to differ tokens with similar values or chars
    [[nodiscard]] std::optional<char> peek(int offset = 0) const
    {
        if (m_index + offset >= m_src.length())
        {
            return {};
        }

        return m_src.at(m_index + offset);
    }

    char consume()
    {
        return m_src.at(m_index++);
    }

    // program source
    const std::string m_src;
    // m_src current index
    size_t m_index = 0;
};