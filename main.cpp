#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <unordered_set>
#include <map>
using namespace std;
enum TokenType { // define all categories of Token
    LEFT_PAREN, //'('
    RIGHT_PAREN, //')'
    INT, // e.g., '123','+123','-123'
    STRING,
    DOT,
    FLOAT,
    NIL, //'nil' or '#f'
    T, //'t' or '#t'
    QUOTE, //'
    SYMBOL, //characters except above ,don't contain ',' ';' ' ' and case-sensitive
    PAIR,
    END_OF_FILE,
    PROCEDURE,
    ERROR_OBJ
};
enum ErrorType {
    NO_CLOSING_QUOTE,
    UNEXPECTED,
    NO_MORE_INPUT,            
    //evaluate error below
    UNBOUND_SYMBOL,
    INCORRECT_NUMBER,
    INCORRECT_ARGUMENT,
    NON_FUNCTION,
    NO_RETURN_VALUE,
    DIVISION_BY_ZERO,
    COND_FORMAT,
    DEFINE_FORMAT,
    NON_LIST,
    STRING_APPEND_INCORRECT_TYPE,
    CLEAN_ENVIRONMENT_LEVEL,
    DEFINE_LEVEL,
    EXIT_LEVEL,
    LET_FORMAT,
    LAMBDA_FORMAT,
    UNBOUND_PARAMETER,
    UNBOUND_CONDITION,
    UNBOUND_TEST_COND,
    SET_FORMAT
};
struct Token { // define the structure of Token
    TokenType type;
    string value;
    int line;
    int column;
};
struct Node {
    Token token;
    Node* left;
    Node* right;
    Node(Token t) {this->token = t; this->left = nullptr; this->right = nullptr;}
};
struct EvalErrorMsg {
    ErrorType errortype;
    string value;
    Node* crash_node;
    bool is_locked;
    EvalErrorMsg(ErrorType e, string v, Node* c= nullptr) {
        this->errortype = e;
        this->value = v;
        this->crash_node = c;
        this->is_locked = false;
    }
};
unordered_set<string> all_procedure = {
    "+", "-", "*", "/", "car", "cdr", "cons",
    "=", ">=", ">", "<=", "<", "define",
    "atom?", "pair?", "list?", "null?", "integer?", 
    "real?", "number?", "string?", "boolean?", 
    "not", "string-append", "string>?", "string<?", "string=?", 
    "eqv?", "equal?", "and", "or", "if", "cond", "clean-environment",
    "list", "quote", "symbol?", "begin", "exit",
    "let", "lambda", "verbose", "verbose?",
    "read", "write", "display-string", "newline", "eval", "set!", 
    "create-error-object", "error-object?", "symbol->string", "number->string"
};
class Environment {
  private:
    vector<map<string, Node*>> environment; // for define
  public:
    Environment() {
        PushFrame(); // make sure there always has environment[0]
    }
    void PushFrame(){
        map<string, Node*> map_empty;
        environment.push_back(map_empty);
    }
    void PopFrame(){
        if (environment.size() > 1) {
            environment.pop_back();
        }
    }
    Node* Lookup(string symbol){
        for (int i = environment.size() - 1; i >= 0; i--) {
            auto it = environment[i].find(symbol);
            if (it != environment[i].end()) {
                return it->second;
            }
        }
        return nullptr;
    }
    void DefineSymbol(string symbol, Node* value){
        environment.front()[symbol] = value;
    }
    void AddLocalBinding(string symbol, Node* value){
        environment.back()[symbol] = value;
    }
    void Clear() {
        environment.clear();
        PushFrame();
    }
    void ResetToGlobal() {
        while (environment.size() > 1) {
            environment.pop_back();
        }
    }
    vector<map<string, Node*>> GetFullEnv() { 
        return environment; 
    }
    void SetFullEnv(vector<map<string, Node*>>& env) { 
        environment = env; 
    }
    void Env_set(string symbol, Node* value) {
        for (int i = environment.size() - 1; i >= 0; i--) {
            auto it = environment[i].find(symbol);
            if (it != environment[i].end()) {
                it->second = value;
                return;
            }
        }
        DefineSymbol(symbol, value);
    }
};
class Printer {
  private:
    void INTrewrite(string value) {
        if (value[0] == '+') {
            value.erase(value.begin());
        }
        cout << value;
    }
    void FLOATrewrite(string value) {
        if (value[0] == '+') {
            value.erase(value.begin());
        }  
        if (value[0] == '-' && value[1] == '.') { //-.
            value.insert(value.begin() + 1, '0');
        } else if (value[0] == '.') {
            value.insert(value.begin(), '0');
        }
        double num = stod(value);
        cout << fixed << setprecision(3) << num;
    }
  public:
    void PrintSExp(Node* node, bool is_right_child, int n, bool is_error_type) {
        
        if (node == nullptr) return;
        if (node->token.type == QUOTE) {
            cout << node->token.value;
        } else if (node->token.type == NIL) {
            if (is_right_child) {
                cout << endl;
                for (int i = 0; i < n - 1; i++) {
                    cout << "  ";
                }
                cout << ")";
            } else {
                cout << "nil";
            }
        } else if (node->token.type != PAIR) { //atom
            if (is_right_child) { // dot
                cout << endl;
                for (int i = 0; i < n; i++) {
                    cout << "  ";
                }
                cout << "." << endl;
                for (int i = 0; i < n; i++) {
                    cout << "  ";
                }              
            }
            if (node->token.type == INT) {
                INTrewrite(node->token.value);
            } else if (node->token.type == FLOAT) {
                FLOATrewrite(node->token.value);
            } else if (node->token.type == T) {
                cout << "#t";
            } else if (node->token.type == PROCEDURE) {
                if (all_procedure.count(node->token.value) && !is_error_type) {
                    cout << "#<procedure " << node->token.value << ">";
                } else {
                    cout << node->token.value;
                }
            } else {
                cout << node->token.value;
            }
            if (is_right_child) {
                cout << endl;
                for (int i = 0; i < n - 1; i++) {
                    cout << "  ";
                }  
                cout << ")";          
            }            
        } else if (node->token.type == PAIR) { //either ( or )
            if (is_right_child) {
                cout << endl;
                for (int i = 0; i < n; i++) {
                    cout << "  ";
                }
                PrintSExp(node->left, false, n, is_error_type);
                PrintSExp(node->right, true, n, is_error_type);
            } else {
                if (node->left->token.value == "quote" || node->left->token.type == QUOTE) {
                    cout << "( quote" << endl;
                    for (int i = 0; i < n + 1; i++) {
                        cout << "  ";
                    }
                    PrintSExp(node->right->left, false, n + 1, is_error_type);
                    PrintSExp(node->right->right, true, n + 1, is_error_type);                    
                } else {
                    cout << "( ";
                    PrintSExp(node->left, false, n + 1, is_error_type);
                    PrintSExp(node->right, true, n + 1, is_error_type);
                }

            }
        }
    }
};
class Scanner {
  private:
    int currentline = 1;
    int currentcolumn = 1;
    bool has_token = false;
    Token token_now;
    bool is_newline = true;
    Token basic_GetToken() {
        while (true) {
            int token_peek = PeekChar();
            if (token_peek == EOF) {
                Token eof_token;
                eof_token.type = END_OF_FILE;
                eof_token.value = "EOF";
                return eof_token;
            }
            if (token_peek == ' ' || token_peek == '\t' || token_peek == '\n') { // process space
                GetChar();
            } else if (token_peek == ';') { // process comment
                int c = 0;
                while (c != EOF && c != '\n') {
                    c = GetChar();
                }
            } else { // process true char
                break;
            }
        }
        is_newline = false;
        /*processing space above*/
        /*processing all chars excepts space below*/
        Token token;
        token.line = currentline;
        token.column = currentcolumn;
        if (PeekChar() == '(') {
            GetChar();
            token.type = LEFT_PAREN;
            token.value = "(";
        } else if (PeekChar() == ')') {
            GetChar();
            token.type = RIGHT_PAREN;
            token.value = ")";            
        } else if (PeekChar() == 39) { //quote
            GetChar();
            token.type = QUOTE;
            token.value = "'";            
        } else if (PeekChar() == '"') { // first double-quote
            token.value += GetChar(); // value + double-quote
            while (PeekChar() != '"') { // second double-quote(processing string over)
                if (PeekChar() == EOF || PeekChar() == 10) { // no closing quote when it meets EOF
                    cout << "ERROR (no closing quote) : END-OF-LINE encountered at Line " << to_string(currentline) << " Column " << to_string(currentcolumn);
                    token.value.clear();
                    ErrorType errortype = NO_CLOSING_QUOTE;
                    throw errortype;
                } 
                if (PeekChar() == 92) { // backslash
                    GetChar(); // eat backslash
                    int peeked = PeekChar(); // peek backslash + 1
                    int get = GetChar(); // eat&get backslash + 1
                    if (peeked == 92) { // backslash, escape immediately
                        token.value += 92;
                    } else if (peeked == 'n') {
                        token.value += "\n";
                    } else if (peeked == 't') {
                        token.value += "\t";
                    } else if (peeked == 39) {
                        token.value += "\'";  
                    } else if (peeked == '"') {
                        token.value += "\"";
                    } else { // otherwise, put into string completely
                        token.value += 92;
                        token.value += get;
                    }
                } else {
                    token.value += GetChar(); // ignore other chars
                }
            } // while
            if (PeekChar() == '"') token.value += GetChar();
            token.type = STRING;
        } else { // Atom: int, float, symbol, dot, T, nil (no: (, ), ', ", \)
            while (true) { // catch atom until meet a separator
                int peeked = PeekChar();
                if (peeked == EOF || peeked == ' ' || peeked == 10
                    || peeked == '(' || peeked == ')' || peeked == 39
                    || peeked == '"' || peeked == ';') {
                        break;
                }
                token.value += GetChar();
            }
            // identify what atom it got
            token.type = IdentifyAtom(token.value);
        }
        return token;
    }
  public:
    int GetChar() {
        int c = cin.get();
        currentcolumn++;
        if (c == '\n') {
            if (is_newline) {
                is_newline = false;
                currentcolumn = 1;
                return c;
            }
            currentline++;
            currentcolumn = 1;
        }
        return c;
    }
    int PeekChar() {
        return cin.peek();
    }
    TokenType NumberChecker(string value, int i) {
        int dot_times = 0, number_times = 0;
        for (; i < value.size(); i++) {
            if (value[i] >= '0' && value[i] <= '9') {
                number_times++;
            } else if (value[i] == '.') {
                dot_times++;
            } else {
                return SYMBOL;
            }
        }
        if (dot_times > 1) { // two or more dots in string
            return SYMBOL;
        } else if (dot_times == 1 && number_times == 0) { // fake float e.g +. -.
            return SYMBOL;
        } else if (dot_times == 1) {
            return FLOAT;
        } else {
            return INT;
        }
    }
    TokenType IdentifyAtom(string value) {
        /*Reversed Words below*/
        if (value == ".") {
            return DOT;
        } else if (value == "nil" || value == "#f") {
            return NIL;
        } else if (value == "t" || value == "#t") {
            return T;
        }
        /*Reversed Words above*/
        /*Numbers below*/
        if (value[0] == '+' || value[0] == '-') {
            if (value.size() == 1) { // + or - appears alone
                return SYMBOL;
            } else {
                return NumberChecker(value, 1);
            }
        }
        return NumberChecker(value, 0);
    }
    Token PeekToken() {
        if (!has_token) {
            has_token = true;
            token_now = basic_GetToken();
        }
        return token_now;
    }
    Token GetToken() {
        if (has_token) {
            has_token = false;
            return token_now;
        } else {
            return basic_GetToken();
        }
    }
    void ResetLocation() {
        currentline = 1;
        currentcolumn = 1;
        is_newline = true;
        has_token = false;
    }
};
class Parser {
  private:
    Scanner scanner;
    Node* ReadList(bool is_top_level = false) {
        Token t_pair;
        t_pair.type = PAIR;
        t_pair.value = "pair";
        Token t_nil;
        t_nil.type = NIL;
        t_nil.value = "nil";
        Node* pair = new Node(t_pair);
        // peektoken
        if (scanner.PeekToken().type == RIGHT_PAREN) {
            scanner.GetToken();
            return new Node(t_nil); //pair->right
        } else if (scanner.PeekToken().type == DOT) {
            Token token = scanner.GetToken(); // eat
            if (is_top_level) {
                string err_msg = "ERROR (unexpected token) : atom or '(' expected when token at Line " + to_string(token.line)
                + " Column " + to_string(token.column) + " is >>" + token.value + "<<" ;
                throw err_msg;
            }
            Node* node_sexp = ReadSExp();
            if (scanner.PeekToken().type != RIGHT_PAREN) {
                Token err_token = scanner.GetToken();
                string err_msg =  "ERROR (unexpected token) : ')' expected when token at Line " + to_string(err_token.line)
                + " Column " + to_string(err_token.column) + " is >>" + err_token.value + "<<" ;
                throw err_msg;
            } else {
                scanner.GetToken();
                return node_sexp;  
            }
        } else {
            pair->left = ReadSExp();
            pair->right = ReadList();
        }      
        return pair;
    }

    public:
    void Reset() {
        scanner.ResetLocation();
    }
    Node* ReadSExp(bool is_top_level = false) {
        Token token = scanner.GetToken(); // get a clean token
        if (token.type == END_OF_FILE) {
            if (is_top_level) { // it's totally over if it meets EOF in main
                ErrorType errortype = NO_MORE_INPUT;
                throw errortype;
            }
        }
        if (token.type == SYMBOL || token.type == INT || token.type == FLOAT
        || token.type == STRING || token.type == NIL || token.type == T) { // processing all atoms excepts ()
            Node* node = new Node(token);
            return node;
        } else if (token.type == QUOTE) {
            Token token_quote;
            token_quote.value = "quote";
            token_quote.type = QUOTE;
            Node* quote = new Node(token_quote); // root->left
            Node* inner_exp = ReadSExp(); // root->right->left
            Token token_nil;
            token_nil.value = "nil";
            token_nil.type = NIL;
            Node* nil = new Node(token_nil); // root->right->right
            Token token_pair2;
            token_pair2.value = "pair2";
            token_pair2.type = PAIR;
            Node* pair2 = new Node(token_pair2); // root->right
            pair2->left = inner_exp;
            pair2->right = nil;
            Token token_pair;
            token_pair.value = "pair";
            token_pair.type = PAIR;
            Node* pair = new Node(token_pair); // root  
            pair->left = quote;
            pair->right = pair2;
            return pair;
        } else if (token.type == LEFT_PAREN) {
            Node* list = ReadList(true);
            return list;
        } else { // ERROR: ), ., EOF
             ErrorType errortype = UNEXPECTED;
            if (token.value != "EOF") {
                string err_msg = "ERROR (unexpected token) : atom or '(' expected when token at Line " + to_string(token.line)
                + " Column " + to_string(token.column) + " is >>" + token.value + "<<" ;
                throw err_msg;
            } else {
                errortype = NO_MORE_INPUT;
            }
           
            throw errortype;
        }
    }


};
class Evaluator {
  private:
    int level = 0;
    Environment environment;
    bool is_verbose = true;
    Node* EvalArgument(Node* node) {
        try {
            return EvalSExp(node);
        } catch (EvalErrorMsg& e) {
            if (e.errortype == NO_RETURN_VALUE && (e.crash_node == node || node->token.type != PAIR)) {
                e.errortype = UNBOUND_PARAMETER;
                e.crash_node = node;
            }
            throw e; 
        }
    }    

    Node* EvalCondition(Node* node) {
        try {
            return EvalSExp(node);
        } catch (EvalErrorMsg& e) {
            if (e.errortype == NO_RETURN_VALUE && (e.crash_node == node || node->token.type != PAIR)) {
                e.errortype = UNBOUND_CONDITION;
                e.crash_node = node;
            }
            throw e; 
        }
    }

    Node* EvalTestCondition(Node* node) {
        try {
            return EvalSExp(node);
        } catch (EvalErrorMsg& e) {
            if (e.errortype == NO_RETURN_VALUE && (e.crash_node == node || node->token.type != PAIR)) {
                e.errortype = UNBOUND_TEST_COND;
                e.crash_node = node;
            }
            throw e; 
        }
    }
    Node* EvaluateMath(string operand, Node* node) { // + - * /
        double sum = 0;
        int arg_count = 0;
        bool is_float = 0;
        Node* head = node;
        node = node->right;
        while (node->token.type != NIL) { // check number of argument
            node = node->right;
            arg_count++;
        }
        if (arg_count < 2) { // e.g(+) (+ 1)
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, head->left->token.value);
            throw evalerrormsg;   
        } 
        node = head;
        node = node->right;
        arg_count = 0;
        while (node->token.type != NIL) {
            Node* number_node = EvalArgument(node->left);
            string value = number_node->token.value;
            if (number_node->token.type == FLOAT || number_node->token.type == INT) {
                if (number_node->token.type == FLOAT) {
                    is_float = true;
                }
                if (arg_count == 0) {
                    sum = stod(value);
                } else {
                    if (operand == "+") {
                        sum += stod(value);
                    } else if (operand == "-") {
                        sum -= stod(value);
                    } else if (operand == "*") {
                        sum *= stod(value);
                    } else if (operand == "/") {
                        if (value == "0") {
                            EvalErrorMsg evalerrormsg = EvalErrorMsg(DIVISION_BY_ZERO, operand); //> ERROR (+ with incorrect argument type) : #<procedure ->
                            throw evalerrormsg;                            
                        } else {
                            sum /= stod(value);
                        }
                    }
                }
            } else { // incorrect argument
                EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_ARGUMENT, operand, number_node); //> ERROR (+ with incorrect argument type) : #<procedure ->
                throw evalerrormsg;
            }
            node = node->right;
            arg_count++;
        }
        Token token;
        
        if (is_float) {
            token.type = FLOAT;
            token.value = to_string(sum);
        } else {
            token.type = INT;
            token.value = to_string((int)sum);
        }
        Node* math_node = new Node(token);
        return math_node;   
    }
    Node* EvaluateCompare(string comparison, Node* node) { // = > < >= <=
        double pre_num = 0;
        int arg_count = 0;
        bool is_match = true;
        Node* head = node;
        node = node->right;
        while (node->token.type != NIL) { // check number of argument
            node = node->right;
            arg_count++;
        }
        if (arg_count < 2) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, head->left->token.value);
            throw evalerrormsg;   
        } 
        node = head;
        node = node->right;
        arg_count = 0;
        while (node->token.type != NIL) {
            Node* number_node = EvalArgument(node->left);
            string value = number_node->token.value;
            if (number_node->token.type == FLOAT || number_node->token.type == INT) {
                if (arg_count == 0) {
                    pre_num = stod(value);
                } else {
                    if (comparison == "=") {
                        if (pre_num != stod(value)) {
                            is_match = false;
                        }
                    } else if (comparison == ">") {
                        if (pre_num <= stod(value)) {
                            is_match = false;
                        }
                    } else if (comparison == ">=") {
                        if (pre_num < stod(value)) {
                            is_match = false;
                        }                        
                    } else if (comparison == "<") {
                        if (pre_num >= stod(value)) {
                            is_match = false;
                        }                        
                    } else if (comparison == "<=") {   
                        if (pre_num > stod(value)) {
                            is_match = false;
                        }                           
                    }
                    pre_num = stod(value);                     
                }
            } else { // incorrect argument
                EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_ARGUMENT, comparison, number_node); 
                throw evalerrormsg;
            }
            node = node->right;
            arg_count++;
        }
        Token match_t;
        if (is_match) {
            match_t.type = T;
            match_t.value = "#t";
        } else {
            match_t.type = NIL;
            match_t.value = "nil";
        }
        Node* match_node = new Node(match_t);
        return match_node; 
    }
    Node* EvaluateStringCompare(string comparison, Node* node) { // string=? string>? string<? =
        string pre_str;
        int arg_count = 0;
        bool is_match = true;
        Node* head = node;
        node = node->right;
        while (node->token.type != NIL) { // check number of argument
            node = node->right;
            arg_count++;
        }
        if (arg_count < 2) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, head->left->token.value);
            throw evalerrormsg;   
        } 
        node = head;
        node = node->right;
        arg_count = 0;
        while (node->token.type != NIL) {
            Node* number_node = EvalArgument(node->left);
            string value = number_node->token.value;
            if (number_node->token.type == STRING) {
                if (arg_count == 0) {
                    pre_str = value;
                } else {
                    if (comparison == "string=?") {
                        if (pre_str != value) {
                            is_match = false;
                        }
                    } else if (comparison == "string>?") {
                        if (pre_str <= value) {
                            is_match = false;
                        }
                    } else if (comparison == "string<?") {
                        if (pre_str >= value) {
                            is_match = false;
                        }                        
                    }
                    pre_str = value;                     
                }
            } else { // incorrect argument
                EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_ARGUMENT, comparison, number_node); 
                throw evalerrormsg;
            }
            node = node->right;
            arg_count++;
        }
        Token match_t;
        if (is_match) {
            match_t.type = T;
            match_t.value = "#t";
        } else {
            match_t.type = NIL;
            match_t.value = "nil";
        }
        Node* match_node = new Node(match_t);
        return match_node; 
    }
    Node* Evaluate_car_cdr(string str, Node* node) { // car cdr
        if (node->right->token.type == NIL) { // e.g(cdr)
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, str);
            throw evalerrormsg;                          
        }
        if (node->right->right->token.type == PAIR) { // e.g(cdr '(1 2) 3) there are extra nodes in the right of node
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, str);
            throw evalerrormsg;   
        }
        Node* evaluated_node = EvalArgument(node->right->left);

        if (evaluated_node->token.type != PAIR) { // e.g(cdr 3) there is no more node in the left of node
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_ARGUMENT, str, evaluated_node);
            throw evalerrormsg;                       
        }
        return evaluated_node;
    } 
    Node* EvaluateIdentity(string identity, Node* node) { // atom?, pair?, list?, null?, integer?, real?, number?, string?, boolean?
        int arg_count = 0;
        bool is_true = true;
        Node* head = node;
        node = node->right;
        while (true) {
            if (node == nullptr && identity == "list?") {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_LIST, identity, head);
                throw evalerrormsg;                 
            }
            if (node->token.type == NIL) {
                break;
            }
            arg_count++;
            node = node->right;
        }
        if (arg_count != 1) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, head->left->token.value);
            throw evalerrormsg;   
        } 
        node = head;
        node = node->right; 
        Node* identity_node = EvalArgument(node->left);
        if (identity == "atom?") {
            if (identity_node->token.type == PAIR) {
                is_true = false;
            }
        } else if (identity == "pair?") { // if true: e.g(pair? '(1))
            if (identity_node->token.type != PAIR) {
                is_true = false;
            }
        } else if (identity == "list?") {
            
            while (true) {
                if (identity_node == nullptr) { //e.g(list? (cons 1 2))
                    is_true = false;
                    break;
                }
                if (identity_node->token.type == NIL) {
                    break;
                }
                identity_node = identity_node->right;
            }
        } else if (identity == "null?") { // meet nil
            if (identity_node->token.type != NIL) {
                is_true = false;
            }
        } else if (identity == "integer?") {
            if (identity_node->token.type != INT) {
                is_true = false;
            }
        } else if (identity == "number?" || identity == "real?") {
            if (!(identity_node->token.type == INT || identity_node->token.type == FLOAT)) { // only INT or FLOAT can be true
                is_true = false;
            }
        } else if (identity == "string?") {
            if (identity_node->token.type != STRING) {
                is_true = false;
            }
        } else if (identity == "boolean?") {
            if (!(identity_node->token.type == NIL || identity_node->token.type == T)) { // only NIL or T can be true
                is_true = false;
            }            
        } else if (identity == "not") { 
            if (identity_node->token.type != NIL) { // when it meets true, return false. otherwise, return true when it meets false(neg neg true)
                is_true = false;
            }            
        } else if (identity == "symbol?") {
            if (identity_node->token.type != SYMBOL) { // when it meets true, return false. otherwise, return true when it meets false(neg neg true)
                is_true = false;
            }                
        } else if (identity == "error-object?") {
            if (identity_node->token.type != ERROR_OBJ) { // when it meets true, return false. otherwise, return true when it meets false(neg neg true)
                is_true = false;
            }  
        }
        Token token;
        if (is_true) {
            token.type = T;
            token.value = "#t";
        } else {
            token.type = NIL;
            token.value = "nil";
        }
        Node* res_node = new Node(token);
        return res_node;         
    }
    Node* Evaluate_cons(Node* node) {
        if (node->right->token.type == NIL) { // e.g(cons) avoid 0 argument
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, "cons");
            throw evalerrormsg;                          
        }
        if (node->right->right->token.type == NIL) { // e.g(cons 3) avoid 1 argument
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, "cons");
            throw evalerrormsg;                        
        }
        if (node->right->right->right->token.type != NIL) { // e.g(cons 3 4 5) avoid over 3 argument
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, "cons");
            throw evalerrormsg;                          
        }
        Node* car_node = EvalArgument(node->right->left);
        Node* cdr_node = EvalArgument(node->right->right->left);
        Token t_pair;
        t_pair.type = PAIR;
        t_pair.value = "PAIR";
        Node* n_pair = new Node(t_pair);
        n_pair->left = car_node;
        n_pair->right = cdr_node;
        return n_pair;
    }
    Node* Evaluate_if(Node* node) {
        int arg_count = 0;
        bool is_true = true;
        Node* head = node;
        node = node->right;
        while (node->token.type != NIL) { // check number of argument
            node = node->right;
            arg_count++;
        }
        if (arg_count < 2 || arg_count > 3) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, head->left->token.value);
            throw evalerrormsg;   
        } 
        node = head;
        node = node->right; 
        Node* first_arg = EvalTestCondition(node->left); // to get true or false
        bool cond = true; // default: #t
        if (first_arg->token.type == NIL) {
            cond = false;
        } 
        node = node->right; 
        Node* after_first;
        if (cond) { // #t return first one
            after_first = EvalSExp(node->left);
        } else { // nil return second one
            // error
            if (node->right->token.type == NIL) { // only an argument
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NO_RETURN_VALUE, head->left->token.value, head);
                throw evalerrormsg; 
            } else {
                after_first = EvalSExp(node->right->left);
            }
            
        }
        return after_first;
    }
    Node* Evaluate_and_or(string str, Node* node) {
        int arg_count = 0;
        Node* head = node;
        node = node->right;
        while (node->token.type != NIL) { // check number of argument
            node = node->right;
            arg_count++;
        }
        if (arg_count < 2) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, head->left->token.value);
            throw evalerrormsg;   
        }
        node = head;
        node = node->right; 
        Node* arg_node = nullptr;

        while (node->token.type != NIL) {
            arg_node = EvalCondition(node->left); // check if true or false
            if (str == "and") {
                if (arg_node->token.type == NIL) {
                    return arg_node;
                }
            } else if (str == "or") {
                if (arg_node->token.type != NIL) {
                    return arg_node;
                }
            }
            node = node->right;
        }
        return arg_node;        

    }
    Node* Evaluate_cond(Node* node) {
        if (node->right->token.type == NIL) { // e.g(cond)
            EvalErrorMsg evalerrormsg = EvalErrorMsg(COND_FORMAT, "cond", node);
            throw evalerrormsg;
        }
        bool have_gotten = false; // if res_node gets a node that should be return, it would be true to make sure other nodes can't overwrite it
        Node* head = node;
        node = node->right;
        while (node->token.type != NIL) {
            Node* car_node = node->left;
            if (car_node->token.type != PAIR) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(COND_FORMAT, "cond", head);
                throw evalerrormsg;                   
            }
            
            int i = 0;
            while (car_node->token.type != NIL) {
                i++;
                car_node = car_node->right;
            }
            if (i < 2) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(COND_FORMAT, "cond", head);
                throw evalerrormsg;                
            }
            node = node->right;
        }
        node = head->right;
        Node* res_node = nullptr;
        while (node->token.type != NIL) { 
            if (node->left->token.type != PAIR) { // list is empty or no list
                EvalErrorMsg evalerrormsg = EvalErrorMsg(COND_FORMAT, "cond", head);
                throw evalerrormsg;                    
            }
            if (!have_gotten) { 
                if (node->left->left->token.value == "else") { 
                    bool is_true = false;
                    
                    if (node->right->token.type == NIL) { 
                        is_true = true;
                    } else {
                        Node* lookup_node = environment.Lookup("else"); 
                        if (lookup_node != nullptr) {
                            if (lookup_node->token.type != NIL) {
                                is_true = true; 
                            }
                        } else {
                            EvalErrorMsg evalerrormsg = EvalErrorMsg(UNBOUND_SYMBOL, "else");
                            throw evalerrormsg; 
                        }
                    }

                    if (is_true) { 
                        Node* traverse_node = node->left->right;
                        if (traverse_node->token.type == NIL) { 
                            EvalErrorMsg evalerrormsg = EvalErrorMsg(COND_FORMAT, "cond", head);
                            throw evalerrormsg;
                        }
                        Node* last_node = nullptr;
                        while (traverse_node->token.type != NIL) { 
                            try { 
                                last_node = EvalSExp(traverse_node->left);
                            } catch (EvalErrorMsg& e) {
                                if (e.errortype == NO_RETURN_VALUE && traverse_node->right->token.type != NIL) {
    
                                } else {
                                    throw e;
                                }
                            }
                            if (traverse_node == nullptr) { 
                                EvalErrorMsg evalerrormsg = EvalErrorMsg(COND_FORMAT, "cond", head);
                                throw evalerrormsg;                                
                            } 
                            traverse_node = traverse_node->right;
                        }
                        res_node = last_node;
                        have_gotten = true;
                    }
                    
                } else {
                    Node* t_f_node = EvalTestCondition(node->left->left); // the node stores true or false
                    if (t_f_node->token.type != NIL) { // only return first true node, so set a boolean to avoid other true nodes overwrite it
                        Node* traverse_node = node->left->right;
                        if (traverse_node->token.type == NIL) { 
                            EvalErrorMsg evalerrormsg = EvalErrorMsg(COND_FORMAT, "cond", head);
                            throw evalerrormsg;
                        }
                        Node* last_node = nullptr;
                        while (traverse_node->token.type != NIL) { 
                            try { 
                                last_node = EvalSExp(traverse_node->left);
                            } catch (EvalErrorMsg& e) {
                                if (e.errortype == NO_RETURN_VALUE && traverse_node->right->token.type != NIL) {
                                } else {
                                    throw e;
                                }
                            }
                            if (traverse_node == nullptr) { 
                                EvalErrorMsg evalerrormsg = EvalErrorMsg(COND_FORMAT, "cond", head);
                                throw evalerrormsg;                                
                            } 
                            traverse_node = traverse_node->right;
                        }
                        res_node = last_node;
                        have_gotten = true;
                        
                    } 
                }
            }
            node = node->right;
        }
        if (res_node == nullptr) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(NO_RETURN_VALUE, head->left->token.value, head);
            throw evalerrormsg;              
        }
        return res_node;
        
    }
    Node* Evaluate_eqv_equal(string str, Node* node) {
        int arg_count = 0;
        Node* head = node;
        node = node->right;
        while (node->token.type != NIL) {
            arg_count++;
            node = node->right;
        }
        if (arg_count != 2) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, str);
            throw evalerrormsg;
        } 
        /*declare in advance*/
        Token token_true;
        token_true.type = T;
        token_true.value = "#t";
        Node* true_node = new Node(token_true);
        Token token_false;
        token_false.type = NIL;
        token_false.value = "nil";
        Node* false_node = new Node(token_false); 
        /*declare in advance*/       
        Node* first_arg = EvalArgument(head->right->left);
        Node* sec_arg = EvalArgument(head->right->right->left);
        if (first_arg == sec_arg) { // share same memory space
            return true_node;
        }
        if (first_arg->token.type != sec_arg->token.type) {
            return false_node;
        }
        if (str == "eqv?") {
            if (first_arg->token.type == STRING || first_arg->token.type == PAIR) { // same value excepts string and pair
                return false_node;
            } else if (first_arg->token.value != sec_arg->token.value) { 
                return false_node;
            } else { // same value
                return true_node;
            }
        } else { // equal?
            if (is_Equal(first_arg, sec_arg)) {
                return true_node;
            } else {
                return false_node;
            }         
        }
    
        return false_node;
    }
    Node* EvaluateStringAppend(Node* node) {
        int arg_count = 0;
        Node* head = node;
        node = node->right;
        while (node->token.type != NIL) {
            arg_count++;
            node = node->right;
        }
        if (arg_count < 2) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, "string-append");
            throw evalerrormsg;
        }        
        node = head->right;
        string str;
        arg_count = 0;
        while(node->token.type != NIL) {
            Node* arg_node = EvalArgument(node->left);
            if (arg_node->token.type == STRING) {
                string temp_str = arg_node->token.value;
                if (arg_count == 0) { // for erase extra double-quote
                    temp_str.pop_back();
                } else {
                    temp_str.pop_back();
                    temp_str.erase(temp_str.begin());
                }
                str += temp_str;
            } else {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(STRING_APPEND_INCORRECT_TYPE, arg_node->token.value);
                throw evalerrormsg;                
            }
            arg_count++;
            node = node->right;
        }
        str.push_back('"'); // put double-quote back to the end of string
        Token token;
        token.type = STRING;
        token.value = str;
        Node* res = new Node(token);
        return res;
    }
    bool is_Equal(Node* node1, Node* node2) {
        if (node1 == node2) { // avoid two nodes meet nullptr at the same time
            return true;
        }
        if (node1 == nullptr || node2 == nullptr) { // it happens when a node has already meet nil but another one hasn't
            return false;
        }
        if (node1->token.type != node2->token.type) {
            return false;
        }
        if (node1->token.type == PAIR) { // check list
            bool left_equal = is_Equal(node1->left, node2->left); 
            bool right_equal = is_Equal(node1->right, node2->right);
            return left_equal && right_equal;
        }
        return node1->token.value == node2->token.value; // they are absolutely atoms
        
    }
    Node* Evaluate_define(Node* node) {
        int arg_count = 0;
        Node* head = node;
        node = node->right;
        while (true) {
            if (node == nullptr) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_LIST, "define", head);
                throw evalerrormsg;                 
            }
            if (node->token.type == NIL) {
                break;
            }
            arg_count++;
            node = node->right;
        }
        if (arg_count == 0) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(DEFINE_FORMAT, "define", head);
            throw evalerrormsg;  
        }
        node = head->right;
        string str;
        Node* arg_node;
        Node* body;
        if (node->left->token.type == SYMBOL) {
            if (arg_count != 2) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(DEFINE_FORMAT, "define", head);
                throw evalerrormsg;            
            }      
            if (all_procedure.count(node->left->token.value)) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(DEFINE_FORMAT, "define", head);
                throw evalerrormsg;  
            } 
            Node* arg_node = nullptr;
            try {
                arg_node = EvalSExp(node->right->left);
            } catch (EvalErrorMsg& e) {
                if (e.errortype == NO_RETURN_VALUE && !e.is_locked) {
                    e.crash_node = node->right->left;
                    e.is_locked = true; 
                }
                throw e;
            }
            environment.DefineSymbol(node->left->token.value, arg_node); 
            if (!is_verbose) {
                return nullptr;
            }
            str = node->left->token.value + " defined";
            Token token;     
            token.value = str; 
            token.type = SYMBOL;
            Node* defined_cout = new Node(token);
            return defined_cout;
        } else if (node->left->token.type == PAIR) {
            if (arg_count < 2) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(DEFINE_FORMAT, "define", head);
                throw evalerrormsg;                    
            }
            Node* func = node->left->left;
            if (func->token.type != SYMBOL) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(DEFINE_FORMAT, "define", head);
                throw evalerrormsg;              
            }
            if (all_procedure.count(func->token.value)) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(DEFINE_FORMAT, "define", head);
                throw evalerrormsg;  
            } 
            arg_node = node->left->right;
            Node* arg_node_for_check = arg_node;
            body = node->right;
            while (true) {
                if (arg_node_for_check->token.type == NIL) {
                    break;
                }
                if (arg_node_for_check == nullptr) {
                    EvalErrorMsg evalerrormsg = EvalErrorMsg(DEFINE_FORMAT, "define", head);
                    throw evalerrormsg;                  
                }
                if (arg_node_for_check->token.type != PAIR) {
                    EvalErrorMsg evalerrormsg = EvalErrorMsg(DEFINE_FORMAT, "define", head);
                    throw evalerrormsg;                      
                }
                if (arg_node_for_check->left->token.type != SYMBOL) {
                    EvalErrorMsg evalerrormsg = EvalErrorMsg(DEFINE_FORMAT, "define", head);
                    throw evalerrormsg;  
                }
                arg_node_for_check = arg_node_for_check->right;
            }
            Token token;
            token.type = PROCEDURE;
            token.value = "#<procedure " + func->token.value + ">";
            Node* procedure_node = new Node(token);
            procedure_node->left = arg_node;
            procedure_node->right = body;
            environment.DefineSymbol(func->token.value, procedure_node);
            environment.DefineSymbol(func->token.value, procedure_node);
            if (!is_verbose) {
                return nullptr;
            }
            string str = func->token.value + " defined";
            Token cout_token;
            cout_token.type = SYMBOL;
            cout_token.value = str;
            Node* defined_cout = new Node(cout_token);
            return defined_cout;            
        } else {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(DEFINE_FORMAT, "define", head);
            throw evalerrormsg;              
        }
        return nullptr;
    }
    Node* Evaluate_list(Node* node) {
        if (node->right->token.type == NIL) {
            Token token;
            token.type = NIL;
            token.value = "nil";
            Node* n = new Node(token);
            return n;
        }
        Node* head = node;
        node = node->right;
        Token token;
        token.type = PAIR;
        token.value = "PAIR";
        Node* res_node = new Node(token);
        Node* res_head = res_node;
        Node* pre = nullptr;
        while (true) {
            if (node->token.type == NIL) {
                break;
            }
            if (node->token.type != PAIR) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_LIST, "list", head);
                throw evalerrormsg;     
            }
            res_node->left = EvalArgument(node->left);
            res_node->right = new Node(token);
            node = node->right;
            pre = res_node;
            res_node = res_node->right;
        }
        Token token_nil;
        token_nil.type = NIL;
        token_nil.value = "nil";
        pre->right = new Node(token_nil);
        return res_head;
    }
    Node* Evaluate_begin(Node* node) {
        if (node->right->token.type == NIL) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, "begin");
            throw evalerrormsg;            
        }
        Node* head = node;
        Node* res_node = nullptr;
        while (true) {
            if (node->token.type == NIL) break;
            if (node->token.type != PAIR) { 
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_LIST, "list", head);
                throw evalerrormsg;    
            }
            try {
                res_node = EvalSExp(node->left);
            } catch (EvalErrorMsg& e) {
                if (e.errortype == NO_RETURN_VALUE && node->right->token.type != NIL) {

                } else {
                    throw e;
                }
            }
            node = node->right;
        }
        return res_node;
    }
    bool is_let_legal(Node* node) {
        node = node->left;
        while (true) {
            if (node == nullptr) {
                return false;         
            }
            if (node->token.type == NIL) {
                break;
            }
            if (node->token.type != PAIR) {
                return false;
            }              
            Node* inside_node = node->left;
            int inside_arg = 0;
            if (inside_node->token.type != PAIR) {
                return false;
            }
            if (inside_node->left->token.type != SYMBOL) {
                return false;
            }    
            if (all_procedure.count(inside_node->left->token.value)) {
                return false;
            }

            while (true) {
                if (inside_node == nullptr) {
                    return false;         
                }
                if (inside_node->token.type == NIL) {
                    break;
                }

                inside_arg++;
                inside_node = inside_node->right;
            }
            if (inside_arg != 2) {
                return false;
            }
            node = node->right;
        } 
        return true;  
    }
    Node* Evaluate_let(Node* node) { // form: (let ((var1 val1) (var2 val2) ...) body...)
        int arg_count = 0;
        Node* head = node;
        node = node->right;
        while (true) {
            if (node == nullptr) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_LIST, "let", head);
                throw evalerrormsg;                 
            }
            if (node->token.type == NIL) {
                break;
            }
            if (arg_count == 0) {
                if (!is_let_legal(node)) {
                    EvalErrorMsg evalerrormsg = EvalErrorMsg(LET_FORMAT, "let", head);
                    throw evalerrormsg;     
                }               
            }
            arg_count++;
            node = node->right;
        } 
        if (arg_count < 2) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(LET_FORMAT, "let", head);
            throw evalerrormsg;            
        }      
        vector<pair<string, Node*>> temp_binding; 
        Node* bind_list = head->right->left; // start
        while (bind_list->token.type != NIL) {
            string var_name = bind_list->left->left->token.value;
            Node* exp_node = bind_list->left->right->left;  
            Node* val_node = nullptr;
            try {
                val_node = EvalSExp(exp_node); 
            } catch (EvalErrorMsg& e) {
                if (e.errortype == NO_RETURN_VALUE && !e.is_locked) {
                    e.crash_node = exp_node;
                    e.is_locked = true; 
                }
                throw e;
            }
            temp_binding.push_back({var_name, val_node});
            bind_list = bind_list->right;
        }
        environment.PushFrame();
        for (auto temp : temp_binding) {
            environment.AddLocalBinding(temp.first, temp.second);
        }
        Node* body_node = head->right->right;
        Node* result = nullptr;
        while (body_node->token.type != NIL) {
            try {
                result = EvalSExp(body_node->left);
            } catch (EvalErrorMsg& e) {
                if (e.errortype == NO_RETURN_VALUE && body_node->right->token.type != NIL) {
                } else {
                    environment.PopFrame();
                    throw e;
                }
            }
            body_node = body_node->right;
        }
        environment.PopFrame();
        return result;
    }
    bool is_Lambda_legal(Node* node) {
        node = node->left;
        while (true) {
            if (node == nullptr) {
                return false;         
            }
            if (node->token.type == NIL) {
                break;
            }
            if (node->token.type != PAIR) {
                return false;
            }
            if (node->left->token.type != SYMBOL) {
                return false;
            }            
            node = node->right;
        } 
        return true;  
    }    
    Node* Evaluate_Lambda(Node* node) {
        int arg_count = 0;
        Node* head = node;
        node = node->right;
        while (true) {
            if (node == nullptr) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_LIST, "lambda", head);
                throw evalerrormsg;                 
            }
            if (node->token.type == NIL) {
                break;
            }
            if (arg_count == 0) {
                if (!is_Lambda_legal(node)) {
                    EvalErrorMsg evalerrormsg = EvalErrorMsg(LAMBDA_FORMAT, "lambda", head);
                    throw evalerrormsg;     
                }               
            }
            arg_count++;
            node = node->right;
        } 
        if (arg_count < 2) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(LAMBDA_FORMAT, "lambda", head);
            throw evalerrormsg;            
        } 
        Token token;
        token.type = PROCEDURE;
        token.value = "#<procedure lambda>";
        Node* procedure_node = new Node(token);
        procedure_node->left = head->right->left;
        procedure_node->right = head->right->right;
        return procedure_node;          
    }
    void RestoreCallerEnvButKeepGlobal(vector<map<string, Node*>>& backup_env) {
        vector<map<string, Node*>> current_env = environment.GetFullEnv();
        if (!backup_env.empty() && !current_env.empty()) {
            backup_env[0] = current_env[0];  
        }
        environment.SetFullEnv(backup_env);
    }
    Node* Apply_Procedure(Node* op_node, Node* args_node) {
        Node* params = op_node->left;
        Node* body = op_node->right;
        Node* actual_args = args_node->right; 
        int expected_arg_cnt = 0;  
        Node* count_node = params;
        while (count_node != nullptr && count_node->token.type != NIL) {  
            expected_arg_cnt++;
            count_node = count_node->right;
        }
        int actual_arg_cnt = 0;
        Node* arg_count_node = actual_args;
        while (arg_count_node != nullptr && arg_count_node->token.type != NIL) {
            actual_arg_cnt++;
            arg_count_node = arg_count_node->right;
        }
        if (expected_arg_cnt != actual_arg_cnt) {
            string err_name;
            if (args_node->left->token.type == PAIR && args_node->left->left->token.value == "lambda") {
                err_name = "lambda"; 
            } else {
                err_name = op_node->token.value; 
                if (err_name.find("#<procedure ") == 0) {
                    err_name = err_name.substr(12, err_name.length() - 13);
                }
            }
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, err_name, args_node);
            throw evalerrormsg;                
        }
        Node* current_arg = actual_args;
        vector<Node*> eval_args; 
        while (current_arg->token.type != NIL) {
            Node* val_node = EvalArgument(current_arg->left); 
            eval_args.push_back(val_node);              
            current_arg = current_arg->right;
        }
        vector<map<string, Node*>> backup_env = environment.GetFullEnv();
        vector<map<string, Node*>> global_only;
        if (backup_env.size() > 0) {
            global_only.push_back(backup_env[0]); 
        }
        environment.SetFullEnv(global_only);        
        environment.PushFrame();
        Node* current_param = params; 
        int i = 0;
        while (current_param->token.type != NIL) {
            string var_name = current_param->left->token.value; 
            environment.AddLocalBinding(var_name, eval_args[i]);
            current_param = current_param->right;
            i++;
        }
        
        Node* result = nullptr;
        while (body->token.type != NIL) {
            try { 
                result = EvalSExp(body->left);
            } catch (EvalErrorMsg& e) {
                if (e.errortype == NO_RETURN_VALUE && body->right->token.type != NIL) {
                } else {
                    RestoreCallerEnvButKeepGlobal(backup_env);
                    throw e; 
                }
            }
            body = body->right;
        }
        RestoreCallerEnvButKeepGlobal(backup_env);
        return result;
    }
    Node* Evaluate_verbose(Node* node) {
        Node* head = node;
        node = node->right;
        int arg_count = 0;
        Node* arg_expr = nullptr;
        while (true) {
            if (node == nullptr) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_LIST, "verbose", head);
                throw evalerrormsg;                 
            }
            if (node->token.type == NIL) {
                break;
            }
            if (arg_count == 0) {
                arg_expr = node->left;
            } else {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, "verbose", head);
                throw evalerrormsg;     
            }
            arg_count++;
            node = node->right;
        }
        if (arg_count != 1) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, "verbose", head);
            throw evalerrormsg; 
        }
        Node* val = EvalArgument(arg_expr);
        if (val->token.type == NIL) {
            is_verbose = false;
        } else {
            is_verbose = true;
        }
        Token token;
        if (is_verbose) {
            token.type = T;
            token.value = "#t";
        } else {
            token.type = NIL;
            token.value = "nil";
        }
            return new Node(token);
    }
    Node* Evaluate_Isverbose() {
        Token token;
        if (is_verbose == true) {
            token.type = T;
            token.value = "#t";
            Node* res = new Node(token);
            return res;
        } else {
            token.type = NIL;
            token.value = "nil";
            Node* res = new Node(token);
            return res;
        }
        return nullptr;
    }
    Node* Evaluate_Create_Error_Object(Node* node) {
        int arg_count = 0;
        Node* head = node;
        node = node->right;
        while (true) {
            if (node == nullptr) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_LIST, "create-error-object", head);
                throw evalerrormsg;                 
            }
            if (node->token.type == NIL) {
                break;
            }
            arg_count++;
            node = node->right;
        } 
        if (arg_count != 1) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, "create-error-object", head);
            throw evalerrormsg;            
        } 
        Node* arg_node = EvalArgument(head->right->left);
        if (arg_node->token.type != STRING) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_ARGUMENT, "create-error-object", arg_node);
            throw evalerrormsg;            
        }
        Token err_token;
        err_token.type = ERROR_OBJ;
        err_token.value = arg_node->token.value;
        Node* err_node = new Node(err_token);
        return err_node;
    }
    Node* Evaluate_Write(Node* node) {
        int arg_count = 0;
        Node* head = node;
        node = node->right;
        while (true) {
            if (node == nullptr) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_LIST, "wrtie", head);
                throw evalerrormsg;                 
            }
            if (node->token.type == NIL) {
                break;
            }
            arg_count++;
            node = node->right;
        } 
        if (arg_count != 1) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, "wrtie", head);
            throw evalerrormsg;            
        }    
        Printer printer;
        Node* processed_node = EvalSExp(head->right->left);
        printer.PrintSExp(processed_node, false, 0, false); 
        return processed_node;   
    }
    Node* Evaluate_Display_String(Node* node) {
        int arg_count = 0;
        Node* head = node;
        node = node->right;
        while (true) {
            if (node == nullptr) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_LIST, "display-string", head);
                throw evalerrormsg;                 
            }
            if (node->token.type == NIL) {
                break;
            }
            arg_count++;
            node = node->right;
        } 
        if (arg_count != 1) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, "display-string", head);
            throw evalerrormsg;            
        }  
        Node* processed_node = EvalSExp(head->right->left);
        if (processed_node->token.type == STRING || processed_node->token.type == ERROR_OBJ) {
            Printer printer;  
            processed_node->token.value.erase(processed_node->token.value.begin());
            processed_node->token.value.pop_back();
            printer.PrintSExp(processed_node, false, 0, false); 

            // recover
            processed_node->token.value.insert(processed_node->token.value.begin(), '"');
            processed_node->token.value.push_back('"');
            return processed_node;   
        } else {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_ARGUMENT, "display-string", processed_node);
            throw evalerrormsg;               
        } 
        return nullptr;
    }
    Node* Evaluate_Newline(Node* node) {
        int arg_count = 0;
        Node* head = node;
        node = node->right;
        while (true) {
            if (node == nullptr) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_LIST, "newline", head);
                throw evalerrormsg;                 
            }
            if (node->token.type == NIL) {
                break;
            }
            arg_count++;
            node = node->right;
        } 
        if (arg_count != 0) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, "newline", head);
            throw evalerrormsg;            
        }  
        cout << endl;
        Token token;
        token.type = NIL;
        token.value = "nil";
        return new Node(token);
    }
    Node* Evaluate_Symbol2String(Node* node) {
        int arg_count = 0;
        Node* head = node;
        node = node->right;
        while (true) {
            if (node == nullptr) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_LIST, "symbol->string", head);
                throw evalerrormsg;                 
            }
            if (node->token.type == NIL) {
                break;
            }
            arg_count++;
            node = node->right;
        } 
        if (arg_count != 1) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, "symbol->string", head);
            throw evalerrormsg;            
        }  
        Node* processed_node = EvalSExp(head->right->left);
        if (processed_node->token.type == SYMBOL) {
            Token new_token;
            new_token.type = STRING;
            new_token.value = "\"" + processed_node->token.value + "\"";
            return new Node(new_token); 
        } else {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_ARGUMENT, "symbol->string", processed_node);
            throw evalerrormsg;               
        } 
        return nullptr;
    }
    Node* Evaluate_Number2String(Node* node) {  
        int arg_count = 0;
        Node* head = node;
        node = node->right;
        while (true) {
            if (node == nullptr) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_LIST, "number->string", head);
                throw evalerrormsg;                 
            }
            if (node->token.type == NIL) {
                break;
            }
            arg_count++;
            node = node->right;
        } 
        if (arg_count != 1) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, "number->string", head);
            throw evalerrormsg;            
        }  
        Node* processed_node = EvalSExp(head->right->left);
        if (processed_node->token.type == INT || processed_node->token.type == FLOAT) {
            string formatted;
            if (processed_node->token.type == INT) {
                formatted = processed_node->token.value;
                if (!formatted.empty() && formatted[0] == '+') {
                    formatted.erase(formatted.begin());
                }
            } else {
                double num = stod(processed_node->token.value);
                ostringstream oss;
                oss << fixed << setprecision(3) << num;
                formatted = oss.str();
                if (!formatted.empty() && formatted[0] == '.')
                    formatted = "0" + formatted;
                else if (formatted.size() >= 2 && formatted[0] == '-' && formatted[1] == '.')
                    formatted = "-0" + formatted.substr(1);
            }
            Token new_token;
            new_token.type = STRING;
            new_token.value = "\"" + formatted + "\"";
            return new Node(new_token);   
        } else {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_ARGUMENT, "number->string", processed_node);
            throw evalerrormsg;
        }
        return nullptr;
    }
    bool IsDefineForm(Node* node) {
        return node != nullptr && node->token.type == PAIR && node->left != nullptr &&
                node->left->token.type == SYMBOL &&  node->left->token.value == "define";
    }
    Node* Evaluate_Eval(Node* node) {
        int arg_count = 0;
        Node* head = node;
        node = node->right;
        while (true) {
            if (node == nullptr) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_LIST, "eval", head);
                throw evalerrormsg;
            }
            if (node->token.type == NIL) {
                break;  
            }
            arg_count++;
            node = node->right;
        }
        if (arg_count != 1) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, "eval", head);
            throw evalerrormsg;
        }
        Node* processed_node = EvalSExp(head->right->left);
        bool eval_define = IsDefineForm(processed_node);
        int saved_level = level;
        level = 0;
        try {
            Node* sec_processed_node = EvalSExp(processed_node);
            level = saved_level;
            if (eval_define) {
                if (sec_processed_node != nullptr) {
                    Printer printer;
                    printer.PrintSExp(sec_processed_node, false, 0, false);
                    cout << endl;
                }
                return nullptr;
            }
            return sec_processed_node;
        } catch (...) {
            level = saved_level;
            throw;
        }
    }
    Node* Evaluate_set(Node* node) {
        int arg_count = 0;
        Node* head = node;
        node = node->right;
        while (true) {
            if (node == nullptr) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_LIST, "set!", head);
                throw evalerrormsg;                 
            }
            if (node->token.type == NIL) {
                break;
            }
            arg_count++;
            node = node->right;
        } 
        if (arg_count != 2) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, "set!", head);
            throw evalerrormsg;            
        } 
        Node* symbol_node = head->right->left;
        if (symbol_node->token.type != SYMBOL) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(SET_FORMAT, "set!", head);
            throw evalerrormsg;    
        }
        Node* arg_node = EvalSExp(head->right->right->left);
        environment.Env_set(symbol_node->token.value, arg_node);
        return arg_node;
    }
    Node* Evaluate_Read(Node* node) {
        int arg_count = 0;
        Node* head = node;
        node = node->right;
        while (true) {
            if (node == nullptr) {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_LIST, "read", head);
                throw evalerrormsg;                 
            }
            if (node->token.type == NIL) {
                break;
            }
            arg_count++;
            node = node->right;
        } 
        if (arg_count != 0) {
            EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, "read", head);
            throw evalerrormsg;            
        }    
        Parser parser;
    try {
        Node* input_node = parser.ReadSExp(false); 
        return input_node;
        
    } catch (string err_msg) {
        Token err_token;
        err_token.type = ERROR_OBJ;
        err_token.value = err_msg; 
        
        return new Node(err_token); 
        
    } catch (ErrorType errortype) {
        if (errortype == NO_MORE_INPUT) {
            Token err_token;
            err_token.type = ERROR_OBJ;
            err_token.value = "ERROR: END-OF-FILE encountered when there should be more input";
            return new Node(err_token);
        }
        throw errortype; 
    }
    }
  public:
    void Reset() {
        level = 0;
    }
    void ResetEnvironment() {
        environment.ResetToGlobal();
    }
    Node* EvalSExp(Node* node) {
        if (node->token.type == INT || node->token.type == STRING || node->token.type == FLOAT 
            || node->token.type == NIL || node->token.type == T) {
            return node;
        }
        if (node->token.type == SYMBOL) {
            if (all_procedure.count(node->token.value)) {
                Token p_token;
                p_token.type = PROCEDURE;
                p_token.value = node->token.value; 
                return new Node(p_token);
            } else {
                Node* lookup_node = environment.Lookup(node->token.value);
                if (lookup_node != nullptr) {
                    return lookup_node;
                }
                throw EvalErrorMsg(UNBOUND_SYMBOL, node->token.value);
            }                                                       
        }
        if (node->token.type == PAIR) {
            Node* check = node;
            while (check != nullptr && check->token.type == PAIR) {
                check = check->right;
            }
            if (check != nullptr && check->token.type == NIL) {

            } else {
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_LIST, "", node);
                throw evalerrormsg;                      
            }
            if (node->left->token.type == QUOTE || node->left->token.value == "quote") {
                return node->right->left;
            }
            level++;
            Node* op_node = EvalSExp(node->left);
            if (op_node->token.type == PROCEDURE && all_procedure.count(op_node->token.value)) {
                string cmd = op_node->token.value;             
                if (cmd == "+" || cmd == "-" || cmd == "*" || cmd == "/") {
                    return EvaluateMath(cmd, node);                 
                } else if (cmd == "=" || cmd == ">" || cmd == ">="
                || cmd == "<" || cmd == "<=") {
                    return EvaluateCompare(cmd, node); 
                } else if (cmd == "car") {
                    return Evaluate_car_cdr("car", node)->left;
                } else if (cmd == "cdr") {
                    return Evaluate_car_cdr("cdr", node)->right;                    
                } else if (cmd == "cons") {
                    return Evaluate_cons(node); 
                } else if (cmd == "atom?" || cmd == "pair?" || cmd == "list?" || cmd == "null?"
                || cmd == "integer?" || cmd == "real?" || cmd == "number?" || cmd == "string?" 
                || cmd == "boolean?" || cmd == "symbol?" || cmd == "not" || cmd == "error-object?") { // atom?, pair?, list?, null?, integer?, real?, number?, string?, boolean?, symbol?, not
                    return EvaluateIdentity(cmd, node); 
                } else if (cmd == "if") {
                    return Evaluate_if(node); 
                } else if (cmd == "cond") {
                    return Evaluate_cond(node); 
                } else if (cmd == "eqv?" || cmd == "equal?") {
                    return Evaluate_eqv_equal(cmd, node);
                } else if (cmd == "define") {
                    if (level > 1) {
                        EvalErrorMsg evalerrormsg = EvalErrorMsg(DEFINE_LEVEL, cmd);
                        throw evalerrormsg;                           
                    }                    
                    return Evaluate_define(node); 
                } else if (cmd == "clean-environment") {
                    if (level > 1) {
                        EvalErrorMsg evalerrormsg = EvalErrorMsg(CLEAN_ENVIRONMENT_LEVEL, cmd);
                        throw evalerrormsg;                           
                    }
                    environment.Clear();
                    string str = "environment cleaned";
                    if (!is_verbose) {
                        return nullptr;
                    }
                    Token token;
                    token.type = SYMBOL;
                    token.value = str;
                    Node* defined_cout = new Node(token);
                    return defined_cout;                    
                } else if (cmd == "list") {
                    return Evaluate_list(node);
                } else if (cmd == "string>?" || cmd == "string<?" || cmd == "string=?") {
                    return EvaluateStringCompare(cmd, node);
                } else if (cmd == "string-append") {
                    return EvaluateStringAppend(node);
                } else if (cmd == "begin") {
                    return Evaluate_begin(node);
                } else if (cmd == "and" || cmd == "or") {
                    return Evaluate_and_or(cmd, node);
                } else if (cmd == "let") {
                    return Evaluate_let(node);
                } else if (cmd == "lambda") {
                    return Evaluate_Lambda(node);
                } else if (cmd == "verbose") {
                    return Evaluate_verbose(node);
                } else if (cmd == "verbose?") {
                    return Evaluate_Isverbose();
                } else if (cmd == "quote") {
                    return node->right->left;
                } else if (cmd == "create-error-object") {
                    return Evaluate_Create_Error_Object(node);
                } else if (cmd == "write") {
                    return Evaluate_Write(node);
                } else if (cmd == "display-string") {
                    return Evaluate_Display_String(node);
                } else if (cmd == "newline") {
                    return Evaluate_Newline(node);
                } else if (cmd == "symbol->string") {
                    return Evaluate_Symbol2String(node);
                } else if (cmd == "number->string") {
                    return Evaluate_Number2String(node);
                } else if (cmd == "eval") {
                    return Evaluate_Eval(node);
                } else if (cmd == "set!") {
                    return Evaluate_set(node);
                } else if (cmd == "read") {
                    return Evaluate_Read(node);
                } else if (cmd == "exit") {
                    if (level > 1) {
                        EvalErrorMsg evalerrormsg = EvalErrorMsg(EXIT_LEVEL, cmd);
                        throw evalerrormsg;                           
                    }
                    EvalErrorMsg evalerrormsg = EvalErrorMsg(INCORRECT_NUMBER, "exit");
                    throw evalerrormsg;                           
                } else {
                    EvalErrorMsg evalerrormsg = EvalErrorMsg(UNBOUND_SYMBOL, node->left->token.value);
                    throw evalerrormsg;   
                }
            } else {
                if (op_node->token.type == PROCEDURE) {
                    try {
                        return Apply_Procedure(op_node, node);
                    } catch (EvalErrorMsg& e) { // fix stucking on previous value
                        if (e.errortype == NO_RETURN_VALUE && !e.is_locked) {
                            e.crash_node = node; 
                        }
                        throw e; 
                    }
                }
                EvalErrorMsg evalerrormsg = EvalErrorMsg(NON_FUNCTION, op_node->token.value, op_node);
                throw evalerrormsg;     
            }
        }

        return node;
    }
  
};

int main() {
    string thisline;
    Parser parser;
    Evaluator evaluator; 
    Printer printer;
    cout << "Welcome to OurScheme!\n";
    getline(cin, thisline);
    while (true) { // '(exit)' OR EOF
        parser.Reset();
        evaluator.Reset();
        cout << "\n> ";
        try {
            Node* node = parser.ReadSExp(true);
            if (node->token.type == PAIR && node->left->token.value == "exit" &&
                node->left != nullptr && node->right != nullptr && node->right->token.type == NIL) {
                break;
            }
            Node* processed_node = evaluator.EvalSExp(node);
            printer.PrintSExp(processed_node, false, 0, false);
        } catch (ErrorType errortype) {
            if (errortype == NO_MORE_INPUT) {
                cout << "ERROR (no more input) : END-OF-FILE encountered";
                break;                
            }
            while (cin.peek() != '\n' && cin.peek() != EOF) {
                cin.get();
            }
            if (cin.peek() == '\n') cin.get();
        } catch (EvalErrorMsg evalerrormsg) {
            bool is_error_type = true;
            evaluator.ResetEnvironment();
            if (evalerrormsg.errortype == UNBOUND_SYMBOL) {
                cout << "ERROR (unbound symbol) : " << evalerrormsg.value;
            } else if (evalerrormsg.errortype == INCORRECT_NUMBER) {
                cout << "ERROR (incorrect number of arguments) : " << evalerrormsg.value;
            } else if (evalerrormsg.errortype == INCORRECT_ARGUMENT) {
                cout << "ERROR (" << evalerrormsg.value << " with incorrect argument type) : ";
                printer.PrintSExp(evalerrormsg.crash_node, false, 0, is_error_type); 
            } else if (evalerrormsg.errortype == NON_FUNCTION) {
                cout << "ERROR (attempt to apply non-function) : ";
                printer.PrintSExp(evalerrormsg.crash_node, false, 0, false); 
            } else if (evalerrormsg.errortype == NO_RETURN_VALUE) {
                cout << "ERROR (no return value) : ";
                printer.PrintSExp(evalerrormsg.crash_node, false, 0, is_error_type); 
            } else if (evalerrormsg.errortype == DIVISION_BY_ZERO) {
                cout << "ERROR (division by zero) : " << evalerrormsg.value;
            } else if (evalerrormsg.errortype == COND_FORMAT) {
                cout << "ERROR (COND format) : ";
                printer.PrintSExp(evalerrormsg.crash_node, false, 0, is_error_type); 
            } else if (evalerrormsg.errortype == DEFINE_FORMAT) {
                cout << "ERROR (DEFINE format) : ";
                printer.PrintSExp(evalerrormsg.crash_node, false, 0, is_error_type); 
            } else if (evalerrormsg.errortype == NON_LIST) {
                cout << "ERROR (non-list) : ";
                printer.PrintSExp(evalerrormsg.crash_node, false, 0, is_error_type); 
            } else if (evalerrormsg.errortype == STRING_APPEND_INCORRECT_TYPE) {
                cout << "ERROR (string-append with incorrect argument type) : " << evalerrormsg.value;
            } else if (evalerrormsg.errortype == CLEAN_ENVIRONMENT_LEVEL) {
                cout << "ERROR (level of CLEAN-ENVIRONMENT)";
            } else if (evalerrormsg.errortype == DEFINE_LEVEL) {
                cout << "ERROR (level of DEFINE)";
            } else if (evalerrormsg.errortype == EXIT_LEVEL) {
                cout << "ERROR (level of EXIT)";
            } else if (evalerrormsg.errortype == LET_FORMAT) {
                cout << "ERROR (LET format) : ";
                printer.PrintSExp(evalerrormsg.crash_node, false, 0, is_error_type);                 
            } else if (evalerrormsg.errortype == UNBOUND_PARAMETER) {
                cout << "ERROR (unbound parameter) : ";
                printer.PrintSExp(evalerrormsg.crash_node, false, 0, is_error_type);                    
            } else if (evalerrormsg.errortype == UNBOUND_CONDITION) {
                cout << "ERROR (unbound condition) : ";
                printer.PrintSExp(evalerrormsg.crash_node, false, 0, is_error_type);                
            } else if (evalerrormsg.errortype == LAMBDA_FORMAT) {
                cout << "ERROR (LAMBDA format) : ";
                printer.PrintSExp(evalerrormsg.crash_node, false, 0, is_error_type);                 
            } else if (evalerrormsg.errortype == UNBOUND_TEST_COND) {
                cout << "ERROR (unbound test-condition) : ";
                printer.PrintSExp(evalerrormsg.crash_node, false, 0, is_error_type);                
            } else if (evalerrormsg.errortype == SET_FORMAT) {
                cout << "ERROR (SET! format) : ";
                printer.PrintSExp(evalerrormsg.crash_node, false, 0, is_error_type);                 
            } 
            if (cin.peek() == '\n') cin.get();        
        }
        catch (string err_msg) {
            cout << err_msg;
            while (cin.peek() != '\n' && cin.peek() != EOF) {
                cin.get();
            }
            if (cin.peek() == '\n') cin.get();
        }
        cout << endl;
    } 
    cout << "\nThanks for using OurScheme!";
    return 0;
}
