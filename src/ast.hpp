#pragma once

#include <iostream>
#include <memory>
#include <stack>
#include <deque>
#include <string>

static int current_id = 0;
static std::deque<std::string> nums;

inline void KoopaIR_one_operands(std::string op)
{
  std::cout << "  %"<< current_id << " = " << op <<" ";
  std::cout << nums.back() << std::endl;
  nums.pop_back();
  nums.push_back("%"+std::to_string(current_id));
  current_id++;
}


inline void KoopaIR_two_operands(std::string op)
{
  std::cout << "  %"<< current_id << " = " << op <<" ";
  std::cout << nums[nums.size()-2] << ", "<< nums.back() << std::endl;
  nums.pop_back();
  nums.pop_back();
  nums.push_back("%"+std::to_string(current_id));
  current_id++;
}
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
      std::cout << "  ret "<< nums.back() << std::endl;
      nums.pop_back();
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
  std::unique_ptr<BaseAST> add_exp;

  void Dump() const override {
    std::cout << "EXPAST { ";
    add_exp->Dump();
    std::cout << " }";
  }
  void KoopaIR() const override {
    add_exp->KoopaIR();
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
          KoopaIR_one_operands("sub 0,");
          break;
        case '!':
          KoopaIR_one_operands("eq 0,");
          break;
      }
    }
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
    nums.push_back(std::to_string(n));
  }
};

class AddExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> add_exp;
  std::unique_ptr<BaseAST> mul_exp;
  char add_op;

  void Dump() const override {
    std::cout << "AddExpAST { ";
    if (add_exp) {
      add_exp->Dump();
      std::cout << add_op;
      mul_exp->Dump();
    } else {
      mul_exp->Dump();
    }
    std::cout << " }";
  }
  void KoopaIR() const override {
    if (add_exp) {
      add_exp->KoopaIR();
      mul_exp->KoopaIR();
      switch(add_op)
      {
        case '+':
          KoopaIR_two_operands("add");
          break;
        case '-':
          KoopaIR_two_operands("sub");
          break;
      }
    } else {
      mul_exp->KoopaIR();
    }
  }
};

class MulExpAST: public BaseAST{
  public:
    std::unique_ptr<BaseAST> mul_exp;
    std::unique_ptr<BaseAST> unary_exp;
    char mul_op;
  
    void Dump() const override {
      std::cout << "MulExpAST { ";
      if (mul_exp) {
        mul_exp->Dump();
        std::cout << mul_op;
        unary_exp->Dump();
      } else {
        unary_exp->Dump();
      }
      std::cout << " }";
    }
    void KoopaIR() const override {
      if (mul_exp) {
        mul_exp->KoopaIR();
        unary_exp->KoopaIR();
        switch(mul_op)
        {
          case '*':
            KoopaIR_two_operands("mul");
            break;
          case '/':
            KoopaIR_two_operands("div");
            break;
          case '%':
            KoopaIR_two_operands("rem");
            break;
        }
      } else {
        unary_exp->KoopaIR();
      }
    }
};
