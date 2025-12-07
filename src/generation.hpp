#pragma once

#include <unordered_map>
#include <unordered_set>
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
           std::string name = term_ident->ident.value.value();
           if (!gen->m_globals.contains(name))
           {
             std::cerr << "Undeclared Global Identifier:  " << term_ident->ident.value.value() << std::endl;
             exit(EXIT_FAILURE);
           }
           std::stringstream addr;
           addr << "QWORD [rel " << name << "]";
           gen->push(addr.str());
           // if (!gen->m_vars.contains(term_ident->ident.value.value()))
           // {
           //   std::cerr << "Undeclared Identifier:  " << term_ident->ident.value.value() << std::endl;
           //   exit(EXIT_FAILURE);
           // }
           // const auto &var = gen->m_vars.at(term_ident->ident.value.value());
           // std::stringstream offset;
           // offset << "QWORD [rsp + " << (gen->m_stack_size - var.stack_loc - 1) * 8 << "]";
           // gen->push(offset.str());
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
          gen->m_output << ";NodeExit\n";
          gen->gen_expr(stmt_exit->expr);
          gen->m_output << "    mov rax, 60\n";
          // popping expression evaluated from the stack
          gen->pop("rdi");
          gen->m_output << "    syscall\n";
        }

        void operator()(const NodeStmtLet *stmt_let) const
        {
          ;
          // //check if a var is not already declared
          // if (gen->m_vars.contains(stmt_let->ident.value.value()))
          // {
          //   std::cerr << "Identifier already used " << stmt_let->ident.value.value() << std::endl;
          //   exit(EXIT_FAILURE);
          // }
          // // evaluate expression for variable assignment
          // gen->m_vars.insert({stmt_let->ident.value.value(), Var {.stack_loc = gen->m_stack_size}});
          // // this pops on the stack for local declaration
          // gen->gen_expr(stmt_let->expr);
        }

        void operator()(const NodeGlobalStmtLet *global_let) const
        {
          std::string name = global_let->ident.value.value();
          if (gen->in_worker)
          {
            std::cerr << "Global let inside worker not allowed\n";
            std::exit(EXIT_FAILURE);
          }
          if (gen->m_globals.contains(name)) {
            std::cerr << "Duplicate global variable: " << name << "\n";
            std::exit(EXIT_FAILURE);
          }
          // add globals to .bss
          gen->m_globals.insert(name);
          gen->m_output << "    " << name << ": resq 1\n";
        }

        void operator()(const NodeStmtStart *stmt_start) const
        {
          // start main for pthread calls
          gen->m_output << "section .text\n";
          gen->m_output << "main:\n";
          gen->m_output << "    push rbp\n";
          gen->m_output << "    mov rbp, rsp\n";
          gen->m_output << "    sub rsp, 16\n";
          gen->m_output << "    ;globals init\n";
          for (auto *stmt : gen->m_prog.stmts)
          {
            // if is NodeGlobalStmtLet
            if (auto global_stmt = std::get_if<NodeGlobalStmtLet*>(&stmt->var))
            {
              NodeGlobalStmtLet * g = *global_stmt;
              gen->gen_expr(g->expr);
              gen->pop("rax");
              gen->m_output
                << "    mov [rel " << g->ident.value.value() << "], rax\n";
            }
          }

          // Spawn workers
          std::size_t i = 0;
          for (const NodeWorker *worker : gen->m_prog.workers)
          {
            const std::string worker_name = worker->ident.value.value();

            gen->m_output << "    ; pthread_create for " << worker_name << "\n";
            // pthread_create(&thread_ids[i], NULL, worker_fn, NULL)
            gen->m_output << "    lea rdi, [rel thread_ids + " << (i * 8) << "]\n";
            gen->m_output << "    xor rsi, rsi\n";
            gen->m_output << "    lea rdx, [rel " << worker_name << "]\n";
            gen->m_output << "    xor rcx, rcx\n";
            gen->m_output << "    call pthread_create\n";

            ++i;
          }

          // Join workers
          i = 0;
          for (const NodeWorker *w : gen->m_prog.workers)
          {
            // pthread_join(&thread_ids[i], NULL)
            gen->m_output << "    ; pthread_join for worker " << i << "\n";
            gen->m_output << "    mov rdi, [rel thread_ids + " << (i * 8) << "]\n";
            gen->m_output << "    xor rsi, rsi\n";
            gen->m_output << "    call pthread_join\n";

            ++i;
          }

          // Print after join
          gen->m_output << "    ; print a\n";
          gen->m_output << "    mov rdi, fmt_a\n";
          gen->m_output << "    mov rsi, [rel a]\n";
          gen->m_output << "    xor rax, rax\n";
          gen->m_output << "    call printf\n";

          gen->m_output << "    ; print b\n";
          gen->m_output << "    mov rdi, fmt_b\n";
          gen->m_output << "    mov rsi, [rel b]\n";
          gen->m_output << "    xor rax, rax\n";
          gen->m_output << "    call printf\n";

          // Return from main
          gen->m_output << "    mov eax, 0\n";
          gen->m_output << "    leave\n";
          gen->m_output << "    ret\n";
        }

        void operator()(const NodeStmtAssign* stmt_assign) const
        {
          // check if the variable was declared
          const auto &name = stmt_assign->ident.value.value();
          if (!gen->m_globals.contains(name))
          {
            std::cerr << "Undeclared global identifier in assignment: " << name << std::endl;
            std::exit(EXIT_FAILURE);
          }

          gen->gen_expr(stmt_assign->expr);
          gen->pop("rax");
          // Store into global memory
          gen->m_output << "    mov [rel " << name << "], rax\n";
          // if (!gen->m_vars.contains(name))
          // {
          //   std::cerr << "Undeclared identifier in assignment: " << name << std::endl;
          //   std::exit(EXIT_FAILURE);
          // }
          //
          // gen->gen_expr(stmt_assign->expr);
          // gen->pop("rax");
          //
          // const auto &var = gen->m_vars.at(name);
          // //get variable offset
          // std::stringstream offset;
          // offset << "QWORD [rsp + " << (gen->m_stack_size - var.stack_loc - 1) * 8 << "]";
          // // Temp RAX save into offset, assigning loc in mem with new value
          // gen->m_output << "    mov " << offset.str() << ", rax\n";
        }
      };

      StmtVisitor visitor{.gen = this};
      std::visit(visitor, stmt->var);
    }

    void gen_worker(const NodeWorker *worker)
     {
       m_output << worker->ident.value.value() << ":\n";
       m_output << "    push rbp\n";
       m_output << "    mov rbp, rsp\n";

       // resetting local scope, and worker state will affect statement handling
       // TODO: Once I figure out local handling, this is will be useful
       this->m_stack_size = 0;
       this->in_worker = true;
       for (const NodeStmt *stmt : worker->body)
         gen_stmt(stmt);

       // Return NULL for pthread
       m_output << "    xor rax, rax\n";
       m_output << "    pop rbp\n";
       m_output << "    ret\n";
       in_worker = false;
     }


    [[nodiscard]] std::string gen_prog()
     {
       if (!m_prog.workers.empty())
       {
         m_output << "default rel\n";
         m_output << "section .text\n";
         m_output << "    global main\n";
         m_output << "    extern pthread_create\n";
         m_output << "    extern pthread_join\n";
         m_output << "    extern printf\n";

         m_output << "section .rodata\n";
         m_output << "    fmt_a: db \"a: %ld\", 10, 0\n";
         m_output << "    fmt_b: db \"b: %ld\", 10, 0\n";

         // undeclared thread ids
         m_output << "section .bss\n";
         m_output << "    thread_ids: resq " << m_prog.workers.size() << "\n";

         for (const NodeStmt *stmt : m_prog.stmts)
           gen_stmt(stmt);

         for (const NodeWorker *worker : m_prog.workers)
           gen_worker(worker);

         return m_output.str();
       }

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

      // could include "types", int literals are enough to test asm threading
      struct Var
      {
        size_t stack_loc = 0;
      };

      const NodeProg m_prog;
      std::stringstream m_output;
      size_t m_stack_size = 0;
      bool in_worker = false;
      // we are just doing global for now
      // std::unordered_map<std::string, Var> m_vars {};
      std::unordered_set<std::string> m_globals {};
    };