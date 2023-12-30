#include <cassert>
#include <fstream>
#include <unordered_map>
#include "ast.hpp"
#include <memory>
#include "koopa.h"
#include <string>
#include <sstream>
#include "visit_koopa_raw.hpp"

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

std::unordered_map<char, const char *> generator = {
    {'k', R"(fun @main(): i32 {
%entry:
  ret 0
}
)"},
    {'r', R"(  .text
  .globl main
main:
  li a0, 0
  ret
)"},
    {'p', R"(  .text
  .globl main
main:
  li a0, 0
  ret
)"},
};

int main(int argc, const char *argv[]) {
  assert(argc == 5);
  ofstream ofs(argv[4]);
  // ofs << generator[argv[1][1]];

  // parse input file
  yyin = fopen(argv[2], "r");
  assert(yyin);

  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);

  stringstream ss;
  

  // dump AST
  ast->Dump();
  cout << endl;

  // 将标准输出重定向到ofs
  streambuf *cout_backup = cout.rdbuf(ss.rdbuf());;

  
  // generate Koopa IR
  ast->KoopaIR();
  string str = ss.str();
  cout.rdbuf(ofs.rdbuf());
  if(string(argv[1])=="-koopa")
  {
    cout << str;
  }
  else if(string(argv[1])=="-riscv")
  {
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(str.c_str(), &program);
    assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    // 处理 raw program
    Visit(raw);
    // ...

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
  }

  // 恢复标准输出
  cout.rdbuf(cout_backup);
  cout<<ss.str()<<endl;
  return 0;
}
