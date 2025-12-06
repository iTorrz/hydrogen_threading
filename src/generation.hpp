#pragma once

#include <unordered_map>
#include "parser.hpp"

class Generator
{
  public:
    inline Generator(NodeProg prog)
      : m_prog(std::move(prog)){}

    void gen_expr(const NodeExpr& expr)
    {
      struct ExprVisitor
      {
        Generator* gen;

        void operator()(const NodeExprIntLit& expr_int_lit) const
        {
          gen->m_output << "    mov rax, " << expr_int_lit.int_lit.value.value() << "\n";
          gen->push("rax");
        }

        void operator()(const NodeExprIdent& expr_ident) const
        {
          //check if the identifier exists
          if (!gen->m_vars.contains(expr_ident.ident.value.value()))
          {
            std::cerr << "Undeclared Identifier:  " << expr_ident.ident.value.value() << std::endl;
            exit(EXIT_FAILURE);
          }
          const auto &var = gen->m_vars.at(expr_ident.ident.value.value());
          std::stringstream offset;
          offset << "QWORD [rsp + " << (gen->m_stack_size - var.stack_loc - 1) * 8 << "]";
          gen->push(offset.str());
        }
      };

      ExprVisitor visitor{.gen = this};
      std::visit(visitor, expr.var);
    }


    void gen_stmt(const NodeStmt& stmt)
    {
      struct StmtVisitor
      {
        Generator* gen;

        void operator()(const NodeStmtExit& stmt_exit) const
        {
          gen->gen_expr(stmt_exit.expr);
          gen->m_output << "    mov rax, 60\n";
          // popping expression evaluated from the stack
          gen->pop("rdi");
          gen->m_output << "    syscall\n";
        }
        void operator()(const NodeStmtLet& stmt_let) const
        {
          //check if a var is not already declared
          if (gen->m_vars.contains(stmt_let.ident.value.value()))
          {
            std::cerr << "Identifier already used " << stmt_let.ident.value.value() << std::endl;
            exit(EXIT_FAILURE);
          }
          // evaluate expression for variable assignment
          gen->m_vars.insert({stmt_let.ident.value.value(), Var {.stack_loc = gen->m_stack_size}});
          gen->gen_expr(stmt_let.expr);
        }
      };

      StmtVisitor visitor{.gen = this};
      std::visit(visitor, stmt.var);
    }


    [[nodiscard]] std::string gen_prog()
      {
        m_output << "global _start\n_start:\n";
        for (const NodeStmt& stmt : m_prog.stmts)
          gen_stmt(stmt);

        // default case if no EXIT specified in program
        // This could be cleaned up with conditional checking
        // if there is already a return syscall though that is not TOO important
        m_output << "    mov rax, 60\n";
        m_output << "    mov rdi, 0\n";
        m_output << "    syscall\n";

        return m_output.str();
      }


  private:

      // function to keep track of current identifier or literal location in registers
      void push(const std::string &reg)
      {
        m_output << "    push " << reg << "\n";
        m_stack_size++;
      }

      void pop(const std::string &reg)
      {
        m_output << "    pop " << reg << "\n";
        m_stack_size--;
      }

      // NOTE: this could include "types", but for this project int
      // literals are enough to test asm threading
      struct Var
      {
        size_t stack_loc;
      };

      const NodeProg m_prog;
      std::stringstream m_output;
      size_t m_stack_size = 0;
      //program variables
      std::unordered_map<std::string, Var> m_vars {};
};