# scheme-interpreter-cpp
A Scheme-like interpreter implemented in C++ with custom scanner, parser, evaluator, and environment management.


Scheme-like Interpreter

A Scheme-like interpreter implemented in C++, featuring lexical scanning, S-expression parsing, expression evaluation, variable environments, procedure application, and runtime error handling.

Overview

This project implements a command-line interpreter for a subset of the Scheme programming language.

The interpreter processes user input through several stages:

Input → Scanner → Parser → S-expression Tree → Evaluator → Environment → Output

The project was developed as a Programming Languages course project and provided hands-on experience with language parsing, recursive data structures, evaluation rules, scope management, and error handling.

Features
Tokenization of integers, floating-point numbers, strings, symbols, and Scheme syntax
Parsing of nested S-expressions
Arithmetic and comparison operations
List operations such as car, cdr, and cons
Conditional expressions including if and cond
Variable definition and lookup
Local variable bindings with let
Anonymous functions with lambda
User-defined procedures
Environment and scope management
eval, read, and set!
Runtime and syntax error handling
Architecture
User Input
    ↓
Scanner
    ↓
Tokens
    ↓
Parser
    ↓
S-expression Tree
    ↓
Evaluator
    ↓
Environment
    ↓
Result / Error
Scanner

Reads raw input and converts it into tokens such as numbers, strings, symbols, parentheses, quotes, and special values.

Parser

Transforms the token stream into recursive S-expression structures used by the evaluator.

Evaluator

Evaluates expressions according to Scheme semantics and dispatches built-in procedures and special forms.

Environment

Maintains variable and procedure bindings and supports global and local scopes.

Demo

Example interactions:

> (+ 10 20)
30

> (define x 5)
x defined

> (* x 4)
20

More examples and screenshots are available in the docs and examples directories.

Implementation

Language: C++

Main concepts used in the implementation:

Recursive parsing
Tree-based expression representation
Dynamic environment management
Function parameter binding
Recursive evaluation
Exception and error handling
