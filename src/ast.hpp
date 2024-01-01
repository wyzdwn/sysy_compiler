#pragma once

#include <iostream>
#include <memory>
#include <stack>
#include <deque>
#include <string>
#include <unordered_map>

static int current_id = 0;
static std::deque<std::string> nums;

// 用于计算的操作符到 Koopa IR 指令的映射
static std::unordered_map<char, std::string> CalOp2Instruct={
  {'+', "add"},
  {'-', "sub"},
  {'*', "mul"},
  {'/', "div"},
  {'%', "rem"},
};
// 用于比较的操作符到 Koopa IR 指令的映射
static std::unordered_map<std::string, std::string> ComOp2Instruct={
  {"<", "lt"},
  {">", "gt"},
  {"<=", "le"},
  {">=", "ge"},
  {"==", "eq"},
  {"!=", "ne"},
};

inline void KoopaIR_one_operands(std::string instruct)
{
  std::cout << "  %"<< current_id << " = " << instruct <<" ";
  std::cout << nums.back() << std::endl;
  nums.pop_back();
  nums.push_back("%"+std::to_string(current_id));
  current_id++;
}
inline void KoopaIR_two_operands(std::string instruct)
{
  std::cout << "  %"<< current_id << " = " << instruct <<" ";
  std::cout << nums[nums.size()-2] << ", "<< nums.back() << std::endl;
  nums.pop_back();
  nums.pop_back();
  nums.push_back("%"+std::to_string(current_id));
  current_id++;
}
inline void KoopaIR_logic_operands(std::string instruct)
{
  KoopaIR_two_operands(instruct);
  KoopaIR_one_operands("ne 0,");
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
      KoopaIR_two_operands(CalOp2Instruct[add_op]);
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
        KoopaIR_two_operands(CalOp2Instruct[mul_op]);
      } else {
        unary_exp->KoopaIR();
      }
    }
};

class LOrExpAST: public BaseAST{
  public:
    std::unique_ptr<BaseAST> lor_exp;
    std::unique_ptr<BaseAST> land_exp;

    void Dump() const override {
      std::cout << "LOrExp { ";
      if (lor_exp) {
        lor_exp->Dump();
        std::cout << "||";
        land_exp->Dump();
      } else {
        land_exp->Dump();
      }
      std::cout << " }";
    }
    void KoopaIR() const override {
      if (lor_exp) {
        lor_exp->KoopaIR();
        land_exp->KoopaIR();
        KoopaIR_logic_operands("or");
      } else {
        land_exp->KoopaIR();
      }
    }
};

class LAndExpAST: public BaseAST{
  public:
    std::unique_ptr<BaseAST> land_exp;
    std::unique_ptr<BaseAST> eq_exp;

    void Dump() const override {
      std::cout << "LAndExp { ";
      if (land_exp) {
        land_exp->Dump();
        std::cout << "&&";
        eq_exp->Dump();
      } else {
        eq_exp->Dump();
      }
      std::cout << " }";
    }
    void KoopaIR() const override {
      if (land_exp) {
        land_exp->KoopaIR();
        eq_exp->KoopaIR();
        KoopaIR_logic_operands("and");
      } else {
        eq_exp->KoopaIR();
      }
    }
};

class EqExpAST: public BaseAST{
  public:
    std::unique_ptr<BaseAST> eq_exp;
    std::unique_ptr<BaseAST> rel_exp;
    std::string eq_op;

    void Dump() const override {
      std::cout << "EqExp { ";
      if (eq_exp) {
        eq_exp->Dump();
        std::cout << eq_op;
        rel_exp->Dump();
      } else {
        rel_exp->Dump();
      }
      std::cout << " }";
    }
    void KoopaIR() const override {
      if (eq_exp) {
        eq_exp->KoopaIR();
        rel_exp->KoopaIR();
        KoopaIR_two_operands(ComOp2Instruct[eq_op]);
      } else {
        rel_exp->KoopaIR();
      }
    }
};

class RelExpAST: public BaseAST{
  public:
    std::unique_ptr<BaseAST> rel_exp;
    std::unique_ptr<BaseAST> add_exp;
    std::string rel_op;

    void Dump() const override {
      std::cout << "RelExp { ";
      if (rel_exp) {
        rel_exp->Dump();
        std::cout << rel_op;
        add_exp->Dump();
      } else {
        add_exp->Dump();
      }
      std::cout << " }";
    }
    void KoopaIR() const override {
      if (rel_exp) {
        rel_exp->KoopaIR();
        add_exp->KoopaIR();
        KoopaIR_two_operands(ComOp2Instruct[rel_op]);
      } else {
        add_exp->KoopaIR();
      }
    }
};
