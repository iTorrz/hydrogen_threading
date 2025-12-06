#pragma once

#include <cassert>
#include <unordered_map>
#include "parser.hpp"

class Generator
{
  public:
     Generator(NodeProg prog)
      : m_prog(std::move(prog)){}

    void gen_term(const NodeTerm *term)
     {
       struct TermVisitor
       {
         Generator* gen;

         void operator()(const NodeTermIntLit *term_int_lit) const
         {
           gen->m_output << "    mov rax, " << term_int_lit->int_lit.value.value() << "\n";
           gen->push("rax");

         }

         void operator()(const NodeTermIdent *term_ident) const
         {
           //check if the identifier exists
           if (!gen->m_vars.contains(term_ident->ident.value.value()))
           {
             std::cerr << "Undeclared Identifier:  " << term_ident->ident.value.value() << std::endl;
             exit(EXIT_FAILURE);
           }
           const auto &var = gen->m_vars.at(term_ident->ident.value.value());
           std::stringstream offset;
           offset << "QWORD [rsp + " << (gen->m_stack_size - var.stack_loc - 1) * 8 << "]";
           gen->push(offset.str());
         }
       };

       TermVisitor visitor{.gen = this};
       std::visit(visitor, term->var);
     }


    void gen_expr(const NodeExpr *expr)
    {
      struct ExprVisitor
      {
        Generator* gen;

        void operator()(const NodeTerm *term) const
        {
          gen->gen_term(term);
        }

        void operator()(const NodeBinExpr *bin_expr) const
        {
          // push left and right handside on stack for register summation
          gen->gen_expr(bin_expr->add->lhs);
          gen->gen_expr(bin_expr->add->rhs);
          gen->pop("rax");
          gen->pop("rbx");
          gen->m_output << "    add rax, rbx\n";
          gen->push("rax");
        }
      };

      ExprVisitor visitor{.gen = this};
      std::visit(visitor, expr->var);
    }


    void gen_stmt(const NodeStmt *stmt)
    {
      struct StmtVisitor
      {
        Generator* gen;

        void operator()(const NodeStmtExit* stmt_exit) const
        {
          gen->gen_expr(stmt_exit->expr);
          gen->m_output << "    mov rax, 60\n";
          // popping expression evaluated from the stack
          gen->pop("rdi");
          gen->m_output << "    syscall\n";
        }

        void operator()(const NodeStmtLet *stmt_let) const
        {
          //check if a var is not already declared
          if (gen->m_vars.contains(stmt_let->ident.value.value()))
          {
            std::cerr << "Identifier already used " << stmt_let->ident.value.value() << std::endl;
            exit(EXIT_FAILURE);
          }
          // evaluate expression for variable assignment
          gen->m_vars.insert({stmt_let->ident.value.value(), Var {.stack_loc = gen->m_stack_size}});
          gen->gen_expr(stmt_let->expr);
        }

        void operator()(const NodeStmtAssign* stmt_assign) const
        {
          // check if the variable was declared
          const auto &name = stmt_assign->ident.value.value();
          if (!gen->m_vars.contains(name))
          {
            std::cerr << "Undeclared identifier in assignment: " << name << std::endl;
            std::exit(EXIT_FAILURE);
          }

          gen->gen_expr(stmt_assign->expr);
          gen->pop("rax");

          const auto &var = gen->m_vars.at(name);
          //get variable offset
          std::stringstream offset;
          offset << "QWORD [rsp + " << (gen->m_stack_size - var.stack_loc - 1) * 8 << "]";
          // Temp RAX save into offset, assigning loc in mem with new value
          gen->m_output << "    mov " << offset.str() << ", rax\n";
        }
      };

      StmtVisitor visitor{.gen = this};
      std::visit(visitor, stmt->var);
    }


    [[nodiscard]] std::string gen_prog()
      {
        m_output << "global _start\n_start:\n";
        for (const NodeStmt *stmt : m_prog.stmts)
          gen_stmt(stmt);

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