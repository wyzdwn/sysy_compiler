#pragma once

#include <iostream>
#include <memory>

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
  }
};

class StmtAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> number;

  void Dump() const override {
    std::cout << "StmtAST { ";
    number->Dump();
    std::cout << " }";
  }
  void KoopaIR() const override {
    std::cout << "  ret ";
    number->KoopaIR();
    std::cout << "\n";
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
    std::cout << n;
  }
};
