%code requires {
  #include <memory>
  #include <string>
  #include "ast.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "ast.hpp"
#include <vector>

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  char char_val;
  std::vector<std::unique_ptr<BaseAST>> *vec_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token RETURN AND OR CONST IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT EQOP RELOP TYPE
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef Block Stmt Exp PrimaryExp Number UnaryExp AddExp MulExp LOrExp LAndExp EqExp RelExp
%type <char_val> UnaryOp AddOp MulOp

%type <ast_val> Decl ConstDecl ConstDef ConstInitVal ConstExp BlockItem LVal
%type <ast_val> VarDecl VarDef InitVal

%type <vec_val> BlockItemList ConstDefList VarDefList

%type <ast_val> IfStmt OnlyIf IfElse

%type <vec_val> FuncFParams  FuncRParams CompUnitItemList

%type <ast_val> CompUnitItem FuncFParam

%type <vec_val> ConstIndexList ConstArrayInitVal ConstInitValList ArrayInitVal IndexList InitValList


%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : CompUnitItemList {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->comp_unit_item_list = unique_ptr<vector<unique_ptr<BaseAST>>>($1);
    ast = move(comp_unit);
  }
  ;

CompUnitItemList
  : CompUnitItem {
    auto vec = new vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | CompUnitItemList CompUnitItem {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;

CompUnitItem
  : FuncDef {
    auto ast = new CompUnitItemAST();
    ast->func_def = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Decl {
    auto ast = new CompUnitItemAST();
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
FuncDef
  : TYPE IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->func_f_param_list = unique_ptr<vector<unique_ptr<BaseAST> > >($4);
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;

FuncFParams
  : FuncFParam {
    auto vec = new vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | FuncFParams ',' FuncFParam {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  | {
    auto vec = new vector<unique_ptr<BaseAST>>();
    $$ = vec;
  }
  ;

FuncFParam
  : TYPE IDENT {
    auto ast = new FuncFParamAST();
    ast->b_type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  ;

Block
  : '{' BlockItemList '}' {
    auto ast = new BlockAST();
    ast->block_item_list = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = ast;
  }
  ;

BlockItemList
  : BlockItem {
    auto vec = new vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | BlockItemList BlockItem {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  | {
    auto vec = new vector<unique_ptr<BaseAST>>();
    $$ = vec;
  }
  ;

BlockItem
  : Stmt {
    auto ast = new BlockItemAST();
    ast->stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Decl {
    auto ast = new BlockItemAST();
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Stmt
  : RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->exp = unique_ptr<BaseAST>($2);
    ast->return_ = true;
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtAST();
    ast->return_ = true;
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new StmtAST();
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtAST();
    ast->exp_only = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtAST();
    $$ = ast;
  }
  | IfStmt {
    auto ast = new StmtAST();
    ast->if_stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->while_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtAST();
    ast->break_ = true;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtAST();
    ast->continue_ = true;
    $$ = ast;
  }
  ;

IfStmt
  : OnlyIf {
    auto ast=new IfStmtAST();
    ast->if_stmt=unique_ptr<BaseAST>($1);
    $$=ast;
  }
  | IfElse {
    auto ast=new IfStmtAST();
    ast->if_stmt=unique_ptr<BaseAST>($1);
    $$=ast;
  }
  ;

OnlyIf
  : IF '(' Exp ')' Stmt {
    auto ast = new OnlyIfAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

IfElse
  : IF '(' Exp ')' Stmt ELSE Stmt {
    auto ast = new IfElseAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->if_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->lor_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LVal
  : IDENT IndexList {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    ast->index_list = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = ast;
  }
  ;

IndexList
  : {
    auto vec = new vector<unique_ptr<BaseAST>>();
    $$ = vec;
  }
  | IndexList '[' Exp ']' {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

PrimaryExp
  : '(' Exp ')'{
    auto ast = new PrimaryExpAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->number = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LVal{
    auto ast = new PrimaryExpAST();
    ast->lval = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    auto ast = new NumberAST();
    ast->n = $1;
    $$ = ast;
  }
  ;


UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->primary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | UnaryOp UnaryExp{
    auto ast = new UnaryExpAST();
    ast->unary_op = $1;
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')'{
    auto ast = new UnaryExpAST();
    ast->ident = *unique_ptr<string>($1);
    ast->func_r_param_list = unique_ptr<vector<unique_ptr<BaseAST>>>($3);
    $$ = ast;
  }
  ;

FuncRParams
  : Exp {
    auto vec = new vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | FuncRParams ',' Exp {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  | {
    auto vec = new vector<unique_ptr<BaseAST>>();
    $$ = vec;
  }

UnaryOp
  : '-' {
    $$ = '-';
  }
  | '!' {
    $$ = '!';
  }
  | '+' {
    $$ = '+';
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp AddOp MulExp {
    auto ast = new AddExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->add_op = $2;
    ast->mul_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

AddOp
  : '+' {
    $$ = '+';
  }
  | '-' {
    $$ = '-';
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->unary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp MulOp UnaryExp {
    auto ast = new MulExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->mul_op = $2;
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

MulOp
  : '*' {
    $$ = '*';
  }
  | '/' {
    $$ = '/';
  }
  | '%' {
    $$ = '%';
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->land_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp OR LAndExp {
    auto ast = new LOrExpAST();
    ast->lor_exp = unique_ptr<BaseAST>($1);
    ast->land_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp AND EqExp {
    auto ast = new LAndExpAST();
    ast->land_exp = unique_ptr<BaseAST>($1);
    ast->eq_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EQOP RelExp {
    auto ast = new EqExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->eq_op = *unique_ptr<string>($2);
    ast->rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp RELOP AddExp {
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->rel_op = *unique_ptr<string>($2);
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

// level 4 begin
Decl
  : ConstDecl{
    auto ast = new DeclAST();
    ast->const_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl{
    auto ast = new DeclAST();
    ast->var_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST TYPE ConstDefList ';'{
    auto ast = new ConstDeclAST();
    ast->b_type = *unique_ptr<string>($2);
    ast->const_def_list = unique_ptr<vector<unique_ptr<BaseAST>>>($3);
    $$ = ast;
  }
  ;

ConstDefList
  : ConstDef{
    auto vec = new vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ConstDefList ',' ConstDef{
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

ConstDef
  : IDENT ConstIndexList '=' ConstInitVal{
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->const_index_list = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    ast->const_init_val = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

ConstIndexList
  : {
    auto vec = new vector<unique_ptr<BaseAST>>();
    $$ = vec;
  }
  | ConstIndexList '[' ConstExp ']' {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = $1;
  }
  ;

ConstInitVal
  : ConstExp{
    auto ast = new ConstInitValAST();
    ast->const_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ConstArrayInitVal{
    auto ast = new ConstInitValAST();
    ast->const_array_init_val = unique_ptr<vector<unique_ptr<BaseAST>>>($1);
    $$ = ast;
  }
  ;

ConstArrayInitVal
  : '{' '}'{
    auto vec = new vector<unique_ptr<BaseAST>>();
    $$ = vec;
  }
  | '{' ConstInitValList '}'{
    auto vec = $2;
    $$ = vec;
  }
  ;

ConstInitValList
  : ConstInitVal{
    auto vec = new vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ConstInitValList ',' ConstInitVal{
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

ConstExp
  : Exp{
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

VarDecl
  : TYPE VarDefList ';'{
    auto ast = new VarDeclAST();
    ast->b_type = *unique_ptr<string>($1);
    ast->var_def_list = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = ast;
  }
  ;

VarDefList
  : VarDef{
    auto vec = new vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | VarDefList ',' VarDef{
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

VarDef
  : IDENT ConstIndexList{
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->const_index_list = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = ast;
  }
  | IDENT ConstIndexList '=' InitVal{
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->const_index_list = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    ast->init_val = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

InitVal
  : Exp{
    auto ast = new InitValAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ArrayInitVal{
    auto ast = new InitValAST();
    ast->array_init_val = unique_ptr<vector<unique_ptr<BaseAST>>>($1);
    $$ = ast;
  }
  ;

ArrayInitVal
  : '{' '}'{
    auto vec = new vector<unique_ptr<BaseAST>>();
    $$ = vec;
  }
  | '{' InitValList '}'{
    auto vec = $2;
    $$ = vec;
  }
  ;

InitValList
  : InitVal{
    auto vec = new vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | InitValList ',' InitVal{
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
