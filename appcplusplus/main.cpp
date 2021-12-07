/*
 *  UCF COP3330 Fall 2021 Assignment 6 Solution
 *  Copyright 2021 Anthony Shiller
 */

#include "std_lib_facilities.h"
using namespace std;

const char number = '8';
const char quit = 'q';
const char print = ';';
const char name = 'a';
const char let = 'L';
const char help = '?';
const char c_sin = 's';
const char c_cos = 'c';
const string prompt = "> ";
const string result = "= ";
const string declkey = "let";
const char square_root = '@';
const char exponent = '^';
const string sqrtkey = "sqrt";
const string expkey = "pow";
const string sinkey = "sin";
const string coskey = "cos";
const string quitkey = "quit";
const string helpkey = "help";

class Token {
public:
    char kind;
    double value;
    string name;

    Token(char k) : kind{k}, value{0} { }
    Token(char k, double v) : kind{k}, value{v} { }
    Token(char k, string n) : kind{k}, value{0}, name{n} { }
};

class Token_stream {
public:
    Token get();
    void putback(Token t);
    void ignore(char c);
private:
    bool full { false };
    Token buffer {'0'};
};

void Token_stream::ignore(char c)
{
    if (full && c == buffer.kind) {
        full = false;
        return;
    }
    full = false;
    char ch = 0;
    while (cin >> ch)
        if (ch == c) return;
}

void Token_stream::putback(Token t)
{
    buffer = t;
    full = true;
};

Token Token_stream::get()
{
    if (full) {
        full = false;
        return buffer;
    }
    char ch;
    cin.get(ch);
    while (isspace(ch) && ch != '\n') cin.get(ch);
    switch (ch) {
        case '\n':
            return Token{print};
        case print:
        case quit:
        case help:
        case '(':
        case ')':
        case '{':
        case '}':
        case '!':
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '=':
        case ',':
            return Token { ch };
        case '.':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            {
                cin.putback(ch);
                double val;
                cin >> val;
                return Token { number, val };
            }
        default:
            if (isalpha(ch)) {
                string s;
                s += ch;
                while (cin.get(ch) &&
                        ((isalpha(ch) || isdigit(ch) || ch == '_')))
                    s += ch;
                cin.putback(ch);
                if (s == declkey) return Token{let};
                else if (s == sqrtkey) return Token{square_root};
                else if (s == expkey) return Token{exponent};
                else if (s == sinkey) return Token{c_sin};
                else if (s == coskey) return Token{c_cos};
                else if (s == quitkey) return Token{quit};
                else if (s == helpkey) return Token{help};
                else return Token{name, s};
            }
            error("Bad token");
    }
};
class Variable {
public:
    string name;
    double value;
    bool constant;
    Variable(string n, double v, bool c = false)
        : name{n}, value{v}, constant{c} { }
};
class Symbol_table {
    vector<Variable> var_table;
public:
    bool is_declared(string);
    double get_value(string);
    double set_value(string, double);
    double define_name(string, double, bool con = false);
};
bool Symbol_table::is_declared(string var)
{
    for (const Variable& v : var_table)
        if (v.name == var) return true;
    return false;
}

double Symbol_table::get_value(string s)
{
    for (const Variable& v : var_table)
        if (v.name == s) return v.value;
    error("get: undefined variable ", s);
}
double Symbol_table::set_value(string s, double d)
{
    for (Variable& v : var_table)
        if (v.name == s) {
            if (v.constant) error("Can't overwrite constant variable");
            v.value = d;
            return d;
        }
    error("set: undefined variable ", s);
}
double Symbol_table::define_name(string var, double val, bool con)
{
    if (is_declared(var)) error(var, " declared twice");
    var_table.push_back(Variable{var,val,con});
    return val;
}
Symbol_table st;
Token_stream ts;
double expression();
double calc_sqrt()
{
    char ch;
    if (cin.get(ch) && ch != '(') error("'(' expected");
    cin.putback(ch);
    double d = expression();
    if (d < 0) error("sqrt: negative val is imaginary");
    return sqrt(d);
}
double calc_pow()
{
    Token t = ts.get();
    if (t.kind != '(') error("'(' expected");
    double base = expression();
    t = ts.get();
    if (t.kind != ',') error("',' expected");
    int power = narrow_cast<int>(expression());
    t = ts.get();
    if (t.kind != ')') error("')' expected");
    return pow(base, power);
}
double calc_sin()
{
    char ch;
    if (cin.get(ch) && ch != '(') error("'(' expected");
    cin.putback(ch);
    double d = expression();
    if (d == 0 || d == 180) return 0;
    return sin(d*3.1415926535/180);
}
double calc_cos()
{
    char ch;
    if (cin.get(ch) && ch != '(') error("'(' expected");
    cin.putback(ch);
    double d = expression();
    if (d == 90 || d == 270) return 0;
    return cos(d*3.1415926535/180);
}
double handle_variable(Token& t)
{
    Token t2 = ts.get();
    if (t2.kind == '=')
        return st.set_value(t.name, expression());
    else {
        ts.putback(t2);
        return st.get_value(t.name);
    }
}
double primary()
{
    Token t = ts.get();
    switch (t.kind) {
        case '(':
            {
                double d = expression();
                t = ts.get();
                if (t.kind != ')') error("')' expected");
                return d;
            }
        case '{':
            {
                double d = expression();
                t = ts.get();
                if (t.kind != '}') error("'}' expected");
                return d;
            }
        case number:
            return t.value;
        case name:
            return handle_variable(t);
        case '-':
            return -primary();
        case '+':
            return primary();
        case square_root:
            return calc_sqrt();
        case exponent:
            return calc_pow();
        case c_sin:
            return calc_sin();
        case c_cos:
            return calc_cos();
        default:
            error("primary expected");
    }
}

double secondary()
{
    double left = primary();
    Token t = ts.get();

    while (true) {
        switch (t.kind) {
            case '!':
                if (left == 0) return 1;
                for (int i = left - 1; i > 0; --i) left *= i;
                t = ts.get();
                break;
            default:
                ts.putback(t);
                return left;
        }
    }
}

double term()
{
    double left = secondary();
    Token t = ts.get();
    while (true) {
        switch (t.kind) {
            case '*':
                left *= secondary();
                t = ts.get();
                break;
            case '/':
                {
                    double d = secondary();
                    if (d == 0) error("divide by zero");
                    left /= d;
                    t = ts.get();
                    break;
                }
            case '%':
                {
                    double d = secondary();
                    if (d == 0) error("%: divide by zero");
                    left = fmod(left, d);
                    t = ts.get();
                    break;
                }
            default:
                ts.putback(t);
                return left;
        }
    }
}

double expression()
{
    double left = term();
    Token t = ts.get();

    while (true) {
        switch (t.kind) {
            case '+':
                left += term();
                t = ts.get();
                break;
            case '-':
                left -= term();
                t = ts.get();
                break;
            default:
                ts.putback(t);
                return left;
        }
    }
}
double declaration()
{
    Token t = ts.get();
    if (t.kind != name) error("name expected in declaration");
    string var_name = t.name;

    Token t2 = ts.get();
    if (t2.kind != '=') error("= missing in declaration of ", var_name);

    double d = expression();
    st.define_name(var_name, d);
    return d;
}
double statement()
{
    Token t = ts.get();
    switch (t.kind) {
        case let:
            return declaration();
        default:
            ts.putback(t);
            return expression();
    }
}
void print_help()
{
    cout << "Calculator Manual\n"
         << "========================\n"
         << "This calculator program supports +, -, *, and / \n"
         << "Enter any compound statement followed by ';' for result\n"
         << "- ex: 4 + 1; (5-2)/{6*(8+14)}\n"
         << "The modulo operator % cam be used on all numbers\n"
         << "An '!' placed after a value will calculate the factorial \n"
         << "- ex: 4! = 4 * 3 * 2 * 1\n"
         << "Square root and exponentiation are provided by 'sqrt' and 'pow'\n"
         << "- ex: sqrt(25) = 5, pow(5,2) = 25\n"
         << "Variable assignment is provided using the 'let' keyword:\n"
         << "- ex: let x = 37; x * 2 = 74; x = 4; x * 2 = 8\n\n";
}
void clean_up_mess()
{
    ts.ignore(print);
}
void calculate()
{
    while (cin)
        try {
            cout << prompt;
            Token t = ts.get();
            while (t.kind == print) t = ts.get();
            if (t.kind == help) print_help();
            else if (t.kind == quit) return;
            else {
                ts.putback(t);
                cout << result << statement() << '\n';
            }
        }
        catch (exception& e) {
            cerr << e.what() << '\n';
            clean_up_mess();
        }
}
int main()
try {
    st.define_name("pi", 3.1415926535, true);
    st.define_name("e", 2.7182818284, true);

    cout << "Simple Calculator (type ? for help)\n";

    calculate();
    return 0;
}
catch(exception& e) {
    cerr << "Exception: " << e.what() << '\n';
    return 1;
}
catch(...) {
    cerr << "Unknown exception\n";
    return 2;
}
