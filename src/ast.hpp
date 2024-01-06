#pragma once

#include <iostream>
#include <memory>
#include <stack>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

static int current_id = 0;
static deque<string> nums;
static deque<unordered_map<string, int>*> symbol_table_stack;
static deque<unordered_map<string, string>*> symbol_type_stack;

static int fun_ret_flag=0;

// 用于计算的操作符到 Koopa IR 指令的映射
static unordered_map<char, string> CalOp2Instruct={
  {'+', "add"},
  {'-', "sub"},
  {'*', "mul"},
  {'/', "div"},
  {'%', "mod"},
};
// 用于比较的操作符到 Koopa IR 指令的映射
static unordered_map<string, string> ComOp2Instruct={
  {"<", "lt"},
  {">", "gt"},
  {"<=", "le"},
  {">=", "ge"},
  {"==", "eq"},
  {"!=", "ne"},
};

inline void KoopaIR_one_operands(string instruct)
{
  cout << "  %"<< current_id << " = " << instruct <<" ";
  cout << nums.back() << endl;
  nums.pop_back();
  nums.push_back("%"+to_string(current_id));
  current_id++;
}
inline void KoopaIR_two_operands(string instruct)
{
  cout << "  %"<< current_id << " = " << instruct <<" ";
  cout << nums[nums.size()-2] << ", "<< nums.back() << endl;
  nums.pop_back();
  nums.pop_back();
  nums.push_back("%"+to_string(current_id));
  current_id++;
}
inline void KoopaIR_logic_operands(string instruct)
{
  if(instruct=="and"){
    KoopaIR_two_operands("and");
  }
  else{
    KoopaIR_two_operands("or");
    KoopaIR_one_operands("ne 0,");
  }
}

// 从符号表栈中找到符号的标识符，返回一个vector: {符号的标识符, 类型(const/var), 符号的值}
inline vector<string> get_target_ident(string ident)
{
  vector<string> ret;
  for(int i=symbol_table_stack.size()-1; i>=0; i--){
    string target_ident = ident + "_" + to_string(i+1);
    if(symbol_table_stack[i]->find(target_ident)!=symbol_table_stack[i]->end()){
      ret.push_back(target_ident);
      ret.push_back(symbol_type_stack[i]->at(target_ident));
      ret.push_back(to_string(symbol_table_stack[i]->at(target_ident)));
      break;
    }
  }
  if(ret.size()==0)
  {
    ret.push_back("");
    ret.push_back("");
    ret.push_back("");
  }
  return ret;
}

// 所有 AST 的基类
class BaseAST {
 public:
  virtual ~BaseAST() = default;

  virtual void Dump() const = 0;
  virtual void KoopaIR() const = 0;
  virtual int Calculate() const = 0;
};

class CompUnitAST : public BaseAST {
 public:
  // 用智能指针管理对象
  unique_ptr<BaseAST> func_def;

  void Dump() const override {
    cout << "CompUnitAST { ";
    func_def->Dump();
    cout << " }";
  }
  void KoopaIR() const override {
    func_def->KoopaIR();
  }
  int Calculate() const override {
    return 0;
  }
};

class FuncDefAST : public BaseAST {
 public:
  unique_ptr<BaseAST> func_type;
  string ident;
  unique_ptr<BaseAST> block;

  void Dump() const override {
    cout << "FuncDefAST { ";
    func_type->Dump();
    cout << ", " << ident << ", ";
    block->Dump();
    cout << " }";
  }
  void KoopaIR() const override {
    cout << "fun @"<<ident<<"(): ";
    func_type->KoopaIR();
    cout << " {\n";
    cout << "%entry:\n";
    fun_ret_flag=0;
    block->KoopaIR();
    if(fun_ret_flag==0)
    {
      cout<<"  ret"<<endl;
    }
    cout << "}";
  }
  int Calculate() const override {
    return 0;
  }
};

class FuncTypeAST : public BaseAST {
 public:
  string type;

  void Dump() const override {
    cout << "FuncDefAST { \"int\" }";
  }
  void KoopaIR() const override {
    cout << type;
  }
  int Calculate() const override {
    return 0;
  }
};

class LValAST: public BaseAST{
  public:
    string ident;

    void Dump() const override {
      cout << "LVal { ";
      cout << ident;
      cout << " }";
    }
    void KoopaIR() const override {
      string target_ident = get_target_ident(ident)[0];
      string target_type = get_target_ident(ident)[1];
      string target_value = get_target_ident(ident)[2];
      if(target_ident=="") throw("undefined variable: " + ident);
      if(target_type=="const"){
        nums.push_back(target_value);
      }
      else{
        cout << "  %"<< current_id << " = load @" << target_ident << endl;
        nums.push_back("%"+to_string(current_id));
        current_id++;
      }
    }
    int Calculate() const override {
      return symbol_table_stack.back()->at(get_target_ident(ident)[0]);
    }
};

class BlockAST : public BaseAST {
 public:
  unique_ptr<vector<unique_ptr<BaseAST>>> block_item_list;

  void Dump() const override {
    if(!block_item_list) return;
    cout << "Block { ";
    for(auto &i:*block_item_list){
      i->Dump();
      cout << ", ";
    }
    cout << " }";
  }
  void KoopaIR() const override {
    if(!block_item_list || fun_ret_flag) return;
    unordered_map<string, int> *symbol_table=new unordered_map<string, int>;
    unordered_map<string, string> *symbol_type=new unordered_map<string, string>;
    symbol_table_stack.push_back(symbol_table);
    symbol_type_stack.push_back(symbol_type);
        for(auto &i:*block_item_list){
      i->KoopaIR();
    }
    symbol_table_stack.pop_back();
    free(symbol_table);
    symbol_type_stack.pop_back();
    free(symbol_type);
  }
  int Calculate() const override {
    return 0;
  }
};

class BlockItemAST: public BaseAST{
  public:
    unique_ptr<BaseAST> stmt;
    unique_ptr<BaseAST> decl;

    void Dump() const override {
      cout << "BlockItem { ";
      if(stmt){
        stmt->Dump();
        cout << ", ";
      }
      if(decl){
        decl->Dump();
        cout << ", ";
      }
      cout << " }";
    }
    void KoopaIR() const override {
      if(stmt){
        stmt->KoopaIR();
      }
      if(decl){
        decl->KoopaIR();
      }
      
    }
    int Calculate() const override {
      return 0;
    }
};

class StmtAST : public BaseAST {
 public:
  unique_ptr<BaseAST> exp;
  unique_ptr<BaseAST> lval;
  unique_ptr<BaseAST> block;
  unique_ptr<BaseAST> exp_only;
  bool return_;

  void Dump() const override {
    cout << "StmtAST { ";
    if(block){
      block->Dump();
    } else if(exp_only){
      exp_only->Dump(); 
    } else if (lval) {
      lval->Dump();
      cout << " = ";
      exp->Dump();
    } else if(return_){
      cout << "return ";
      if(exp) exp->Dump();
    }
    cout << " }";
  }
  void KoopaIR() const override {
    if (block){
      block->KoopaIR();
    } else if(exp_only){
      exp_only->KoopaIR();
    } else if (lval) {
      exp->KoopaIR();
      cout << "  store " << nums.back() << ", @";
      string ident = dynamic_cast<LValAST*>(lval.get())->ident;
      string target_ident = get_target_ident(ident)[0];
      if(target_ident=="") throw("undefined variable: " + ident);
      cout << target_ident << endl;
      nums.pop_back();
    } else if(return_){
      if(fun_ret_flag) return;
      if(!exp)
      {
        cout<<"  ret"<<endl;
        fun_ret_flag=1;
        return ;
      }
      exp->KoopaIR();
      cout<<"  ret "<<nums.back()<<endl;
      fun_ret_flag=1;
      nums.pop_back();
    }
    
    
  }
  int Calculate() const override {
    return 0;
  }
};

class ExpAST : public BaseAST {
 public:
  unique_ptr<BaseAST> lor_exp;

  void Dump() const override {
    cout << "EXPAST { ";
    lor_exp->Dump();
    cout << " }";
  }
  void KoopaIR() const override {
    lor_exp->KoopaIR();
  }
  int Calculate() const override {
    return lor_exp->Calculate();
  }
};

class PrimaryExpAST : public BaseAST {
 public:
  unique_ptr<BaseAST> exp;
  unique_ptr<BaseAST> number;
  unique_ptr<BaseAST> lval;

  void Dump() const override {
    cout << "PrimaryExpAST { ";
    if (exp) {
      exp->Dump();
    } else if (number) {
      number->Dump();
    } else {
      lval->Dump();
    }
    cout << " }";
  }
  void KoopaIR() const override {
    if (exp) {
      exp->KoopaIR();
    } else if (number) {
      number->KoopaIR();
    } else {
      lval->KoopaIR();
    }
  }
  int Calculate() const override {
    if (exp) {
      return exp->Calculate();
    } else if (number) {
      return number->Calculate();
    } else {
      return lval->Calculate();
    }
  }
};

class UnaryExpAST : public BaseAST {
 public:
  unique_ptr<BaseAST> primary_exp;
  char unary_op;
  unique_ptr<BaseAST> unary_exp;

  void Dump() const override {
    cout << "UnaryExpAST { ";
    if (primary_exp) {
      primary_exp->Dump();
    } else {
      cout << unary_op;
      unary_exp->Dump();
    }
    cout << " }";
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
  int Calculate() const override {
    if(primary_exp){
      return primary_exp->Calculate();
    }
    else{
      switch(unary_op)
      {
        case '-':
          return -unary_exp->Calculate();
        case '!':
          return !unary_exp->Calculate();
        case '+':
          return unary_exp->Calculate();
      }
    }
    return 0;
  }
};

class NumberAST : public BaseAST {
 public:
  int32_t n;

  void Dump() const override {
    cout << "NumberAST { ";
    cout << n;
    cout << " }";
  }
  void KoopaIR() const override {
    nums.push_back(to_string(n));
  }
  int Calculate() const override {
    return n;
  }
};

class AddExpAST : public BaseAST {
 public:
  unique_ptr<BaseAST> add_exp;
  unique_ptr<BaseAST> mul_exp;
  char add_op;

  void Dump() const override {
    cout << "AddExpAST { ";
    if (add_exp) {
      add_exp->Dump();
      cout << add_op;
      mul_exp->Dump();
    } else {
      mul_exp->Dump();
    }
    cout << " }";
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
  int Calculate() const override {
    if(add_exp){
      if(add_op=='+'){
        return add_exp->Calculate() + mul_exp->Calculate();
      }
      else{
        return add_exp->Calculate() - mul_exp->Calculate();
      }
    }
    else{
      return mul_exp->Calculate();
    }
  }
};

class MulExpAST: public BaseAST{
  public:
    unique_ptr<BaseAST> mul_exp;
    unique_ptr<BaseAST> unary_exp;
    char mul_op;
  
    void Dump() const override {
      cout << "MulExpAST { ";
      if (mul_exp) {
        mul_exp->Dump();
        cout << mul_op;
        unary_exp->Dump();
      } else {
        unary_exp->Dump();
      }
      cout << " }";
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
    int Calculate() const override {
      if(mul_exp){
        if(mul_op=='*'){
          return mul_exp->Calculate() * unary_exp->Calculate();
        }
        else if(mul_op=='/'){
          return mul_exp->Calculate() / unary_exp->Calculate();
        }
        else{
          return mul_exp->Calculate() % unary_exp->Calculate();
        }
      }
      else{
        return unary_exp->Calculate();
      }
    }
};

class LOrExpAST: public BaseAST{
  public:
    unique_ptr<BaseAST> lor_exp;
    unique_ptr<BaseAST> land_exp;

    void Dump() const override {
      cout << "LOrExp { ";
      if (lor_exp) {
        lor_exp->Dump();
        cout << "||";
        land_exp->Dump();
      } else {
        land_exp->Dump();
      }
      cout << " }";
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
    int Calculate() const override {
      if(lor_exp){
        return lor_exp->Calculate() || land_exp->Calculate();
      }
      else{
        return land_exp->Calculate();
      }
    }
};

class LAndExpAST: public BaseAST{
  public:
    unique_ptr<BaseAST> land_exp;
    unique_ptr<BaseAST> eq_exp;

    void Dump() const override {
      cout << "LAndExp { ";
      if (land_exp) {
        land_exp->Dump();
        cout << "&&";
        eq_exp->Dump();
      } else {
        eq_exp->Dump();
      }
      cout << " }";
    }
    void KoopaIR() const override {
      if (land_exp) {
        land_exp->KoopaIR();
        cout << "  %"<< current_id++ << " = ne 0, " << nums.back() << endl;
        nums.pop_back();
        nums.push_back("%"+to_string(current_id-1));

        eq_exp->KoopaIR();
        cout << "  %"<< current_id++ << " = ne 0, " << nums.back() << endl;
        nums.pop_back();
        nums.push_back("%"+to_string(current_id-1));
        
        KoopaIR_logic_operands("and");
      } else {
        eq_exp->KoopaIR();
      }
    }
    int Calculate() const override {
      if(land_exp){
        return land_exp->Calculate() && eq_exp->Calculate();
      }
      else{
        return eq_exp->Calculate();
      }
    }
};

class EqExpAST: public BaseAST{
  public:
    unique_ptr<BaseAST> eq_exp;
    unique_ptr<BaseAST> rel_exp;
    string eq_op;

    void Dump() const override {
      cout << "EqExp { ";
      if (eq_exp) {
        eq_exp->Dump();
        cout << eq_op;
        rel_exp->Dump();
      } else {
        rel_exp->Dump();
      }
      cout << " }";
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
    int Calculate() const override {
      if(eq_exp){
        if(eq_op=="=="){
          return eq_exp->Calculate() == rel_exp->Calculate();
        }
        else{
          return eq_exp->Calculate() != rel_exp->Calculate();
        }
      }
      else{
        return rel_exp->Calculate();
      }
    }
};

class RelExpAST: public BaseAST{
  public:
    unique_ptr<BaseAST> rel_exp;
    unique_ptr<BaseAST> add_exp;
    string rel_op;

    void Dump() const override {
      cout << "RelExp { ";
      if (rel_exp) {
        rel_exp->Dump();
        cout << rel_op;
        add_exp->Dump();
      } else {
        add_exp->Dump();
      }
      cout << " }";
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
    int Calculate() const override {
      if(rel_exp){
        if(rel_op=="<"){
          return rel_exp->Calculate() < add_exp->Calculate();
        }
        else if(rel_op==">"){
          return rel_exp->Calculate() > add_exp->Calculate();
        }
        else if(rel_op=="<="){
          return rel_exp->Calculate() <= add_exp->Calculate();
        }
        else{
          return rel_exp->Calculate() >= add_exp->Calculate();
        }
      }
      else{
        return add_exp->Calculate();
      }
    }
};

// lv4 start
class DeclAST: public BaseAST{
  public:
    unique_ptr<BaseAST> const_decl;
    unique_ptr<BaseAST> var_decl;

    void Dump() const override {
      cout << "Decl { ";
      if(const_decl){
        const_decl->Dump();
        cout << ", ";
      }
      if(var_decl){
        var_decl->Dump();
        cout << ", ";
      }
      cout << " }";
    }
    void KoopaIR() const override {
      if(const_decl){
        const_decl->KoopaIR();
      }
      if(var_decl){
        var_decl->KoopaIR();
      }
    }
    int Calculate() const override {
      return 0;
    }
};

class BTypeAST: public BaseAST{
  public:
    string type;

    void Dump() const override {
      cout << "BType { ";
      cout << type;
      cout << " }";
    }
    void KoopaIR() const override {
      // cout << type;
    }
    int Calculate() const override {
      return 0;
    }
};

class ConstDeclAST: public BaseAST{
  public:
    unique_ptr<BaseAST> b_type;
    unique_ptr<vector<unique_ptr<BaseAST>>> const_def_list;

    void Dump() const override {
      cout << "ConstDecl { ";
      b_type->Dump();
      cout << ", ";
      for(auto &i:*const_def_list){
        i->Dump();
        cout << ", ";
      }
      cout << " }";
    }
    void KoopaIR() const override {
      for(auto &i:*const_def_list){
        i->KoopaIR();
      }
    }
    int Calculate() const override {
      return 0;
    }
};

class ConstDefAST: public BaseAST{
  public:
    string ident;
    unique_ptr<BaseAST> const_init_val;

    void Dump() const override {
      cout << "ConstDef { ";
      cout << ident;
      cout << ", ";
      const_init_val->Dump();
      cout << " }";
    }
    void KoopaIR() const override {
      string target_ident = ident + "_" + to_string(symbol_table_stack.size());
      symbol_table_stack.back()->emplace(target_ident, const_init_val->Calculate());
      symbol_type_stack.back()->emplace(target_ident, "const");
    }
    int Calculate() const override {
      return 0;
    }
};

class ConstInitValAST: public BaseAST{
  public:
    unique_ptr<BaseAST> const_exp;

    void Dump() const override {
      cout << "ConstInitVal { ";
      const_exp->Dump();
      cout << " }";
    }
    void KoopaIR() const override {
      // const_exp->KoopaIR();
    }
    int Calculate() const override {
      return const_exp->Calculate();
    }
};

class ConstExpAST: public BaseAST{
  public:
    unique_ptr<BaseAST> exp;

    void Dump() const override {
      cout << "ConstExp { ";
      exp->Dump();
      cout << " }";
    }
    void KoopaIR() const override {
      exp->KoopaIR();
    }
    int Calculate() const override {
      return exp->Calculate();
    }
};

class VarDeclAST: public BaseAST{
  public:
    unique_ptr<BaseAST> b_type;
    unique_ptr<vector<unique_ptr<BaseAST>>> var_def_list;

    void Dump() const override {
      cout << "VarDecl { ";
      b_type->Dump();
      cout << ", ";
      for(auto &i:*var_def_list){
        i->Dump();
        cout << ", ";
      }
      cout << " }";
    }
    void KoopaIR() const override {
      for(auto &i:*var_def_list){
        i->KoopaIR();
      }
    }
    int Calculate() const override {
      return 0;
    }
};

class VarDefAST: public BaseAST{
  public:
    string ident;
    unique_ptr<BaseAST> init_val;

    void Dump() const override {
      cout << "VarDef { ";
      cout << ident;
      cout << ", ";
      if(init_val){
        init_val->Dump();
      }
      cout << " }";
    }
    void KoopaIR() const override {
      string target_ident = ident + "_" + to_string(symbol_table_stack.size());
      cout << "  @" << target_ident << " = alloc i32" << endl;
      symbol_table_stack.back()->emplace(target_ident, 1); // 这里随便给的值，因为不会用到
      symbol_type_stack.back()->emplace(target_ident, "var");
      if(init_val) {
        init_val->KoopaIR();
        cout << "  store " << nums.back() << ", @" << target_ident << endl;
        nums.pop_back();
      }
    }
    int Calculate() const override {
      return 0;
    }
};

class InitValAST: public BaseAST{
  public:
    unique_ptr<BaseAST> exp;

    void Dump() const override {
      cout << "InitVal { ";
      exp->Dump();
      cout << " }";
    }
    void KoopaIR() const override {
      exp->KoopaIR();
    }
    int Calculate() const override {
      return exp->Calculate();
    }
};
