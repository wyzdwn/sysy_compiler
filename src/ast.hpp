#pragma once

#include <iostream>
#include <memory>
#include <stack>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

using namespace std;

static int current_id = 0;
static int block_id = 0;
static string  block_name = "";
static int if_id = 0;
static int or_id = 0;
static int and_id = 0;
static int while_id = 0;
static int now_while = 0;
static deque<string> nums;
static deque<unordered_map<string, int>*> symbol_table_stack;
static deque<unordered_map<string, string>*> symbol_type_stack;
static deque<string> block_stack;

static int fun_ret_flag=0;

#define FUNCTYPE 22 // 函数类型

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
    string target_ident = block_stack[i] + ident ;
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

inline void enter_block()
{
  unordered_map<string, int> *symbol_table=new unordered_map<string, int>;
  unordered_map<string, string> *symbol_type=new unordered_map<string, string>;
  symbol_table_stack.push_back(symbol_table);
  symbol_type_stack.push_back(symbol_type);
  if(block_name!="") 
  {
    block_stack.push_back(block_name);
    block_name="";
  }
  else block_stack.push_back("Block_"+to_string(block_id++)+"_");
}
inline void exit_block()
{
  free(symbol_table_stack.back());
  symbol_table_stack.pop_back();
  free(symbol_type_stack.back());
  symbol_type_stack.pop_back();
  block_stack.pop_back();
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
  unique_ptr<vector<unique_ptr<BaseAST>>> comp_unit_item_list;

  void Dump() const override {
    return;
  }
  void KoopaIR() const override {
    enter_block();

    // 声明库函数
    cout << "decl @getint(): i32\n" \
                "decl @getch(): i32\n" \
                "decl @getarray(*i32): i32\n" \
                "decl @putint(i32)\n" \
                "decl @putch(i32)\n" \
                "decl @putarray(i32, *i32)\n" \
                "decl @starttime()\n" \
                "decl @stoptime()\n" << endl;
    symbol_table_stack[0]->emplace("getint", FUNCTYPE);
    symbol_type_stack[0]->emplace("getint", "Func_int");
    symbol_table_stack[0]->emplace("getch", FUNCTYPE);
    symbol_type_stack[0]->emplace("getch", "Func_int");
    symbol_table_stack[0]->emplace("getarray", FUNCTYPE);
    symbol_type_stack[0]->emplace("getarray", "Func_int");
    symbol_table_stack[0]->emplace("putint", FUNCTYPE);
    symbol_type_stack[0]->emplace("putint", "Func_void");
    symbol_table_stack[0]->emplace("putch", FUNCTYPE);
    symbol_type_stack[0]->emplace("putch", "Func_void");
    symbol_table_stack[0]->emplace("putarray", FUNCTYPE);
    symbol_type_stack[0]->emplace("putarray", "Func_void");
    symbol_table_stack[0]->emplace("starttime", FUNCTYPE);
    symbol_type_stack[0]->emplace("starttime", "Func_void");
    symbol_table_stack[0]->emplace("stoptime", FUNCTYPE);
    symbol_type_stack[0]->emplace("stoptime", "Func_void");

    for(auto &i:*comp_unit_item_list){
      i->KoopaIR();
      cout<<endl;
      current_id=0;
      nums.clear();
    }
    exit_block();
  }
  int Calculate() const override {
    return 0;
  }
};

class CompUnitItemAST : public BaseAST {
 public:
  unique_ptr<BaseAST> func_def;
  unique_ptr<BaseAST> decl;

  void Dump() const override {
    return;
  }
  void KoopaIR() const override {
    if(func_def){
      func_def->KoopaIR();
    }
    else if(decl){
      decl->KoopaIR();
    }
  }
  int Calculate() const override {
    return 0;
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

class FuncFParamAST : public BaseAST {
 public:
  string b_type;
  string ident;
  unique_ptr<vector<unique_ptr<BaseAST>>> const_index_list;

  void Dump() const override {
    return;
  }
  void KoopaIR() const override {
    if(const_index_list){
      cout << "@" << ident << ": *";
      for (int i = 0; i < const_index_list->size(); i++) cout<<"[";
      cout<<"i32";
      if(const_index_list->size()) cout<<", ";
      for (int i = const_index_list->size() - 1; i >= 0; i--) {
        const auto& const_exp = (*const_index_list)[i];
        int num = dynamic_cast<ConstExpAST*>(const_exp.get())->Calculate();
        if(i) cout<<std::to_string(num) + "], ";
        else cout<<std::to_string(num)+"]";
      }
    } else cout << "@" << ident << ": i32";
  }
  int Calculate() const override {
    return 0;
  }
  void Alloc() const {
    string target_ident = block_stack.back() + ident ;
    if(const_index_list){
      cout<<"  @"<<target_ident<<" = alloc *";
      for (int i = 0; i < const_index_list->size(); i++) cout<<"[";
      cout<<"i32";
      if(const_index_list->size()) cout<<", ";
      for (int i = const_index_list->size() - 1; i >= 0; i--) {
        const auto& const_exp = (*const_index_list)[i];
        int num = dynamic_cast<ConstExpAST*>(const_exp.get())->Calculate();
        if(i) cout<<std::to_string(num) + "], ";
        else cout<<std::to_string(num)+"]";
      }
      cout<<endl;
      // 这里符号表里存放的是数组有几个维度，如 arr*[2][3] -> 3，以在Stmt和Lval中部分解引用数组
      // 注意这里是*，即数组指针，所以要加1
      symbol_table_stack.back()->emplace(target_ident, const_index_list->size()+1); 
      symbol_type_stack.back()->emplace(target_ident, "ptr"); // 指针类型
    } else{
      cout << "  @" << target_ident << " = alloc i32" << endl;
      symbol_table_stack.back()->emplace(target_ident, 1); // 这里随便给的值，因为不会用到
      symbol_type_stack.back()->emplace(target_ident, "var");
    }
    cout<<"  store @"<<ident<<", @"<<target_ident<<endl;
  }
};

class FuncTypeAST : public BaseAST {
 public:
  string type;

  void Dump() const override {
    cout << "FuncDefAST { \"int\" }";
  }
  void KoopaIR() const override {
    if(type=="int") cout<<": i32";
    else cout<<" ";
  }
  int Calculate() const override {
    return 0;
  }
};

class FuncDefAST : public BaseAST {
 public:
  string func_type;
  string ident;
  unique_ptr<BaseAST> block;
  unique_ptr<vector<unique_ptr<BaseAST>>> func_f_param_list;

  void Dump() const override {
    return ;
  }
  void KoopaIR() const override {
    const string& type = func_type;
    symbol_table_stack[0]->emplace(ident, FUNCTYPE);
    symbol_type_stack[0]->emplace(ident, "Func_"+type);
    block_name="FUNC_"+ident+"_";
    enter_block();
    
    cout<< "fun @"<<ident<<"(";
    int i=0, sz=func_f_param_list->size();
    for(auto &param:*func_f_param_list){
      param->KoopaIR();
      if(i!=sz-1) cout<<", ";
      i++;
    }
    cout<<")";
    if(type=="int") cout<<": i32";
    else cout<<" ";
    cout << " {\n";
    cout << "%entry:\n";
    fun_ret_flag=0;

    for(auto& func_f_param: *func_f_param_list) 
      dynamic_cast<FuncFParamAST*>(func_f_param.get())->Alloc();
    
    block->KoopaIR();
    if(fun_ret_flag==0)
    {
      if(type=="void") cout<<"  ret"<<endl;
      else cout<<"  ret 0"<<endl;
    }
    cout << "}\n";
    exit_block();
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

class LValAST: public BaseAST{
  public:
    string ident;
    unique_ptr<vector<unique_ptr<BaseAST>>> index_list;

    void Dump() const override {
      cout << "LVal { ";
      cout << ident;
      cout << " }";
    }
    void KoopaIR() const override {
      string target_ident = get_target_ident(ident)[0];
      string target_type = get_target_ident(ident)[1];
      string target_value = get_target_ident(ident)[2];
      if(target_type=="array"||target_type=="const array"){
        // 数组
        for (int i = 0; i < index_list->size(); i++) {
          const auto& exp_index = (*index_list)[i];
          dynamic_cast<ExpAST*>(exp_index.get())->KoopaIR();
          cout << "  %" << current_id << " = getelemptr ";
          if(i == 0) cout << "@" << get_target_ident(ident)[0];
          else cout <<nums[nums.size()-2]; // 上一个 getelemptr 的结果
          cout << ", " << nums.back() << endl;
          nums.pop_back();
          if(i!=0) nums.pop_back();
          nums.push_back("%"+to_string(current_id));
          current_id++;
        }
        if(index_list->size()==0)
        {
          cout << "  %" << current_id << " = getelemptr @" << target_ident << ", 0" << endl;
          nums.push_back("%"+to_string(current_id));
          current_id++;
          return ;
        }
        if(stoi(target_value)!=index_list->size())
          cout << "  %" << current_id << " = getelemptr " << nums.back() << ", 0" << endl;
        else
          cout << "  %" << current_id << " = load " << nums.back() << endl;
        nums.pop_back();
        nums.push_back("%"+to_string(current_id));
        current_id++;
      } else if(target_type=="var"||target_type=="const"){
        // 变量
        if(target_ident=="") throw("undefined variable: " + ident);
        if(target_type=="const") nums.push_back(target_value);
        else{
          cout << "  %"<< current_id << " = load @" << target_ident << endl;
          nums.push_back("%"+to_string(current_id));
          current_id++;
        }
      } else if(target_type=="ptr"){
        // 指针
        cout << "  %"<< current_id << " = load @" << target_ident << endl;
        current_id++;
        for (int i = 0; i<index_list->size(); i++) {
          int lastptr_current_id = current_id-1;
          const auto& exp_index = (*index_list)[i];
          dynamic_cast<ExpAST*>(exp_index.get())->KoopaIR();
          if(i==0) std::cout << "  %" << current_id << " = getptr %";
          else std::cout << "  %" << current_id << " = getelemptr %";
          std::cout << lastptr_current_id << ", " << nums.back() << std::endl;
          nums.pop_back();
          current_id++;
        }
        if(index_list->size()==0)
        {
          cout << "  %" << current_id << " = getptr %" << current_id-1 << ", 0" << endl;
          nums.push_back("%"+to_string(current_id));
          current_id++;
          return ;
        }
        if(stoi(target_value)!=index_list->size())
          std::cout << "  %" << current_id << " = getelemptr %" << current_id-1 << ", 0" << std::endl;
        else
          std::cout << "  %" << current_id << " = load %" << current_id-1 << std::endl;
        nums.push_back("%"+to_string(current_id));
        current_id++;
      } else throw("In LVal undefined variable type: " + target_type);
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
    if(!block_item_list) return;
    enter_block();
    
    for(auto &i:*block_item_list){
      if(fun_ret_flag) break;
      i->KoopaIR();
    }
    exit_block();
    
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

class IfStmtAST: public BaseAST{
  public:
    unique_ptr<BaseAST> if_stmt;

    void Dump() const override {
      cout << "IfStmt { ";
      if_stmt->Dump();
      cout << " }";
    }
    void KoopaIR() const override {
      if_stmt->KoopaIR();
    }
    int Calculate() const override {
      return 0;
    }
};

class OnlyIfAST: public BaseAST{
  public:
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> stmt;

    void Dump() const override {
      cout << "OnlyIf { ";
      exp->Dump();
      cout << ", ";
      stmt->Dump();
      cout << " }";
    }
    void KoopaIR() const override {
      if(fun_ret_flag) return;
      int now_if = if_id++;
      exp->KoopaIR();
      cout << "  br " << nums.back() << ", %If_" << now_if << ", %IfEnd_" << now_if << endl; 
      nums.pop_back();

      cout << "%If_" << now_if << ":" << endl;
      fun_ret_flag=0;
      block_name="If_" + to_string(now_if) + "_";
      stmt->KoopaIR();
      if(!fun_ret_flag) cout << "  jump %IfEnd_" << now_if << endl;

      cout << "%IfEnd_" << now_if << ":" <<endl;
      fun_ret_flag=0;
    }
    int Calculate() const override {
      return 0;
    }
};

class IfElseAST: public BaseAST{
  public:
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> if_stmt;
    unique_ptr<BaseAST> else_stmt;

    void Dump() const override {
      cout << "IfElse { ";
      exp->Dump();
      cout << ", ";
      if_stmt->Dump();
      cout << ", ";
      else_stmt->Dump();
      cout << " }";
    }
    void KoopaIR() const override {
      if(fun_ret_flag) return;
      int now_if=if_id++;
      exp->KoopaIR();
      cout << "  br " << nums.back() << ", %If_" << now_if << ", %Else_" << now_if << endl; 
      nums.pop_back();

      cout << "%If_" << now_if << ":" << endl;
      fun_ret_flag=0;
      block_name="If_" + to_string(now_if) + "_";
      if_stmt->KoopaIR();
      if(!fun_ret_flag) cout << "  jump %IfEnd_" << now_if << endl;

      cout << "%Else_" << now_if << ":" <<endl;
      fun_ret_flag=0;
      block_name="Else_" + to_string(now_if) + "_";
      else_stmt->KoopaIR();
      if(!fun_ret_flag) cout << "  jump %IfEnd_" << now_if << endl;

      cout << "%IfEnd_" << now_if << ":" <<endl;
      fun_ret_flag=0;
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
  unique_ptr<BaseAST> if_stmt;
  unique_ptr<BaseAST> while_stmt;
  bool break_;
  bool continue_;
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
    } else if(if_stmt){
      if_stmt->Dump();
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
      string exp_save = nums.back();
      nums.pop_back();
      auto lval_ptr = dynamic_cast<LValAST*>(lval.get());
      string ident = lval_ptr->ident;
      string target_ident = get_target_ident(ident)[0];
      string target_type = get_target_ident(ident)[1];
      if (target_type=="array" || target_type=="const array") {
        // LVal为数组
        for (int i = 0; i < lval_ptr->index_list->size(); i++) {
          const auto& exp_index = (*(lval_ptr->index_list))[i];
          dynamic_cast<ExpAST*>(exp_index.get())->KoopaIR();
          cout << "  %" << current_id << " = getelemptr ";
          if(i == 0)cout << "@" << get_target_ident(lval_ptr->ident)[0];
          else cout << nums[nums.size()-2]; // 上一个 getelemptr 的结果
          cout << ", " << nums.back() << endl;
          nums.pop_back();
          if(i!=0) nums.pop_back();
          nums.push_back("%"+to_string(current_id));
          current_id++;
        }
        cout << "  store " << exp_save << ", " << nums.back() << endl;
        nums.pop_back();
      } else if(target_type=="var" || target_type=="const"){
        // LVal为变量
        cout << "  store " << exp_save << ", @";
        if(target_ident=="") throw("undefined variable: " + ident);
        cout << target_ident << endl;
      } else if (target_type=="ptr"){
        std::cout << "  %" << current_id << " = load @" << target_ident << std::endl;
        current_id++;
        for (int i = 0; i<lval_ptr->index_list->size(); i++) {
          int lastptr_current_id = current_id-1;
          const auto& exp_index = (*(lval_ptr->index_list))[i];
          dynamic_cast<ExpAST*>(exp_index.get())->KoopaIR();
          if(i==0)
            std::cout << "  %" << current_id << " = getptr %";
          else
            std::cout << "  %" << current_id << " = getelemptr %";
          std::cout << lastptr_current_id << ", " << nums.back() << std::endl;
          nums.pop_back();
          current_id++;
        }
        std::cout << "  store " << exp_save << ", %" << current_id-1 << std::endl;
      } else throw("In Stmt undefined variable type: " + target_type);
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
    } else if(if_stmt){
      if_stmt->KoopaIR();
    } else if(while_stmt){
      if(fun_ret_flag) return;
      int save_while=now_while;
      now_while=while_id++;
      cout<<"  jump %While_"<<now_while<<endl;
      cout<<"%While_"<<now_while<<":"<<endl;
      fun_ret_flag=0;
      block_name="While_" + to_string(now_while) + "_";
      exp->KoopaIR();
      cout << "  br " << nums.back() << ", %WhileBody_" << now_while << ", %WhileEnd_" << now_while << endl; 
      nums.pop_back();

      cout << "%WhileBody_" << now_while << ":" << endl;
      fun_ret_flag=0;
      block_name="WhileBody_" + to_string(now_while) + "_";
      while_stmt->KoopaIR();
      if(!fun_ret_flag) cout << "  jump %While_" << now_while << endl;

      cout << "%WhileEnd_" << now_while << ":" <<endl;
      now_while=save_while;
      fun_ret_flag=0;
    } else if(break_){
      // if(fun_ret_flag) return;
      cout << "  jump %WhileEnd_" << now_while << endl;
      fun_ret_flag=1;
    } else if(continue_){
      // if(fun_ret_flag) return;
      cout << "  jump %While_" << now_while << endl;
      fun_ret_flag=1;
    }
    
    
  }
  int Calculate() const override {
    return 0;
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
  string ident;
  unique_ptr<vector<unique_ptr<BaseAST>>> func_r_param_list;

  void Dump() const override {
    return;
  }
  void KoopaIR() const override {
    if (primary_exp) {
      primary_exp->KoopaIR();
    } else if(unary_exp) {
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
    } else if(ident!=""){
      int cnt=0, sz=func_r_param_list->size();
      for(auto &param:*func_r_param_list){
        param->KoopaIR();
        cnt++;
      }

      if(symbol_type_stack[0]->at(ident)=="Func_int")
        cout << "  %"<< current_id << " = call @" << ident << "(";
      else
        cout << "  call @" << ident << "(";
      
      for(int i=sz-1;i>=0;i--)
      {
        cout << nums[nums.size()-1-i];
        if(i!=0) cout<<", ";
      }
      for(int i=sz-1;i>=0;i--) nums.pop_back();
      cout<<")"<<endl;

      if(symbol_type_stack[0]->at(ident)=="Func_int")
      {
        nums.push_back("%"+to_string(current_id));
        current_id++;
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
        int now_or=or_id++;
        cout << "  @" << "Or_" << now_or << " = alloc i32" << endl;
        lor_exp->KoopaIR();
        // 如果lor_exp为真，那么land_exp就不用计算了，设置标签跳过land_exp
        cout << "  br " << nums.back() << ", %OrSkip_" << now_or << ", %OrBody_" << now_or << endl;
        
        cout << "%OrBody_" << now_or << ":" << endl;
        fun_ret_flag=0;
        block_name="Or_Body" + to_string(now_or) + "_";
        land_exp->KoopaIR();
        KoopaIR_logic_operands("or");
        cout << "  store " << nums.back() << ", @Or_" << now_or << endl;
        nums.pop_back();
        if(!fun_ret_flag) cout << "  jump %OrEnd_" << now_or << endl;

        cout << "%OrSkip_" << now_or << ":" << endl;
        fun_ret_flag=0;
        cout << "  store 1, @Or_" << now_or << endl;
        if(!fun_ret_flag) cout << "  jump %OrEnd_" << now_or << endl;

        cout << "%OrEnd_" << now_or << ":" << endl;
        fun_ret_flag=0;
        cout << "  %"<< current_id++ << " = load @Or_" << now_or << endl;
        nums.push_back("%"+to_string(current_id-1));
      } else {
        land_exp->KoopaIR();
      }
    }
    int Calculate() const override {
      if(lor_exp){
        int lor_exp_value = lor_exp->Calculate();
        if(lor_exp_value) return 1;
        else return !!land_exp->Calculate();
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
        int now_and = and_id++;
        cout << "  @" << "And_" << now_and << " = alloc i32" << endl;
        land_exp->KoopaIR();
        cout << "  %"<< current_id++ << " = ne 0, " << nums.back() << endl;
        nums.pop_back();
        nums.push_back("%"+to_string(current_id-1));
        // 如果land_exp为假，那么eq_exp就不用计算了，设置标签跳过eq_exp
        cout << "  br " << nums.back() << ", %AndBody_" << now_and << ", %AndSkip_" << now_and << endl;

        cout << "%AndBody_" << now_and << ":" << endl;
        fun_ret_flag=0;
        block_name="And_Body" + to_string(now_and) + "_";
        eq_exp->KoopaIR();
        cout << "  %"<< current_id++ << " = ne 0, " << nums.back() << endl;
        nums.pop_back();
        nums.push_back("%"+to_string(current_id-1));
        KoopaIR_logic_operands("and");
        cout << "  store " << nums.back() << ", @And_" << now_and << endl;
        nums.pop_back();
        if(!fun_ret_flag) cout << "  jump %AndEnd_" << now_and << endl;

        cout << "%AndSkip_" << now_and << ":" << endl;
        fun_ret_flag=0;
        cout << "  store 0, @And_" << now_and << endl;
        if(!fun_ret_flag) cout << "  jump %AndEnd_" << now_and << endl;

        cout << "%AndEnd_" << now_and << ":" << endl;
        fun_ret_flag=0;
        cout << "  %"<< current_id++ << " = load @And_" << now_and << endl;
        nums.push_back("%"+to_string(current_id-1));
      } else {
        eq_exp->KoopaIR();
      }
    }
    int Calculate() const override {
      if(land_exp){
        int land_exp_value = land_exp->Calculate();
        if(!land_exp_value) return 0;
        else return !!eq_exp->Calculate();
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
    string b_type;
    unique_ptr<vector<unique_ptr<BaseAST>>> const_def_list;

    void Dump() const override {
      return;
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

// 递归打印‘数组初始化的表达式’
// params:
// ident: 数组的名称
// array_init_agg: 数组的初始化值，会在各个涉及数组初始化的地方初始化好，并填充上0
// len: 因为是递归打印，这里代表当前所在的维度的长度
// mul_len: 因为是递归打印，这里代表当前维度之后的所有维度的长度的乘积
// depth: 当前所在的维度
// idx: 当前所在的维度的起始下标，相当于把多维数组展开成一维数组，idx就是这个一维数组的下标
// format: 打印的格式，A(aggregate): 打印{x1,x2,x3,...}，S(store): 获取数组元素的指针，然后用store指令把值存进去
static void print_array_init(const string& ident, 
                            vector<int>* array_init_agg, 
                            deque<int>* len, 
                            deque<int>* mul_len, 
                            int depth, 
                            int idx, 
                            char format) {
  if (format == 'A') {
    if(depth == len->size())
      cout << (*array_init_agg)[idx];
    else {
      cout << "{";
      int size = (*mul_len)[depth] / (*len)[depth];
      for (int i=0; i < (*len)[depth] ;i++) {
        print_array_init(ident, array_init_agg, len, mul_len, depth+1, idx + i*size, format);
        if(i != (*len)[depth]-1)
          cout << ", ";
      }
      cout << "}";
    }
  } else if (format == 'S'){
    if(depth == len->size()) {
      // 一维数组，不需要递归
      cout << "  store " << (*array_init_agg)[idx] << ", %" << current_id-1 << endl;
    } else {
      // 多维数组，需要递归，且其中其实有“跳维”的操作，具体看下面注释
      // 举例: a=int[2][3][4]
      // 则mul_len = {4*3*2, 4*3, 4}, len = {2, 3, 4}
      // step = 4*3*2/2 = 12，步长，即打印一个元素需要跳过多少个下标
      // 我们会先从最低维开始打印，即从a[0][0][0]开始打印，打印完一维后，再打印下一维
      // 所以会有“跳维”的操作，即打印第一轮打印的其实是a中的第0，12，24，36个元素，所以需要计算步长step
      int step = (*mul_len)[depth] / (*len)[depth];
      int parent_id = current_id-1;
      // 这里不使用nums，nums的push和pop的逻辑在这里有些复杂，且nums在此处可以不用
      for (int i=0; i < (*len)[depth] ;i++) {
        cout << "  %" << current_id << " = getelemptr ";
        if(depth == 0) cout << "@" << get_target_ident(ident)[0];
        else cout <<"%"<<parent_id;
        cout << ", " << i << endl;
        current_id++;
        print_array_init(ident, array_init_agg, len, mul_len, depth+1, idx + i*step, format);
      }
    }
  }
}


class ConstInitValAST: public BaseAST{
  public:
    unique_ptr<BaseAST> const_exp;
    unique_ptr<vector<unique_ptr<BaseAST>>> const_array_init_val;

    void Dump() const override {
      cout << "ConstInitVal { ";
      const_exp->Dump();
      cout << " }";
    }
    void KoopaIR() const override {
      return;
    }
    int Calculate() const override {
      return const_exp->Calculate();
    }
    // 递归聚合数组初始化的值，返回一个vector<int>，里面存放的是聚合后的数组初始化值
    // 即使是多维数组，也可以展开成一维数组
    vector<int> Aggregate(deque<int>::iterator len_begin,deque<int>::iterator len_end) const {
      vector<int> array_init_agg;
      for(auto& const_init_val : *const_array_init_val) {
        auto child = dynamic_cast<ConstInitValAST*>(const_init_val.get());
        if (!child->const_array_init_val) array_init_agg.push_back(child->Calculate());
        else{
          auto it = len_begin;
          ++it;
          for (; it !=  len_end; ++it) {
            if (array_init_agg.size() % (*it) == 0) {
              auto child_agg = child->Aggregate(it, len_end);
              array_init_agg.insert(array_init_agg.end(), child_agg.begin(), child_agg.end());
              break;
            }
          }
        }
      }
      array_init_agg.insert(array_init_agg.end(), (*len_begin) - array_init_agg.size(), 0);
      return array_init_agg;
    }
};

class ConstDefAST: public BaseAST{
  public:
    string ident;
    unique_ptr<BaseAST> const_init_val;
    unique_ptr<vector<unique_ptr<BaseAST>>> const_index_list;

    void Dump() const override {
      cout << "ConstDef { ";
      cout << ident;
      cout << ", ";
      const_init_val->Dump();
      cout << " }";
    }
    void KoopaIR() const override {
      string target_ident = block_stack.back() + ident ;
      if(const_index_list->size())
      {
        // 数组
        // 这里符号表里存放的是数组有几个维度，如 arr[2][3][4] -> 3，以在Stmt和Lval中部分解引用数组
        symbol_table_stack.back()->emplace(target_ident, const_index_list->size());
        symbol_type_stack.back()->emplace(target_ident, "const array");
        if(symbol_table_stack.size()==1) cout<<"global "<<"@"<<target_ident<<" = alloc ";
        else cout << "  @" << target_ident << " = alloc ";
        for (int i = 0; i < const_index_list->size(); i++) cout << "[";
        cout << "i32";
        if(const_index_list->size()) cout << ", ";

        // arr[2][3][4] -> len = {2, 3, 4}, mul_len = {4*3*2, 4*3, 4}
        auto mul_len = new deque<int>();
        auto len = new deque<int>();
        for (int i = const_index_list->size() - 1; i >= 0; i--){
          const auto& const_exp = (*const_index_list)[i];
          int tmp = dynamic_cast<ConstExpAST*>(const_exp.get())->Calculate();
          len->push_front(tmp);
          if(mul_len->empty()) mul_len->push_front(tmp);
          else mul_len->push_front(mul_len->front() * tmp);
          if(i!=0) cout << tmp << "], ";
          else cout << tmp << "]";
        }

        vector<int> array_init_agg = dynamic_cast<ConstInitValAST*>
          (const_init_val.get())->Aggregate(mul_len->begin(), mul_len->end());
        if (symbol_table_stack.size() == 1) {
          // 全局用aggregate初始化
          cout << ", ";
          print_array_init(ident, &array_init_agg, len, mul_len, 0, 0, 'A');
          cout << endl;
        } else{
          // 局部用store指令初始化，方便目标代码生成
          cout << endl;
          print_array_init(ident, &array_init_agg, len, mul_len, 0, 0, 'S');
        }
        delete mul_len;
        delete len;
      }
      else{
        // 常量
        symbol_table_stack.back()->emplace(target_ident, const_init_val->Calculate());
        symbol_type_stack.back()->emplace(target_ident, "const");
      }
    }
    int Calculate() const override {
      return 0;
    }
};

class VarDeclAST: public BaseAST{
  public:
    string b_type;
    unique_ptr<vector<unique_ptr<BaseAST>>> var_def_list;

    void Dump() const override {
      return ;
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

class InitValAST: public BaseAST{
  public:
    unique_ptr<BaseAST> exp;
    unique_ptr<vector<unique_ptr<BaseAST>>> array_init_val;

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
    vector<int> Aggregate(deque<int>::iterator mul_len_begin,deque<int>::iterator mul_len_end) const {
      if(symbol_table_stack.size()==1) {
        // 全局数组变量的初始化列表中只能出现常量表达式, 返回的 array_init_agg 即为各项的值
        vector<int> array_init_agg;
        for(auto& init_val : *array_init_val) {
          auto child = dynamic_cast<InitValAST*>(init_val.get());
          if (!child->array_init_val) {
            array_init_agg.push_back(child->Calculate());
          } else{
            auto it = mul_len_begin;
            ++it;
            for (; it !=  mul_len_end; ++it) {
              if (array_init_agg.size() % (*it) == 0) {
                auto child_agg = child->Aggregate(it, mul_len_end);
                array_init_agg.insert(array_init_agg.end(), child_agg.begin(), child_agg.end());
                break;
              }
            }
          }
        }
        array_init_agg.insert(array_init_agg.end(), (*mul_len_begin) - array_init_agg.size(), 0);
        return array_init_agg;
      }
      else {
        // 局部数组变量的初始化列表中可以出现任何表达式, 返回的 array_init_agg 为各项的 KoopaIR Symbol 编号
        // 若编号为 -1, 则对应的值为 0
        vector<int> array_init_agg;
        for(auto& init_val : *array_init_val) {
          auto child = dynamic_cast<InitValAST*>(init_val.get());
          if (!child->array_init_val) {
            child->KoopaIR();
            array_init_agg.push_back(stoi(nums.back()));
            nums.pop_back();
          } else{
            auto it = mul_len_begin;
            ++it;
            for (; it !=  mul_len_end; ++it) {
              if (array_init_agg.size() % (*it) == 0) {
                auto child_agg = child->Aggregate(it, mul_len_end);
                array_init_agg.insert(array_init_agg.end(), child_agg.begin(), child_agg.end());
                break;
              }
            }
          }
        }
        // cout<<"array_init_agg.size()="<<array_init_agg.size()<<endl;
        array_init_agg.insert(array_init_agg.end(), (*mul_len_begin) - array_init_agg.size(), 0);
        return array_init_agg;
    }
}
};

class VarDefAST: public BaseAST{
  public:
    string ident;
    unique_ptr<BaseAST> init_val;
    unique_ptr<vector<unique_ptr<BaseAST>>> const_index_list;

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
      if(const_index_list->size()){
        // 数组
        string target_ident = block_stack.back() + ident ;
        // 这里符号表里存放的是数组有几个维度，如 arr[2][3][4] -> 3，以在Stmt和Lval中部分解引用数组
        symbol_table_stack.back()->emplace(target_ident, const_index_list->size());
        symbol_type_stack.back()->emplace(target_ident, "array");
        if(symbol_table_stack.size()==1) cout<<"global "<<"@"<<target_ident<<" = alloc ";
        else cout << "  @" << target_ident << " = alloc ";
        for (int i = 0; i < const_index_list->size(); i++) cout << "[";
        cout << "i32";
        if(const_index_list->size()) cout << ", ";

        auto mul_len = new deque<int>();
        auto len = new deque<int>();
        for (int i = const_index_list->size() - 1; i >= 0; i--) {
          const auto& const_exp = (*const_index_list)[i];
          int tmp = dynamic_cast<ConstExpAST*>(const_exp.get())->Calculate();
          len->push_front(tmp);
          if(mul_len->empty()) mul_len->push_front(tmp);
          else mul_len->push_front(mul_len->front() * tmp);
          if(i!=0) cout << tmp << "], ";
          else cout << tmp << "]";
        }

        if(symbol_table_stack.size()==1){
          // 全局
          if(init_val){
            vector<int> array_init_agg = 
              dynamic_cast<InitValAST*>(init_val.get())->Aggregate(mul_len->begin(), mul_len->end());
            cout << ", ";
            print_array_init(ident, &array_init_agg, len, mul_len, 0, 0, 'A');
            cout << endl;
          }
          else cout<<", zeroinit"<<endl;
        } else{
          // 局部
          cout<<endl;
          if(init_val) {
            vector<int> array_init_agg = 
              dynamic_cast<InitValAST*>(init_val.get())->Aggregate(mul_len->begin(), mul_len->end());
            print_array_init(ident, &array_init_agg, len, mul_len, 0, 0, 'S');
          };
          // 如果没有init_val，局部数组先不进行处理，不打印zeroinit，这是为了之后方便生成目标代码
        }
        delete mul_len;
        delete len;
      } else{
        // 变量
        string target_ident = block_stack.back() + ident ;
        if(symbol_table_stack.size()==1) cout<<"global "<<"@"<<target_ident<<" = alloc i32, ";
        else cout << "  @" << target_ident << " = alloc i32";
        symbol_table_stack.back()->emplace(target_ident, 1); // 这里随便给的值，因为不会用到
        symbol_type_stack.back()->emplace(target_ident, "var");
        if(symbol_table_stack.size()==1){
          if(init_val) cout << init_val->Calculate() << endl;
          else cout<<"zeroinit"<<endl;
        }
        else{
          if(init_val) {
          init_val->KoopaIR();
          cout << "  store " << nums.back() << ", @" << target_ident << endl;
          nums.pop_back();
          } else cout<<endl;
        }
      }
    }
    int Calculate() const override {
      return 0;
    }
};

