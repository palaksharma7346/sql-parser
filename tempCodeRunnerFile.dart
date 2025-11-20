// sql_complete.cpp
// Complete mini SQL parser (Lexer + Parser for SELECT/INSERT/UPDATE/DELETE)

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <utility>

using namespace std;

/* ================================
   Tokens & Lexer
================================ */
enum class TokType {
    // Keywords
    SELECT, FROM, WHERE, AS, AND, OR, LIKE, IN,
    INSERT, INTO, VALUES, UPDATE, SET, DELETE,
    // Symbols
    STAR, COMMA, DOT, LPAREN, RPAREN, SEMI, EQ, NEQ, LT, LTE, GT, GTE,
    // Others
    IDENT, NUMBER, STRING,
    END, INVALID
};

struct Token {
    TokType type;
    string  lex;
    int     pos;
    int     line, col;
    Token(TokType t=TokType::END, const string& lx="", int p=0, int ln=1, int cl=1)
        : type(t), lex(lx), pos(p), line(ln), col(cl) {}
};

static TokType kw_or_ident(const string& up) {
    if (up == "SELECT") return TokType::SELECT;
    if (up == "FROM")   return TokType::FROM;
    if (up == "WHERE")  return TokType::WHERE;
    if (up == "AS")     return TokType::AS;
    if (up == "AND")    return TokType::AND;
    if (up == "OR")     return TokType::OR;
    if (up == "LIKE")   return TokType::LIKE;
    if (up == "IN")     return TokType::IN;
    if (up == "INSERT") return TokType::INSERT;
    if (up == "INTO")   return TokType::INTO;
    if (up == "VALUES") return TokType::VALUES;
    if (up == "UPDATE") return TokType::UPDATE;
    if (up == "SET")    return TokType::SET;
    if (up == "DELETE") return TokType::DELETE;
    return TokType::IDENT;
}

struct Lexer {
    string s; int i, n; int line, col;
    Lexer(const string& in) : s(in), i(0), n((int)in.size()), line(1), col(1) {}

    bool done() const { return i>=n; }
    char peek(int k=0) const { return (i+k<n)? s[i+k] : '\0'; }

    void bump() {
        if (done()) return;
        if (s[i] == '\n') { line++; col = 1; }
        else col++;
        i++;
    }

    static bool is_ident_start(char c){ return isalpha((unsigned char)c) || c=='_'; }
    static bool is_ident_char (char c){ return isalnum((unsigned char)c) || c=='_'; }

    Token make(TokType t, int start, int l, int c, const string& lex=""){
        if (lex.empty()) return Token(t, s.substr(start, i-start), start, l, c);
        return Token(t, lex, start, l, c);
    }

    Token string_lit(){
        int start=i, l=line, c=col;
        bump();
        string out; bool closed=false;
        while(!done()){
            char ch = peek();
            bump();
            if (ch=='\''){
                if (peek()=='\''){ out.push_back('\''); bump(); }
                else { closed=true; break; }
            } else out.push_back(ch);
        }
        if(!closed) return Token(TokType::INVALID, "Unterminated string", start, l, c);
        return Token(TokType::STRING, out, start, l, c);
    }

    Token number(){
        int start=i, l=line, c=col; bool dot=false;
        while (isdigit((unsigned char)peek()) || (!dot && peek()=='.')) {
            if (peek()=='.') dot=true;
            bump();
        }
        return make(TokType::NUMBER, start, l, c);
    }

    Token ident_or_kw(){
        int start=i, l=line, c=col;
        while (is_ident_char(peek())) bump();
        string raw = s.substr(start, i-start);
        string up = raw; 
        for (size_t k=0;k<up.size();++k) up[k] = (char)toupper((unsigned char)up[k]);
        TokType tt = kw_or_ident(up);
        return Token(tt, raw, start, l, c);
    }

    Token next(){
        while (isspace((unsigned char)peek())) bump();
        int start=i, l=line, c=col;
        if (done()) return Token(TokType::END, "", i, line, col);
        char ch = peek();

        if (ch=='!' && peek(1)=='='){ bump(); bump(); return make(TokType::NEQ, start, l, c); }
        if (ch=='<' && peek(1)=='='){ bump(); bump(); return make(TokType::LTE, start, l, c); }
        if (ch=='>' && peek(1)=='='){ bump(); bump(); return make(TokType::GTE, start, l, c); }

        switch(ch){
            case '*': bump(); return make(TokType::STAR,  start, l, c);
            case ',': bump(); return make(TokType::COMMA, start, l, c);
            case '.': bump(); return make(TokType::DOT,   start, l, c);
            case '(': bump(); return make(TokType::LPAREN,start, l, c);
            case ')': bump(); return make(TokType::RPAREN,start, l, c);
            case ';': bump(); return make(TokType::SEMI,  start, l, c);
            case '=': bump(); return make(TokType::EQ,    start, l, c);
            case '<': bump(); return make(TokType::LT,    start, l, c);
            case '>': bump(); return make(TokType::GT,    start, l, c);
            case '\'': return string_lit();
        }

        if (isdigit((unsigned char)ch)) return number();
        if (is_ident_start(ch))         return ident_or_kw();

        bump();
        return Token(TokType::INVALID, string(1, ch), start, l, c);
    }

    vector<Token> tokenize(){
        vector<Token> out;
        for(;;){
            Token t = next();
            out.push_back(t);
            if (t.type == TokType::END) break;
        }
        return out;
    }
};

/* ================================
   AST
================================ */
struct Expr {
    string kind;
    string value;
    vector<Expr*> kids;
    Expr(const string& k="", const string& v="") : kind(k), value(v), kids() {}
    ~Expr(){ for (auto x:kids) delete x; }
};

struct SelectItem { vector<string> path; bool isStar; string alias; SelectItem(): isStar(false) {} };
struct TableRef { vector<string> path; string alias; };
struct InsertQuery { string table; vector<string> cols, values; };
struct UpdateQuery { string table; vector<pair<string,string>> setList; Expr* where=nullptr; ~UpdateQuery(){ delete where;} };
struct DeleteQuery { string table; Expr* where=nullptr; ~DeleteQuery(){ delete where;} };

struct Query {
    string type;
    vector<SelectItem> selectItems;
    vector<TableRef> from;
    Expr* where=nullptr;
    InsertQuery* insertStmt=nullptr;
    UpdateQuery* updateStmt=nullptr;
    DeleteQuery* deleteStmt=nullptr;
    ~Query(){ delete where; delete insertStmt; delete updateStmt; delete deleteStmt; }
};

/* ================================
   Parser
================================ */
struct ParseError : runtime_error { int line,col; ParseError(string m,int l,int c):runtime_error(m),line(l),col(c){} };

struct Parser {
    const vector<Token>& T; int k; const string& input;
    Parser(const vector<Token>& t,const string& s):T(t),k(0),input(s){}

    const Token& peek(int d=0) const { int idx=k+d; return idx>=T.size()?T.back():T[idx];}
    bool match(TokType tt){ if(peek().type==tt){k++;return true;} return false; }
    const Token& expect(TokType tt,string name){
        if(peek().type==tt) return T[k++];
        auto&t=peek(); throw ParseError("Expected "+name+" but found '"+t.lex+"'",t.line,t.col);
    }

    Query parseQuery(){
        if(peek().type==TokType::SELECT) return parseSelect();
        if(peek().type==TokType::INSERT) return parseInsert();
        if(peek().type==TokType::UPDATE) return parseUpdate();
        if(peek().type==TokType::DELETE) return parseDelete();
        auto&t=peek(); throw ParseError("Unknown statement '"+t.lex+"'",t.line,t.col);
    }

    // SELECT
    Query parseSelect(){
        Query q; q.type="SELECT";
        expect(TokType::SELECT,"SELECT");
        if(match(TokType::STAR)){ SelectItem si; si.isStar=true; q.selectItems.push_back(si); }
        else{ q.selectItems.push_back(parseSelectItem()); while(match(TokType::COMMA)) q.selectItems.push_back(parseSelectItem());}
        expect(TokType::FROM,"FROM");
        q.from.push_back(parseTableRef());
        while(match(TokType::COMMA)) q.from.push_back(parseTableRef());
        if(match(TokType::WHERE)) q.where=parseDisjunction();
        if(match(TokType::SEMI)){}
        if(peek().type!=TokType::END){auto&t=peek(); throw ParseError("Unexpected token '"+t.lex+"'",t.line,t.col);}
        return q;
    }

    SelectItem parseSelectItem(){
        SelectItem si; si.path=parseIdentPath();
        if(match(TokType::AS)) si.alias=expect(TokType::IDENT,"alias").lex;
        else if(peek().type==TokType::IDENT){ si.alias=peek().lex; k++; }
        return si;
    }

    TableRef parseTableRef(){
        TableRef tr; tr.path=parseIdentPath();
        if(match(TokType::AS)) tr.alias=expect(TokType::IDENT,"alias").lex;
        else if(peek().type==TokType::IDENT){ tr.alias=peek().lex; k++; }
        return tr;
    }

    vector<string> parseIdentPath(){
        auto&f=expect(TokType::IDENT,"identifier");
        vector<string> path{f.lex};
        while(match(TokType::DOT)) path.push_back(expect(TokType::IDENT,"identifier").lex);
        return path;
    }

    Expr* parseDisjunction(){
        Expr* left=parseConjunction();
        while(peek().type==TokType::OR){ k++; auto*right=parseConjunction(); auto*n=new Expr("bin","OR"); n->kids={left,right}; left=n; }
        return left;
    }
    Expr* parseConjunction(){
        Expr* left=parseCondition();
        while(peek().type==TokType::AND){ k++; auto*right=parseCondition(); auto*n=new Expr("bin","AND"); n->kids={left,right}; left=n; }
        return left;
    }
    Expr* parseCondition(){
        if(peek().type==TokType::LPAREN){ k++; Expr*e=parseDisjunction(); expect(TokType::RPAREN,")"); return e;}
        Expr*l=parseOperand(); auto&op=peek();
        if(op.type==TokType::EQ||op.type==TokType::NEQ||op.type==TokType::LT||op.type==TokType::LTE||op.type==TokType::GT||op.type==TokType::GTE||op.type==TokType::LIKE||op.type==TokType::IN){
            k++; Expr*r=parseOperand(); auto*n=new Expr("bin",op.lex); n->kids={l,r}; return n;
        }
        throw ParseError("Expected condition operator",op.line,op.col);
    }

    Expr* parseOperand(){
        auto&t=peek();
        if(t.type==TokType::IDENT){k++;return new Expr("ident",t.lex);}
        if(t.type==TokType::NUMBER){k++;return new Expr("number",t.lex);}
        if(t.type==TokType::STRING){k++;return new Expr("string",t.lex);}
        throw ParseError("Expected operand",t.line,t.col);
    }

    // INSERT
    Query parseInsert(){
        Query q; q.type="INSERT"; auto*ins=new InsertQuery();
        expect(TokType::INSERT,"INSERT"); expect(TokType::INTO,"INTO");
        ins->table=expect(TokType::IDENT,"table").lex;
        if(match(TokType::LPAREN)){
            ins->cols.push_back(expect(TokType::IDENT,"col").lex);
            while(match(TokType::COMMA)) ins->cols.push_back(expect(TokType::IDENT,"col").lex);
            expect(TokType::RPAREN,")");
        }
        expect(TokType::VALUES,"VALUES"); expect(TokType::LPAREN,"(");
        ins->values.push_back(parseValue());
        while(match(TokType::COMMA)) ins->values.push_back(parseValue());
        expect(TokType::RPAREN,")");
        match(TokType::SEMI);
        q.insertStmt=ins;
        return q;
    }

    string parseValue(){
        auto&t=peek();
        if(t.type==TokType::NUMBER||t.type==TokType::STRING){k++;return t.lex;}
        throw ParseError("Expected value",t.line,t.col);
    }

    // UPDATE
    Query parseUpdate(){
        Query q; q.type="UPDATE"; auto*up=new UpdateQuery();
        expect(TokType::UPDATE,"UPDATE");
        up->table=expect(TokType::IDENT,"table").lex;
        expect(TokType::SET,"SET");
        string c=expect(TokType::IDENT,"col").lex; expect(TokType::EQ,"="); string v=parseValue();
        up->setList.push_back({c,v});
        while(match(TokType::COMMA)){
            string c2=expect(TokType::IDENT,"col").lex; expect(TokType::EQ,"="); string v2=parseValue();
            up->setList.push_back({c2,v2});
        }
        if(match(TokType::WHERE)) up->where=parseDisjunction();
        match(TokType::SEMI);
        q.updateStmt=up;
        return q;
    }

    // DELETE
    Query parseDelete(){
        Query q; q.type="DELETE"; auto*del=new DeleteQuery();
        expect(TokType::DELETE,"DELETE"); expect(TokType::FROM,"FROM");
        del->table=expect(TokType::IDENT,"table").lex;
        if(match(TokType::WHERE)) del->where=parseDisjunction();
        match(TokType::SEMI);
        q.deleteStmt=del;
        return q;
    }
};

/* ================================
   Printing
================================ */
static void print_expr(const Expr* e){
    if(!e){ cout<<"(null)"; return;}
    if(e->kind=="bin"){
        cout<<"(";
        if(e->kids.size()) print_expr(e->kids[0]);
        cout<<" "<<e->value<<" ";
        if(e->kids.size()>1) print_expr(e->kids[1]);
        cout<<")";
    } else cout<<e->value;
}

static void print_query(const Query& q){
    cout<<"Parsed: "<<q.type<<"\n";
    if(q.type=="SELECT"){
        cout<<"  SELECT: ";
        for(size_t i=0;i<q.selectItems.size();++i){
            if(i)cout<<", ";
            auto&si=q.selectItems[i];
            if(si.isStar) cout<<"*";
            else{
                for(size_t p=0;p<si.path.size();++p){
                    if(p) cout<<".";
                    cout<<si.path[p];
                }
                if(!si.alias.empty()) cout<<" AS "<<si.alias;
            }
        }
        cout<<"\n  FROM: ";
        for(size_t i=0;i<q.from.size();++i){
            if(i)cout<<", ";
            auto&tr=q.from[i];
            for(size_t p=0;p<tr.path.size();++p){ if(p)cout<<"."; cout<<tr.path[p];}
            if(!tr.alias.empty()) cout<<" AS "<<tr.alias;
        }
        cout<<"\n";
        if(q.where){ cout<<"  WHERE: "; print_expr(q.where); cout<<"\n";}
    }
    else if(q.type=="INSERT" && q.insertStmt){
        cout<<"  INTO "<<q.insertStmt->table<<" (";
        for(size_t i=0;i<q.insertStmt->cols.size();++i){ if(i)cout<<", "; cout<<q.insertStmt->cols[i];}
        cout<<") VALUES (";
        for(size_t i=0;i<q.insertStmt->values.size();++i){ if(i)cout<<", "; cout<<q.insertStmt->values[i];}
        cout<<")\n";
    }
    else if(q.type=="UPDATE" && q.updateStmt){
        cout<<"  TABLE "<<q.updateStmt->table<<"\n  SET ";
        for(size_t i=0;i<q.updateStmt->setList.size();++i){
            if(i)cout<<", ";
            cout<<q.updateStmt->setList[i].first<<" = "<<q.updateStmt->setList[i].second;
        }
        cout<<"\n";
        if(q.updateStmt->where){ cout<<"  WHERE: "; print_expr(q.updateStmt->where); cout<<"\n";}
    }
    else if(q.type=="DELETE" && q.deleteStmt){
        cout<<"  FROM "<<q.deleteStmt->table<<"\n";
        if(q.deleteStmt->where){ cout<<"  WHERE: "; print_expr(q.deleteStmt->where); cout<<"\n";}
    }
}

/* ================================
   MAIN
================================ */
int main(){
    cout<<"Enter one SQL statement (end with Enter). ';' is optional.\n";
    string sql; if(!getline(cin,sql)) return 0;

    try{
        Lexer lex(sql);
        auto tokens = lex.tokenize();
        for(auto&t:tokens) if(t.type==TokType::INVALID)
            throw ParseError("Invalid token: "+t.lex,t.line,t.col);

        Parser p(tokens,sql);
        Query q=p.parseQuery();
        print_query(q);
    }
    catch(const ParseError&e){
        cerr<<"Parse error at line "<<e.line<<", col "<<e.col<<": "<<e.what()<<"\n"<<sql<<"\n";
        for(int i=1;i<e.col;i++) cerr<<" ";
        cerr<<"^\n";
        return 1;
    }
    return 0;
}


// SAMPLE QUERIES BELOW (COMMENT SECTION)

// SELECT emp_id, name, department, city, salary 
// FROM employees 
// WHERE (age > 25 AND city = 'Delhi') 
//    OR (age < 22 AND city = 'Mumbai') 
//    OR (salary >= 50000 AND department = 'Engineering') 
//    AND status = 'active' 
//    AND (experience > 2 OR position = 'Manager');

// INSERT INTO employees (emp_id, name, department, city, salary) 
// VALUES (101, 'Kanishk', 'Engineering', 'Delhi', 75000);

// UPDATE employees 
// SET salary = 85000, city = 'Bangalore' 
// WHERE emp_id = 101 AND department = 'Engineering';

// DELETE FROM employees 
// WHERE salary < 30000 OR city = 'Kolkata';