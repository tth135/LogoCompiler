#include "Interpreter.h"
#include "Function.h"
#include "Variable.h"
#include "symbols.h"
#include "utility.h"
#include <sstream>
Interpreter::Interpreter() {
}

Interpreter::~Interpreter() {
}

int assertSymbolType(Symbol &s, SymbolType type) {
    if (s.getType() == type) {
        return 0;
    }
    std::cerr << "Unexpected symbol at line " << s.getLineno() << ": [type=" << getSymbolTypeName(s.getType()) << ", name=\'" << s.getName() << "\', value=" << s.getValue() << "]" << std::endl;
    exit(1);
    return -1;
}
void Interpreter::compile(const char *filename, const char *outName) {
    FILE *fp = fopen(filename, "r");
    if (fp == nullptr) {
        std::cout << "Cannot open the file" << std::endl;
        return;
    }
    extern FILE *yyin;
    yyin = fp;
    yylex();
    fclose(fp);

    // std::cout << "symbol queue size: " << lexQueue.size() << std::endl;
    if (lexQueue.empty()) {
        issueError("The file is empty");
    }
    // header
    assertSymbolType(lexQueue.front(), ATSIZE);
    lexQueue.pop(); // @SIZE
    int width, height;
    width = nextInt();
    height = nextInt();
    executor.initNewBuffer(width, height);

    assertSymbolType(lexQueue.front(), ATBACKGROUND);
    lexQueue.pop(); // @BACKGROUND
    int r;
    int g;
    int b;
    r = nextInt();
    g = nextInt();
    b = nextInt();
    executor.setBackground(r, g, b);

    assertSymbolType(lexQueue.front(), ATPOSITION);
    lexQueue.pop(); // @POSITION
    int x, y;
    x = nextInt();
    y = nextInt();
    executor.setPenPosition(x, y);

    // body
    while (!lexQueue.empty()) {
        Symbol s = nextSymbol();
        processSymbol(s);
    }
    if (executor.current_function->getName() != "0global") {
        issueError("End of file in function definition, did you miss \"END FUNC\" for " + executor.current_function->getName() + "()?");
    }
    executor.run();
    std::string outFileName;
    if (outName) {
        outFileName = outName;
    } else {
        std::string inputName(filename);
        // remove the last ".bmp", if there is one
        if (ends_with(inputName, ".logo") || ends_with(inputName, ".LOGO")) {
            outFileName = std::string(inputName.begin(), inputName.end() - 5) + ".bmp";
        } else {
            outFileName = inputName + ".bmp";
        }
    }

    executor.writeFile(outFileName);
}

int Interpreter::nextInt() {
    if (lexQueue.empty()) {
        issueError("Expecting a value");
    }
    assertSymbolType(lexQueue.front(), INTCONST);
    int result;
    result = lexQueue.front().getValue();
    lexQueue.pop();
    return result;
}

VariableWrapper Interpreter::getNextVariableWrapper() {
    if (lexQueue.empty()) {
        issueError("Expecting a symbol");
    }
    auto sym = nextSymbol();
    if (sym.getType() == INTCONST) {
        return VariableWrapper(sym.getValue());
    } else {
        assertSymbolType(sym, IDENTIFIER);
        return VariableWrapper(sym.getName());
    }
}

Symbol Interpreter::nextSymbol() {
    if (lexQueue.empty()) {
        issueError("Expecting a symbol");
    }
    Symbol result = lexQueue.front();
    lexQueue.pop();
    return result;
}

void Interpreter::processSymbol(Symbol &symbol) {
    if (symbol.getType() == TURN) {
        auto sym = nextSymbol();
        if (sym.getType() == INTCONST) {
            executor.turn(VariableWrapper(sym.getValue()), symbol.getLineno());
        } else {
            assertSymbolType(sym, IDENTIFIER);
            executor.turn(VariableWrapper(sym.getName()), symbol.getLineno());
        }
    } else if (symbol.getType() == MOVE) {
        auto sym = nextSymbol();
        if (sym.getType() == INTCONST) {
            // Todo: use VariableWrapper
            executor.move(sym.getValue(), symbol.getLineno());
        } else {
            assertSymbolType(sym, IDENTIFIER);
            // Todo: use VariableWrapper
            executor.move(sym.getName(), symbol.getLineno());
        }
    } else if (symbol.getType() == ENDLOOP) {
        executor.endLoop(symbol.getLineno());
    } else if (symbol.getType() == ENDFUNC) {
        executor.endFuncDef(symbol.getLineno());
    } else if (symbol.getType() == FILL) {
        executor.fill(symbol.getLineno());
    } else if (symbol.getType() == PENWIDTH) {
        auto widthValue = nextSymbol();
        VariableWrapper value(0);
        if (widthValue.getType() == INTCONST) {
            value = VariableWrapper(widthValue.getValue());
        } else {
            assertSymbolType(widthValue, IDENTIFIER);
            value = VariableWrapper(widthValue.getName());
        }
        executor.setPenWidth(value, symbol.getLineno());
    } else if (symbol.getType() == ADD) {
        auto sym = nextSymbol();
        assertSymbolType(sym, IDENTIFIER);
        auto addValue = nextSymbol();
        VariableWrapper value(0);
        if (addValue.getType() == INTCONST) {
            value = VariableWrapper(addValue.getValue());
        } else {
            assertSymbolType(addValue, IDENTIFIER);
            value = VariableWrapper(addValue.getName());
        }
        executor.add(VariableWrapper(sym.getName()), value, symbol.getLineno());

    } else if (symbol.getType() == COLOR) {
        VariableWrapper r(0);
        VariableWrapper g(0);
        VariableWrapper b(0);
        r = getNextVariableWrapper();
        g = getNextVariableWrapper();
        b = getNextVariableWrapper();
        executor.setPenColor(r, g, b, symbol.getLineno());
    } else if (symbol.getType() == CLOAK) {
        executor.cloak(symbol.getLineno());
    } else if (symbol.getType() == LOOP) {
        int loop = nextInt();
        executor.loop(loop, symbol.getLineno());
    } else if (symbol.getType() == FUNC) {
        auto funcSymbol = nextSymbol();
        assertSymbolType(funcSymbol, IDENTIFIER);
        auto list = getIdentifierList();
        executor.startFuncDef(funcSymbol.getName(), list, symbol.getLineno());
    }

    else if (symbol.getType() == CALL) {
        auto funcSymbol = nextSymbol();
        assertSymbolType(funcSymbol, IDENTIFIER);
        auto list = getParaList();
        executor.call(funcSymbol.getName(), list, symbol.getLineno());
    } else if (symbol.getType() == DEF) {
        auto varName = nextSymbol();
        assertSymbolType(varName, IDENTIFIER);
        int init_value = nextInt();
        executor.def(varName.getName(), init_value, symbol.getLineno());
    } else {
        std::string msg = "Unexpected symbol: " + symbol.getName();
        issueError(msg, symbol.getLineno());
    }
}
std::vector<VariableWrapper> Interpreter::getParaList() {
    std::vector<VariableWrapper> result;
    auto symbol = nextSymbol();
    assertSymbolType(symbol, LPAR);
    symbol = nextSymbol();
    while (symbol.getType() != RPAR) {
        if (symbol.getType() == IDENTIFIER) {
            result.push_back(VariableWrapper(symbol.getName()));
        } else if (symbol.getType() == INTCONST) {
            result.push_back(VariableWrapper(symbol.getValue()));
        }
        symbol = nextSymbol();
        if (symbol.getType() == COMMA) {
            symbol = nextSymbol();
        }
    }
    return result;
}
std::vector<VariableWrapper> Interpreter::getIdentifierList() {
    std::vector<VariableWrapper> result;
    auto symbol = nextSymbol();
    assertSymbolType(symbol, LPAR);
    symbol = nextSymbol();
    while (symbol.getType() != RPAR) {
        if (symbol.getType() == IDENTIFIER) {
            result.push_back(VariableWrapper(symbol.getName()));
        } else if (symbol.getType() == INTCONST) {
            issueError("unexpected int const here", symbol.getLineno());
        }

        symbol = nextSymbol();
        if (symbol.getType() == COMMA) {
            symbol = nextSymbol();
        }
    }
    return result;
}

void Interpreter::issueError(std::string err, int lineno) {
    if (lineno == -1) {
        std::cerr << "Error: " << err << std::endl;
    } else {
        std::cerr << "Error at line " << lineno << ": " << err << std::endl;
    }
    exit(1);
}

void Interpreter::issueWarning(std::string err, int lineno) {
    if (lineno == -1) {
        std::cerr << "Warning: " << err << std::endl;
    } else {
        std::cout << "Warning at " << lineno << ": " << err << std::endl;
    }
}
