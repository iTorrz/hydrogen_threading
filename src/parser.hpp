#pragma once

#include <complex>
#include <vector>
#include "tokenization.hpp"
#include <variant>

#include "arena.hpp"


struct NodeTermIntLit
{
    Token int_lit;
};

struct NodeTermIdent
{
    Token ident;
};

struct NodeExpr;

struct NodeBinExprAdd
{
    NodeExpr* lhs;
    NodeExpr* rhs;
};

// Will not implement for this project
// struct NodeBinExprMulti
// {
//     NodeExpr* lhs;
//     NodeExpr* rhs;
// };

struct NodeBinExpr
{
    NodeBinExprAdd* add;
};

struct NodeTerm
{
    std::variant<NodeTermIntLit*, NodeTermIdent*> var;
};

struct NodeExpr
{
    // variant is able to decide which type specified
    std::variant<NodeTerm*, NodeBinExpr*> var;
};

struct NodeStmtExit
{
    NodeExpr* expr;
};

struct NodeStmtLet
{
    Token ident;
    NodeExpr* expr;
};

struct NodeStmtAssign
{
    Token ident;
    NodeExpr* expr;
};

struct NodeStmt
{
    std::variant<NodeStmtExit*, NodeStmtLet*,NodeStmtAssign*> var;
};

struct NodeProg
{
    std::vector<NodeStmt*> stmts;
};

class Parser
{
public:
     explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)),
          m_allocator(1024 * 1024 * 4){}


    std::optional<NodeTerm*> parse_term()
     {
         if (peek().has_value() && peek().value().type == TokenType::int_lit)
         {
             auto term_int_lit = m_allocator.alloc<NodeTermIntLit>();
             term_int_lit->int_lit = consume();
             auto term = m_allocator.alloc<NodeTerm>();
             term->var = term_int_lit;
             return term;
         }
         else if (peek().has_value() && peek().value().type == TokenType::ident)
         {
             auto term_ident = m_allocator.alloc<NodeTermIdent>();
             term_ident->ident = consume();
             auto term = m_allocator.alloc<NodeTerm>();
             term->var = term_ident;
             return term;
         }
         return {};
     }

    std::optional<NodeExpr*> parse_expr()
    {
         if (auto term = parse_term())
         {
             if (peek().has_value() && peek().value().type == TokenType::plus)
             {
                 auto bin_expr = m_allocator.alloc<NodeBinExpr>();
                 auto bin_expr_add = m_allocator.alloc<NodeBinExprAdd>();
                 auto lhs_expr = m_allocator.alloc<NodeExpr>();
                 lhs_expr->var = term.value();
                 bin_expr_add->lhs = lhs_expr;
                 consume();
                 if (auto rhs = parse_expr())
                 {
                     bin_expr_add->rhs = rhs.value();
                     bin_expr->add = bin_expr_add;
                     auto expr = m_allocator.alloc<NodeExpr>();
                     expr->var = bin_expr;
                     return expr;
                 }
             }
             else
             {
                 auto expr = m_allocator.alloc<NodeExpr>();
                 expr->var = term.value();
                 return expr;
             }
         }

        return{};
    }

    std::optional<NodeStmt*> parse_stmt()
    {
        using namespace std;
        if(peek().value().type == TokenType::exit &&
                peek(1).has_value() && peek(1).value().type == TokenType::open_paren)
        {
            // consuming exit and open_paren Token
            consume();
            consume();
            auto stmt_exit = m_allocator.alloc<NodeStmtExit>();
            // parse for an expression
            if (auto node_expr = parse_expr())
            {
                stmt_exit->expr = node_expr.value();
            }
            else
            {
                cerr << "Invalid expression" << endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paren, "Expected `)`");
            try_consume(TokenType::semi, "Expected `;`");
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_exit;
            return stmt;
        }
        // check for identifiers initialization
        else if (peek().value().type == TokenType::let &&
                peek(1).has_value() && peek(1).value().type == TokenType::ident &&
                peek(2).has_value() && peek(2).value().type == TokenType::eq)
        {
            consume();
            auto stmt_let = m_allocator.alloc<NodeStmtLet>();
            stmt_let->ident = consume();
            consume();
            if (auto expr = parse_expr())
            {
                stmt_let->expr = expr.value();
            }
            else
            {
                // for future reference will have to include var name
                cerr << "Invalid expression for var init" << endl;
            }
            try_consume(TokenType::semi, "Expected `;`");
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_let;
            return stmt;
        }
         // check for an identifier, should I create a new node type for the generator?
         // I think that we do have to make a new struct
         if(peek().value().type == TokenType::ident &&
                peek(1).has_value() && peek(1).value().type == TokenType::eq)
         {
             auto stmt_assign = m_allocator.alloc<NodeStmtAssign>();
             stmt_assign->ident = consume();
             consume();
             if (auto expr = parse_expr())
             {
                 stmt_assign->expr = expr.value();
             }
             else
             {
                 // for future reference will have to include var name
                 cerr << "Invalid expression for var assignment" << endl;
             }
             try_consume(TokenType::semi, "Expected `;`");
             auto stmt = m_allocator.alloc<NodeStmt>();
             stmt->var = stmt_assign;
             return stmt;
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

    Token try_consume(TokenType type, const std::string err_msg)
    {
        if (peek().has_value() && peek().value().type == type)
            return consume();
        else
        {
            std::cerr << err_msg << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // program source
    size_t m_index = 0;
    const std::vector<Token> m_tokens;
    ArenaAllocator m_allocator;
};