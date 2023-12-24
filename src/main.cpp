#include <cassert>
#include <fstream>
#include <unordered_map>
#include "ast.hpp"
#include <memory>

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

  // dump AST
  ast->Dump();
  cout << endl;

  // 将标准输出重定向到ofs
  streambuf *cout_backup = cout.rdbuf(ofs.rdbuf());

  // generate Koopa IR
  ast->KoopaIR();
  cout << endl;

  // 恢复标准输出
  cout.rdbuf(cout_backup);
  
  return 0;
}
