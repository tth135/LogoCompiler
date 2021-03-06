#include "Executor.h"
#include "FileWriter.h"
#include "Function.h"
#include <iostream>
Executor *Executor::globalExe = nullptr;
Executor::Executor() {
    penColor = Pixel(0, 0, 0, 1);
    Function *globalFunc = new Function("0global", std::vector<VariableWrapper>());
    callStack.push_back(StackFrame(globalFunc, 0, std::vector<Variable>()));
    current_function = globalFunc;
    current_ops = current_function->getOps();
    allFunctions.push_back(globalFunc);
    globalExe = this;
    pc = 0;
}

Executor::~Executor() {
}
Variable &Executor::getVariableByName(std::string name) {
    // std::cout << "debug: name=" << name << ", result=";

    Variable *v = nullptr;
    bool found = false;
    // check local variables in every stack frame
    for (auto it = callStack.rbegin(); it != callStack.rend(); it++) {
        if (found)
            break;
        std::vector<Variable> &l_vars = it->localVariables;
        for (auto it2 = l_vars.begin(); it2 != l_vars.end(); it2++) {
            if (it2->getName() == name) {
                v = &(*it2);
                found = true;
                break;
            }
        }
    }

    if (v) {
        // found
        return *v;
    } else {
        // not found
        return Variable::noVar();
    }
}
Variable &Executor::getVariableByNameStatic(std::string name) {
    return globalExe->getVariableByName(name);
}

void Executor::initNewBuffer(int width, int height) {
    delete buffer;
    this->width = width;
    this->height = height;
    buffer = new unsigned char[width * height * sizeof(Pixel)];
}

void Executor::setBackground(int R, int G, int B) {
    Pixel *pixels = reinterpret_cast<Pixel *>(buffer);
    Pixel fill(R, G, B, 1);
    for (size_t i = 0; i < width * height; i++) {
        pixels[i] = fill;
    }
}

void Executor::setPenPosition(int x, int y) {
    logical_pen_x = x;
    logical_pen_y = y;
}

void Executor::def(std::string name, int value, int lineno) {
    Op *op;
    op = new DefOp(this, name, value, lineno);
    current_ops->push_back(op);
}

void Executor::add(VariableWrapper vw, VariableWrapper value, int lineno) {
    Op *op;
    op = new AddOp(this, vw, value, lineno);
    current_ops->push_back(op);
}

void Executor::move(std::string varName, int lineno) {
    Op *op;
    op = new MoveOp(this, varName, lineno);
    current_ops->push_back(op);
}

void Executor::move(int step, int lineno) {
    Op *op;
    op = new MoveOp(this, step, lineno);
    current_ops->push_back(op);
}

void Executor::cloak(int lineno) {
    Op *op;
    op = new CloakOp(this, lineno);
    current_ops->push_back(op);
}

void Executor::turn(VariableWrapper vw, int lineno) {
    Op *op;
    op = new TurnOp(this, vw, lineno);
    current_ops->push_back(op);
}

void Executor::setPenColor(VariableWrapper r, VariableWrapper g, VariableWrapper b, int lineno) {
    Op *op;
    op = new ColorOp(this, r, g, b, lineno);
    current_ops->push_back(op);
}

void Executor::loop(int value, int lineno) {
    Op *op;
    op = new StartLoopOp(this, value, lineno);
    current_ops->push_back(op);
}

void Executor::endLoop(int lineno) {
    Op *op;
    Op *start = nullptr;
    int cnt = 1;
    for (auto it = current_ops->rbegin(); it != current_ops->rend(); it++) {
        if ((*it)->isEndLoopOp()) {
            cnt++;
        }
        if ((*it)->isStartLoopOp()) {
            cnt--;
            if (cnt == 0) {
                start = *it;

                break;
            }
        }
    }
    if (start) {
        op = new EndLoopOp(this, start, lineno);
        dynamic_cast<StartLoopOp *>(start)->setEndLoopOp(op);
        current_ops->push_back(op);
    } else {
        issueError("unexpected END LOOP");
    }
}

void Executor::startFuncDef(std::string name, std::vector<VariableWrapper> list, int lineno) {
    Function *f = new Function(name, list);
    current_function = f;
    current_ops = f->getOps();
    allFunctions.push_back(f);
    // OpsQueue *q = new OpsQueue(name);
    // current_ops = q->getOps();
    // allOps.push_back(q);
}

void Executor::endFuncDef(int lineno) {
    // set current_ops to 0global
    current_function = allFunctions[0];
    current_ops = current_function->getOps();
}
void Executor::run() {

    while (!callStack.empty()) {
        current_function = callStack[callStack.size() - 1].function;
        current_ops = current_function->getOps();
        //        StackFrame& frame =callStack[callStack.size()-1];
        //        size_t ops_cnt = current_ops->size();
        if (verbose)
            std::cout << "Execute " << current_function->getName() << " from pc[" << pc << "]" << std::endl;
        while (pc < current_ops->size()) {
            if (verbose)
                std::cout << "pc[" << pc << "]: ";
            (*current_ops)[pc]->exec();
            pc++;
        }
        // frame complete
        if (verbose)
            std::cout << "return from " << current_function->getName() << std::endl;

        pc = callStack[callStack.size() - 1].ret_pc + 1;
        callStack.pop_back();
    }
}
void Executor::step() {
    bool runOnce = true;
}

void Executor::drawPixel(int x, int y) {
    getBufferPixel(x, y) = penColor;
}

Pixel &Executor::getBufferPixel(int x, int y) {
    // std::cout << "get buffer[" << x << "," << y << "]" << std::endl;
    if (0 <= x && x < width && 0 <= y && y < height) {
        Pixel *p = reinterpret_cast<Pixel *>(buffer);
        p += y * this->width;
        p += x;
        return *p;
    } else {
        return _noPixel;
    }
}

void Executor::writeFile(std::string filename) {
    FileWriter writer;
    auto sz = writer.WriteBMP(filename, this->buffer, width, height);
    if (verbose)
        std::cout << "write file return value: " << sz << std::endl;
    if (sz) {
        std::cout << "write to file " << filename << std::endl;
    } else {
        std::cerr << "cannot write to file " << filename << std::endl;
    }
}

void Executor::call(std::string name, std::vector<VariableWrapper> paraList, int lineno) {
    Op *op;
    op = new CallOp(this, name, paraList, lineno);
    current_ops->push_back(op);
}

void Executor::setPenWidth(VariableWrapper w, int lineno) {
    Op *op;
    op = new SetPenWidthOp(this, w, lineno);
    current_ops->push_back(op);
}
void Executor::fill(int lineno) {
    Op *op;
    op = new FillOp(this, lineno);
    current_ops->push_back(op);
}

StackFrame::~StackFrame() {
}