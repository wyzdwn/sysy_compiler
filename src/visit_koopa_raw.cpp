#include <iostream>
#include "koopa.h"
#include <cstring>
#include <sstream>
#include <cassert>
#include "visit_koopa_raw.hpp"
#include <unordered_map>
#include <deque>
#include <algorithm>

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
const deque<string> tmp_regs=\
{"t0", "t1", "t2", "t3", "t4", "t5", "t6", };
const deque<string> param_regs=\
{"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",};
const int num_regs=tmp_regs.size();
unordered_map<string, int> reg_used; // 记录临时寄存器是否被使用过，1表示被使用过，0表示没被使用过

/********************************lv4 start**********************************/

static unordered_map<koopa_raw_value_t, int> loc; // 有返回值的语句在栈中的位置
static int stack_frame_length = 0; // 栈帧长度
static int stack_frame_used = 0; // 已经使用的栈帧长度

static int saved_ra = 0; // 当前正在访问的函数有没有保存ra

/*********************************lv4 end***********************************/

// 返回没被使用过的第一个寄存器
inline string get_reg()
{
  for(int i=0; i<num_regs; i++)
    if(reg_used[tmp_regs[i]] == 0)
    {
      reg_used[tmp_regs[i]] = 1;
      return tmp_regs[i];
    }
  assert(false);
}

inline void free_reg()
{
  string reg = nums.back();
  nums.pop_back();
  reg_used[reg] = 0;
}

// 将reg中的值存到value所在的位置
inline void save_reg(const koopa_raw_value_t &value, const std::string &reg) {
  if(value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
  {
    string tmp_reg=get_reg();
    cout<<"  lui "<<tmp_reg<<", %hi("<<value->name+1<<")"<<endl;
    cout<<"  sw "<<reg<<", %lo("<<value->name+1<<")("<<tmp_reg<<")"<<endl;
    reg_used[tmp_reg] = 0;
    return;
  }
  string tmp_reg=get_reg();
  cout<<"  li "<<tmp_reg<<", "<<loc[value]<<endl;
  cout<<"  add "<<tmp_reg<<", "<<tmp_reg<<", sp"<<endl;
  cout<<"  sw "<<reg<<", 0("<<tmp_reg<<")"<<endl;
  reg_used[tmp_reg] = 0;
}

inline void load_reg(const koopa_raw_value_t &value, const std::string &reg) {
  // cout<<"value->kind.tag="<<value->kind.tag<<endl;
  if (value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
    const auto& index = value->kind.data.func_arg_ref.index;
    cout<<"index="<<index<<endl;
    if (index < param_regs.size()) {
      std::cout << "  mv " << reg << ", a" << index << std::endl;
    }
    else {
      string tmp_reg=get_reg();
      std::cout << "  li " << tmp_reg << ", " << stack_frame_length + (index - 8) * 4 << std::endl;
      std::cout << "  add " << tmp_reg << ", " << tmp_reg << ", sp" << std::endl;
      std::cout << "  lw " << reg << ", 0(" << tmp_reg << ")" << std::endl;
      reg_used[tmp_reg] = 0;
    }
  } else if(value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    // lui     a1, %hi(x)
    // lw      a0, %lo(x)(a1)
    string tmp_reg=get_reg();
    cout<<"  lui "<<tmp_reg<<", %hi("<<value->name+1<<")"<<endl;
    cout<<"  lw "<<reg<<", %lo("<<value->name+1<<")("<<tmp_reg<<")"<<endl;
    reg_used[tmp_reg] = 0;
    // std::cout << "  la " << reg << ", " << value->name+1 << std::endl;
    // cout<<"  lw "<<reg<<", 0("<<reg<<")"<<endl;
  } else{
    string tmp_reg=get_reg();
    cout<<"  li "<<tmp_reg<<", "<<loc[value]<<endl;
    cout<<"  add "<<tmp_reg<<", "<<tmp_reg<<", sp"<<endl;
    cout<<"  lw "<<reg<<", 0("<<tmp_reg<<")"<<endl;
    reg_used[tmp_reg] = 0;
  }
}

// 给定riscv的运算符（op），以及koopaIR的两个操作数（lhs, rhs），生成对应的汇编代码
inline void binary_two_operands(koopa_raw_value_t lhs, koopa_raw_value_t rhs, string op, const koopa_raw_value_t &value)
{
  
  Visit(lhs);
  Visit(rhs);
  string target_reg = get_reg();
  cout<<"  "<<op<<" "<<target_reg<<", "<<nums[nums.size()-2]<<", "<<nums.back()<<endl;
  free_reg();
  free_reg();
  nums.push_back(target_reg);

  loc[value] = stack_frame_used;
  stack_frame_used += 4;
  save_reg(value, target_reg);
  free_reg();
}



/***********************************main************************************/
// 访问 raw program
void Visit(const koopa_raw_program_t &program) {
  // 执行一些其他的必要操作
  // ...
  for(int i=0; i<num_regs; i++) reg_used[tmp_regs[i]] = 0;
  // 访问所有全局变量
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
  if(func->bbs.len == 0) return;

  cout << "  .text" << endl;
  cout << "  .globl " << func->name+1 << endl;
  cout << func->name+1 << ":" << endl;
  // 清空
  stack_frame_length = 0;
  stack_frame_used = 0;

  // 局部变量个数
  int local_var = 0;
  // 是否需要为 ra 分配栈空间
  int return_addr = 0;
  // 需要为传参预留几个变量的栈空间
  int arg_var = 0;

  // 遍历基本块
  for (size_t i = 0; i < func->bbs.len; ++i)
  {
    const auto& insts = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i])->insts;
    local_var += insts.len;
    for (size_t j = 0; j < insts.len; ++j)
    {
      auto inst = reinterpret_cast<koopa_raw_value_t>(insts.buffer[j]);
      if(inst->ty->tag == KOOPA_RTT_UNIT) local_var--;
      if(inst->kind.tag == KOOPA_RVT_CALL)
      {
        return_addr = 1;
        arg_var = std::max(arg_var, std::max(0, int(inst->kind.data.call.args.len) - 8));
      }
    }
  }
  stack_frame_length = (local_var + return_addr + arg_var) << 2;
  stack_frame_length = (stack_frame_length + 15) & (~15);
  stack_frame_used = arg_var<<2;

  if (stack_frame_length > 0 && stack_frame_length < 2048)
    cout << "  addi sp, sp, -" << stack_frame_length << endl;
  else if (stack_frame_length >= 2048)
    cout << "  li t0, -" << stack_frame_length << endl
              << "  add sp, sp, t0" << endl;

  if(return_addr&&stack_frame_length) {
    std::cout << "  li t0, " << stack_frame_length - 4 << std::endl;
    std::cout << "  add t0, t0, sp" << std::endl;
    std::cout << "  sw ra, 0(t0)" << std::endl;
    saved_ra = 1;
  }
  else saved_ra = 0;


  Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  // 如果bb->name是entry，那么就不用输出标签
  if(strncmp(bb->name+1, "entry", 5))
    std::cout << bb->name+1 << ":" << std::endl;
  Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  if(loc.find(value) != loc.end())
  {
    string target_reg = get_reg();
    load_reg(value, target_reg); // 把变量的值放到寄存器里
    nums.push_back(target_reg);
    return;
  }
  const auto &kind = value->kind;
  // cout<<"kind.tag="<<kind.tag<<": ";
  // return ;
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
      // 访问 binary 指令
      Visit(kind.data.binary, value);
      break;
    case KOOPA_RVT_ALLOC:
      // 访问 alloc 指令
      loc[value] = stack_frame_used;
      stack_frame_used += 4;
      break;
    case KOOPA_RVT_LOAD:
      // 访问 load 指令
      Visit(kind.data.load, value);
      break;
    case KOOPA_RVT_STORE:
      // 访问 store 指令
      Visit(kind.data.store);
      break;
    case KOOPA_RVT_BRANCH:
      // 访问 branch 指令
      Visit(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      // 访问 jump 指令
      Visit(kind.data.jump);
      break;
    case KOOPA_RVT_CALL:
      // 访问 call 指令
      Visit(kind.data.call, value);
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      // 访问 global alloc 指令
      Visit(value->kind.data.global_alloc, value);
      break;
    case KOOPA_RVT_FUNC_ARG_REF:
      // 访问 func arg ref 指令
      if(value->kind.data.func_arg_ref.index < 8)
        nums.push_back(param_regs[value->kind.data.func_arg_ref.index]);
      else
      {
        string tmp_reg = get_reg();
        cout<<"  li "<<tmp_reg<<", "<<stack_frame_length + (value->kind.data.func_arg_ref.index - 8) * 4<<endl;
        cout<<"  add "<<tmp_reg<<", "<<tmp_reg<<", sp"<<endl;
        cout<<"  lw "<<tmp_reg<<", 0("<<tmp_reg<<")"<<endl;
        nums.push_back(tmp_reg);
      }
      break;
    default:
      // 其他类型暂时遇不到
      cout<<"kind.tag="<<kind.tag<<endl;
      assert(false);
  }
}

void Visit(const koopa_raw_return_t &ret) {
  // 执行一些其他的必要操作
  // ...
  
  // 访问返回值
  if(ret.value != nullptr)
  {
    Visit(ret.value);
    cout<<"  mv a0, "<<nums.back()<<endl;
    free_reg();
  }

  // 恢复 ra
  if (saved_ra) {
    string tmp_reg = get_reg();
    cout<<"  li "<<tmp_reg<<", "<<stack_frame_length - 4<<endl;
    cout<<"  add "<<tmp_reg<<", "<<tmp_reg<<", sp"<<endl;
    cout<<"  lw ra, 0("<<tmp_reg<<")"<<endl;
    reg_used[tmp_reg] = 0;
  }

  // 释放栈帧
  string tmp_reg = get_reg();
  if (stack_frame_length != 0) {
    cout<<"  li "<<tmp_reg<<", "<<stack_frame_length<<endl;
    cout<<"  add sp, sp, "<<tmp_reg<<endl;
  }
  reg_used[tmp_reg] = 0;
  cout<<"  ret\n";
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
  if(value->ty->tag != KOOPA_RTT_UNIT)
  {
    save_reg(value, nums.back());
    free_reg();
  }
}

void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value) {
  // 执行一些其他的必要操作
  // ...
  // 访问 load 指令
  string target_reg = get_reg();
  load_reg(load.src, target_reg);
  nums.push_back(target_reg);

  if(value->ty->tag != KOOPA_RTT_UNIT)
  {
    loc[value] = stack_frame_used;
    stack_frame_used += 4;
    save_reg(value, target_reg);
    free_reg();
  }
}

void Visit(const koopa_raw_store_t &store) {
  // 执行一些其他的必要操作
  // ...
  // 访问 store 指令
  Visit(store.value);
  save_reg(store.dest, nums.back());
  free_reg();
}

void Visit(const koopa_raw_branch_t &branch) {
  // 执行一些其他的必要操作
  // ...
  // 访问 branch 指令
  Visit(branch.cond);
  
  // 此处不直接跳转至true_bb，因为bnez的跳转距离有限，但是j的跳转距离非常大
  // 所以先跳转到TO_true_bb，这里有且仅有“j true_bb"，再跳转到true_bb
  cout<<"  bnez "<<nums.back()<<", TO_"<<branch.true_bb->name+1<<endl;
  free_reg();
  cout<<"  j "<<branch.false_bb->name+1<<endl;
  cout<<"TO_"<<branch.true_bb->name+1<<":"<<endl;
  cout<<"  j "<<branch.true_bb->name+1<<endl;
}

void Visit(const koopa_raw_jump_t &jump) {
  // 执行一些其他的必要操作
  // ...
  // 访问 jump 指令
  cout<<"  j "<<jump.target->name+1<<endl;
}

void Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value) {
  // 处理参数
  for (size_t i = 0; i < call.args.len; ++i) {
    auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
    Visit(arg);
    if (i < 8) {
      cout<<"  mv a"<<i<<", "<<nums.back()<<endl;
      free_reg();
    }
    else {
      string src_reg = nums.back();
      string tmp_reg = get_reg();
      std::cout << "  li " << tmp_reg << ", " << (i - 8) * 4 << std::endl;
      std::cout << "  add " << tmp_reg << ", " << tmp_reg << ", sp" << std::endl;
      std::cout << "  sw " << src_reg << ", 0(" << tmp_reg << ")" << std::endl;
      reg_used[tmp_reg] = 0;
      free_reg();
    }
  }
  std::cout << "  call " << call.callee->name+1 << std::endl;

  // 若有返回值则将 a0 中的结果存入栈
  if(value->ty->tag != KOOPA_RTT_UNIT) {
    loc[value] = stack_frame_used;
    stack_frame_used += 4;
    save_reg(value, "a0");
  }
}

void Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value) {
  std::cout << "  .data" << std::endl;
  std::cout << "  .globl " << value->name+1 << std::endl;
  std::cout << value->name+1 << ":" << std::endl;
  if (global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT) {
    // 初始化为 0
    std::cout << "  .zero 4" << std::endl;
  }
  else if (global_alloc.init->kind.tag == KOOPA_RVT_INTEGER) {
    // 初始化为 int
    std::cout << "  .word " << global_alloc.init->kind.data.integer.value << std::endl;
  }
  std::cout << std::endl;
}