#pragma once

#include <iostream>
#include <memory>
#include <stack>

static int current_id = 0;
static std::stack<int> nums;

// 所有 AST 的基类
class BaseAST {
 public:
  virtual ~BaseAST() = default;

  virtual void Dump() const = 0;
  virtual void KoopaIR() const = 0;
};

class CompUnitAST : public BaseAST {
 public:
  // 用智能指针管理对象
  std::unique_ptr<BaseAST> func_def;

  void Dump() const override {
    std::cout << "CompUnitAST { ";
    func_def->Dump();
    std::cout << " }";
  }
  void KoopaIR() const override {
    func_def->KoopaIR();
  }
};

class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;

  void Dump() const override {
    std::cout << "FuncDefAST { ";
    func_type->Dump();
    std::cout << ", " << ident << ", ";
    block->Dump();
    std::cout << " }";
  }
  void KoopaIR() const override {
    std::cout << "fun @"<<ident<<"(): ";
    func_type->KoopaIR();
    std::cout << " {\n";
    block->KoopaIR();
    std::cout << "}";
  }
};

class FuncTypeAST : public BaseAST {
 public:
  std::string type;

  void Dump() const override {
    std::cout << "FuncDefAST { \"int\" }";
  }
  void KoopaIR() const override {
    std::cout << type;
  }
};

class BlockAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> stmt;

  void Dump() const override {
    std::cout << "BlockAST { ";
    stmt->Dump();
    std::cout << " }";
  }
  void KoopaIR() const override {
    std::cout << "%entry:\n";
    stmt->KoopaIR();
    if(nums.empty())
      std::cout << "  ret %"<< current_id-1 << std::endl;
    else
    {
      std::cout << "  ret "<< nums.top() << std::endl;
      nums.pop();
    }
  }
};

class StmtAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> Exp;

  void Dump() const override {
    std::cout << "StmtAST { ";
    Exp->Dump();
    std::cout << " }";
  }
  void KoopaIR() const override {
    Exp->KoopaIR();
  }
};

class ExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> unary_exp;

  void Dump() const override {
    std::cout << "EXPAST { ";
    unary_exp->Dump();
    std::cout << " }";
  }
  void KoopaIR() const override {
    unary_exp->KoopaIR();
  }
};

class PrimaryExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> number;

  void Dump() const override {
    std::cout << "PrimaryExpAST { ";
    if (exp) {
      std::cout<<"(";
      exp->Dump();
      std::cout<<")";
    } else {
      number->Dump();
    }
    std::cout << " }";
  }
  void KoopaIR() const override {
    if (exp) {
      exp->KoopaIR();
    } else {
      number->KoopaIR();
    }
  }
};

class UnaryExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> primary_exp;
  char unary_op;
  std::unique_ptr<BaseAST> unary_exp;

  void Dump() const override {
    std::cout << "UnaryExpAST { ";
    if (primary_exp) {
      primary_exp->Dump();
    } else {
      std::cout << unary_op;
      unary_exp->Dump();
    }
    std::cout << " }";
  }
  void KoopaIR() const override {
    if (primary_exp) {
      primary_exp->KoopaIR();
    } else {
      unary_exp->KoopaIR();
      switch(unary_op)
      {
        case '-':
          if(nums.empty()){
            std::cout << "  %"<< current_id <<" = sub 0, %";
            std::cout << current_id-1 << std::endl;
            current_id++;
          }
          else{
            std::cout << "  %"<< current_id <<" = sub 0, ";
            std::cout << nums.top() << std::endl;
            current_id++;
            nums.pop();
          }
          break;
        case '!':
          if(nums.empty()){
            std::cout << "  %"<< current_id <<" = eq %";
            std::cout << current_id-1 << ", 0" << std::endl;
            current_id++;
          }
          else{
            std::cout << "  %"<< current_id <<" = eq ";
            std::cout << nums.top() << ", 0" << std::endl;
            current_id++;
            nums.pop();
          }
          break;
      }
    }
  }
};

class UnaryOpAST : public BaseAST {
 public:
  char op;

  void Dump() const override {
    std::cout << "UnaryOpAST { ";
    std::cout << op;
    std::cout << " }";
  }
  void KoopaIR() const override {
    std::cout << op;
  }
};

class NumberAST : public BaseAST {
 public:
  std::int32_t n;

  void Dump() const override {
    std::cout << "NumberAST { ";
    std::cout << n;
    std::cout << " }";
  }
  void KoopaIR() const override {
    nums.push(n);
  }
};
