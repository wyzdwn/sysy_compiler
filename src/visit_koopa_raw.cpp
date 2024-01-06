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

/********************************lv4 start**********************************/

static unordered_map<koopa_raw_value_t, string> loc; // 有返回值的语句在栈中的位置
static int stack_frame_length = 0; // 栈帧长度
static int stack_frame_used = 0; // 已经使用的栈帧长度

/*********************************lv4 end***********************************/


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
inline void binary_two_operands(koopa_raw_value_t lhs, koopa_raw_value_t rhs, string op, const koopa_raw_value_t &value)
{
  
  Visit(lhs);
  Visit(rhs);
  string target_reg = get_reg();
  cout<<"  "<<op<<" "<<target_reg<<", "<<nums[nums.size()-2]<<", "<<nums.back()<<endl;
  nums.push_back(target_reg);
  free_reg();
  free_reg();

  loc[value] = to_string(stack_frame_used) + "(sp)";
  stack_frame_used += 4;
  cout<<"  sw "<<target_reg<<", "<<loc[value]<<endl;
  free_reg();
}



/***********************************main************************************/
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
  // 清空
  stack_frame_length = 0;
  stack_frame_used = 0;

  // 计算栈帧长度
  int var_cnt = 0;

  // 遍历基本块
  for (size_t i = 0; i < func->bbs.len; ++i)
  {
    const auto& insts = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i])->insts;
    var_cnt += insts.len;
    for (size_t j = 0; j < insts.len; ++j)
    {
      auto inst = reinterpret_cast<koopa_raw_value_t>(insts.buffer[j]);
      if(inst->ty->tag == KOOPA_RTT_UNIT)
        var_cnt--;
    }
  }
  // cout<<"++++"<<var_cnt<<endl;
  stack_frame_length = var_cnt * 4;
  // cout<<"++++"<<stack_frame_length<<endl;
  // 将栈帧长度对齐到 16
  stack_frame_length = (stack_frame_length + 15) & (~15);
  // cout<<"++++"<<stack_frame_length<<endl;

  if (stack_frame_length > 0 && stack_frame_length < 2048)
    cout << "  addi sp, sp, -" << stack_frame_length << endl;
  else if (stack_frame_length >= 2048)
    cout << "  li t0, -" << stack_frame_length << endl
              << "  add sp, sp, t0" << endl;
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
  if(loc.find(value) != loc.end())
  {
    string target_reg = get_reg();
    cout<<"  lw "<<target_reg<<", "<<loc[value]<<endl; // 把变量的值放到寄存器里
    nums.push_back(target_reg);
    return;
  }
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
      Visit(kind.data.binary, value);
      break;
    case KOOPA_RVT_ALLOC:
      loc[value] = to_string(stack_frame_used) + "(sp)";
      stack_frame_used += 4;
      break;
    case KOOPA_RVT_LOAD:
      Visit(kind.data.load, value);
      break;
    case KOOPA_RVT_STORE:
      Visit(kind.data.store);
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
  Visit(ret.value);
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

void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value) {
  // 执行一些其他的必要操作
  // ...
  // 访问二元运算符
  // 把两个操作符的值放到寄存器里，再运算，换句话说，不会出现I类指令
  if(binary.op == KOOPA_RBO_NOT_EQ || binary.op == KOOPA_RBO_EQ)
  {
    binary_two_operands(binary.lhs, binary.rhs, "xor", value);
    Visit(value);
    cout<<"  "<<binary_op_map[binary.op]<<" "<<nums.back()<<", "<<nums.back()<<endl;
  }
  else if(binary.op == KOOPA_RBO_LE || binary.op == KOOPA_RBO_GE)
  {
    binary_two_operands(binary.lhs, binary.rhs, binary_op_map[binary.op], value);
    Visit(value);
    cout<<"  seqz "<<nums.back()<<", "<<nums.back()<<endl;
  }
  else{
    binary_two_operands(binary.lhs, binary.rhs, binary_op_map[binary.op], value);
    Visit(value);
  }
  cout<<"  sw "<<nums.back()<<", "<<loc[value]<<endl;
  free_reg();
}

void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value) {
  // 执行一些其他的必要操作
  // ...
  // 访问 load 指令
  string target_reg = get_reg();
  cout<<"  lw "<<target_reg<<", "<<loc[load.src]<<endl;
  nums.push_back(target_reg);
  loc[value] = to_string(stack_frame_used) + "(sp)";
  stack_frame_used += 4;
  cout<<"  sw "<<target_reg<<", "<<loc[value]<<endl;
  free_reg();
}

void Visit(const koopa_raw_store_t &store) {
  // 执行一些其他的必要操作
  // ...
  // 访问 store 指令
  Visit(store.value);
  cout<<"  sw "<<nums.back()<<", "<<loc[store.dest]<<endl;
  free_reg();
}
