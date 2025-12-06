#pragma once

#include <vector>
#include "tokenization.hpp"
#include <variant>

struct NodeExprIntLit
{
    Token int_lit;
};

struct NodeExprIdent
{
    Token ident;
};

struct NodeExpr
{
    // variant is able to decide which type specified
    std::variant<NodeExprIntLit, NodeExprIdent> var;
};

struct NodeStmtExit
{
    NodeExpr expr;
};

struct NodeStmtLet
{
    Token ident;
    NodeExpr expr;
};

struct NodeStmt
{
    std::variant<NodeStmtExit, NodeStmtLet> var;
};

struct NodeProg
{
    std::vector<NodeStmt> stmts;
};

class Parser
{
public:
    inline explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)){}

    std::optional<NodeExpr> parse_expr()
    {
        if (peek().has_value() && peek().value().type == TokenType::int_lit)
            return NodeExpr{.var = NodeExprIntLit {.int_lit = consume()}};
        else if (peek().has_value() && peek().value().type == TokenType::ident)
            return NodeExpr{.var = NodeExprIdent {.ident = consume()}};

        return{};
    }

    std::optional<NodeStmt> parse_stmt()
    {
        using namespace std;
        if(peek().value().type == TokenType::exit &&
                peek(1).has_value() && peek(1).value().type == TokenType::open_paren)
        {
            // consuming exit and open_paren Token
            consume();
            consume();
            NodeStmtExit stmt_exit;
            // parse for an expression
            if (auto node_expr = parse_expr())
            {
                stmt_exit = {.expr = node_expr.value()};
            }
            else
            {
                cerr << "Invalid expression" << endl;
                exit(EXIT_FAILURE);
            }
            if (peek().has_value() && peek().value().type == TokenType::close_paren)
                consume();
            else
            {
                cerr << "Expected `)`" << endl;
                exit(EXIT_FAILURE);
            }
            if (peek().has_value() && peek().value().type == TokenType::semi)
                consume();
            else
            {
                cerr << "Expected `;`" << endl;
                exit(EXIT_FAILURE);
            }
            return NodeStmt {.var = stmt_exit};
        }
        // check for identifiers initialization
        else if (peek().value().type == TokenType::let &&
                peek(1).has_value() && peek(1).value().type == TokenType::ident &&
                peek(2).has_value() && peek(2).value().type == TokenType::eq)
        {
            consume();
            auto stmt_let = NodeStmtLet {.ident = consume()};
            consume();
            if (auto expr = parse_expr())
            {
                stmt_let.expr = expr.value();
            }
            else
            {
                // for future reference will have to include var name
                cerr << "Invalid expression for var init" << endl;
            }
            if (peek().has_value() && peek().value().type == TokenType::semi)
                consume();
            else
            {
                cerr << "Expected `;`" << endl;
                exit(EXIT_FAILURE);
            }
            return NodeStmt {.var = stmt_let};
        }
        return {};
    }

    std::optional<NodeProg> parse_prog()
    {
        using namespace std;
        NodeProg prog;
        while(peek().has_value())
        {
            if (auto stmt = parse_stmt())
            {
                prog.stmts.push_back(stmt.value());
            }
            else
            {
                cerr << "Invalid statement " << endl;
                exit(EXIT_FAILURE);
            }
        }
        // is this still needed?
        m_index = 0;
        return prog;
    }

private:
    [[nodiscard]] std::optional<Token> peek(int offset = 0) const
    {
        if (m_index + offset >= m_tokens.size())
        {
            return {};
        }
        else
        {
            return m_tokens.at(m_index + offset);
        }
    }

    Token consume()
    {
        return m_tokens.at(m_index++);
    }

    // program source
    size_t m_index = 0;
    const std::vector<Token> m_tokens;
};