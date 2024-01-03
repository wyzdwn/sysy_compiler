#include <iostream>
#include "koopa.h"
#include <string>
#include <sstream>
#include <cassert>
#include "visit_koopa_raw.hpp"
#include <unordered_map>
#include <deque>

using namespace std;

// binary op: koopa --> riscv , 注意这里的LE, GE都是“反”的，比如: LE --> sgt
unordered_map<int, string> binary_op_map={
  {KOOPA_RBO_NOT_EQ, "snez"},
  {KOOPA_RBO_EQ, "seqz"},
  {KOOPA_RBO_LT, "slt"},
  {KOOPA_RBO_LE, "sgt"},
  {KOOPA_RBO_GT, "sgt"},
  {KOOPA_RBO_GE, "slt"},
  {KOOPA_RBO_ADD, "add"},
  {KOOPA_RBO_SUB, "sub"},
  {KOOPA_RBO_MUL, "mul"},
  {KOOPA_RBO_DIV, "div"},
  {KOOPA_RBO_MOD, "rem"},
  {KOOPA_RBO_AND, "and"},
  {KOOPA_RBO_OR, "or"},
};

static deque <string> nums; 
const deque<string> regs=\
{"t0", "t1", "t2", "t3", "t4", "t5", "t6", \
 "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",};
const int num_regs=regs.size();
unordered_map<string, int> reg_used; // 记录每个寄存器是否被使用过，1表示被使用过，0表示没被使用过

// 返回没被使用过的第一个寄存器
inline string get_reg()
{
  for(int i=0; i<num_regs; i++)
    if(reg_used[regs[i]] == 0)
    {
      reg_used[regs[i]] = 1;
      return regs[i];
    }
  assert(false);
}

inline void free_reg()
{
  string reg = nums.back();
  nums.pop_back();
  reg_used[reg] = 0;
}

// 给定riscv的运算符（op），以及koopaIR的两个操作数（lhs, rhs），生成对应的汇编代码
inline void binary_two_operands(koopa_raw_value_t lhs, koopa_raw_value_t rhs, string op)
{
  string target_reg;
  if(lhs->kind.tag == KOOPA_RVT_INTEGER && rhs->kind.tag == KOOPA_RVT_INTEGER){
    Visit(lhs);
    Visit(rhs);
    target_reg = get_reg();
    // 用两个操作数对应的两个寄存器中一个来存结果，这里选择最先进入nums的那个，这样只需进行一次pop_back
    cout<<"  "<<op<<" "<<target_reg<<", "<<nums[nums.size()-2]<<", "<<nums.back()<<endl;
  }
  else if(lhs->kind.tag == KOOPA_RVT_INTEGER){
    Visit(lhs);
    target_reg = get_reg();
    cout<<"  "<<op<<" "<<target_reg<<", "<<nums.back()<<", "<<nums[nums.size()-2]<<endl;
  }
  else if(rhs->kind.tag == KOOPA_RVT_INTEGER){
    Visit(rhs);
    target_reg = get_reg();
    cout<<"  "<<op<<" "<<target_reg<<", "<<nums[nums.size()-2]<<", "<<nums.back()<<endl;
  }
  else{
    target_reg = get_reg();
    cout<<"  "<<op<<" "<<target_reg<<", "<<nums[nums.size()-2]<<", "<<nums.back()<<endl;
  }
  free_reg();
  free_reg();
  nums.push_back(target_reg);
}


// 访问 raw program
void Visit(const koopa_raw_program_t &program) {
  // 执行一些其他的必要操作
  // ...
  for(int i=0; i<num_regs; i++) reg_used[regs[i]] = 0;
  // 访问所有全局变量
  cout<<"  .text"<<endl;
  Visit(program.values);
  // 访问所有函数
  Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        // 访问函数
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        // 访问指令
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        // 我们暂时不会遇到其他内容, 于是不对其做任何处理
        assert(false);
    }
  }
}

// 访问函数
void Visit(const koopa_raw_function_t &func) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有基本块
  cout << "  .globl " << func->name+1 << endl;
  cout << func->name+1 << ":" << endl;
  Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
    switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit(kind.data.integer);
      break;
    case KOOPA_RVT_BINARY:
      Visit(kind.data.binary);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
}

void Visit(const koopa_raw_return_t &ret) {
  // 执行一些其他的必要操作
  // ...
  
  // 访问返回值
  if(ret.value->kind.tag == KOOPA_RVT_INTEGER) Visit(ret.value);
  cout<<"  mv a0, "<<nums.back()<<endl;
  free_reg();
  cout<<"  ret";
}

void Visit(const koopa_raw_integer_t &integer) {
  // 执行一些其他的必要操作
  // ...
  // 访问整数值
  if(integer.value == 0)
  {
    nums.push_back("x0");
    return;
  }
  string target_reg = get_reg();
  cout<<"  li "<<target_reg<<", "<<integer.value<<endl;
  nums.push_back(target_reg);
}

void Visit(const koopa_raw_binary_t &binary) {
  // 执行一些其他的必要操作
  // ...
  // 访问二元运算符
  // 把两个操作符的值放到寄存器里，再运算，换句话说，不会出现I类指令
  if(binary.op == KOOPA_RBO_NOT_EQ || binary.op == KOOPA_RBO_EQ)
  {
    binary_two_operands(binary.lhs, binary.rhs, "xor");
    cout<<"  "<<binary_op_map[binary.op]<<" "<<nums.back()<<", "<<nums.back()<<endl;
  }
  else if(binary.op == KOOPA_RBO_LE || binary.op == KOOPA_RBO_GE)
  {
    binary_two_operands(binary.lhs, binary.rhs, binary_op_map[binary.op]);
    cout<<"  seqz "<<nums.back()<<", "<<nums.back()<<endl;
  }
  else
    binary_two_operands(binary.lhs, binary.rhs, binary_op_map[binary.op]);
}