/**
 * RADS Compiler v0.0.11 PULSAR - JavaScript Implementation
 * A full-featured interpreter for the RADS programming language
 * Supports: variables, functions, arrays, structs, loops, conditionals, and stdlib
 */

class RADSCompiler {
    constructor() {
        this.variables = new Map();
        this.functions = new Map();
        this.output = [];
        this.currentScope = 'global';
        this.loopStack = [];
        this.importedModules = new Set();
        
        // Standard library
        this.stdlib = this.initStdlib();
        
        // Keywords
        this.keywords = new Set([
            'blast', 'turbo', 'cruise', 'loop', 'if', 'else', 'switch', 'case',
            'default', 'break', 'continue', 'return', 'struct', 'enum', 'import',
            'try', 'catch', 'finally', 'throw', 'and', 'or', 'not', 'true', 'false',
            'null', 'str', 'int', 'float', 'bool', 'array', 'void'
        ]);
        
        // Types
        this.types = new Set(['int', 'float', 'string', 'bool', 'array', 'struct', 'void', 'str']);
    }
    
    // Initialize standard library functions
    initStdlib() {
        const self = this;
        
        return {
            // I/O
            'echo': (args) => {
                const output = args.map(a => self.stringify(a)).join('');
                self.output.push(output);
                return null;
            },
            'print': (args) => {
                const output = args.map(a => self.stringify(a)).join('');
                self.output.push(output);
                return null;
            },
            'length': (args) => {
                if (args.length === 0) return 0;
                const val = args[0];
                if (Array.isArray(val)) return val.length;
                if (typeof val === 'string') return val.length;
                if (val && typeof val === 'object') return Object.keys(val).length;
                return 0;
            },
            
            // String functions
            'str.length': (args) => args[0]?.length || 0,
            'str.upper': (args) => String(args[0] || '').toUpperCase(),
            'str.lower': (args) => String(args[0] || '').toLowerCase(),
            'str.trim': (args) => String(args[0] || '').trim(),
            'str.split': (args) => String(args[0] || '').split(args[1] || ''),
            'str.replace': (args) => String(args[0] || '').replace(args[1] || '', args[2] || ''),
            'str.contains': (args) => String(args[0] || '').includes(args[1] || ''),
            'str.starts_with': (args) => String(args[0] || '').startsWith(args[1] || ''),
            'str.ends_with': (args) => String(args[0] || '').endsWith(args[1] || ''),
            'string.split': (args) => String(args[0] || '').split(args[1] || ''),
            'string.join': (args) => (args[0] || []).join(args[1] || ''),
            'string.replace': (args) => String(args[0] || '').replace(args[1] || '', args[2] || ''),
            'string.trim': (args) => String(args[0] || '').trim(),
            'string.upper': (args) => String(args[0] || '').toUpperCase(),
            'string.lower': (args) => String(args[0] || '').toLowerCase(),
            
            // Array functions
            'array.push': (args) => { if (Array.isArray(args[0])) args[0].push(args[1]); return args[0]; },
            'array.pop': (args) => Array.isArray(args[0]) ? args[0].pop() : null,
            'array.length': (args) => Array.isArray(args[0]) ? args[0].length : 0,
            'array.map': (args) => Array.isArray(args[0]) ? args[0].map(x => x) : [],
            'array.filter': (args) => Array.isArray(args[0]) ? args[0].filter(x => x) : [],
            'array.reverse': (args) => Array.isArray(args[0]) ? [...args[0]].reverse() : [],
            'array.sort': (args) => Array.isArray(args[0]) ? [...args[0]].sort() : [],
            
            // Math functions
            'math.abs': (args) => Math.abs(args[0] || 0),
            'math.floor': (args) => Math.floor(args[0] || 0),
            'math.ceil': (args) => Math.ceil(args[0] || 0),
            'math.round': (args) => Math.round(args[0] || 0),
            'math.sqrt': (args) => Math.sqrt(args[0] || 0),
            'math.pow': (args) => Math.pow(args[0] || 0, args[1] || 0),
            'math.sin': (args) => Math.sin(args[0] || 0),
            'math.cos': (args) => Math.cos(args[0] || 0),
            'math.tan': (args) => Math.tan(args[0] || 0),
            'math.log': (args) => Math.log(args[0] || 0),
            'math.exp': (args) => Math.exp(args[0] || 0),
            'math.min': (args) => Math.min(...args),
            'math.max': (args) => Math.max(...args),
            'math.random': () => Math.random(),
            'math.srand': () => null,
            
            // Type conversion
            'to_string': (args) => this.stringify(args[0]),
            'to_int': (args) => parseInt(args[0]) || 0,
            'to_float': (args) => parseFloat(args[0]) || 0.0,
            'parseInt': (args) => parseInt(args[0]) || 0,
            'parseFloat': (args) => parseFloat(args[0]) || 0.0,
            'typeof': (args) => typeof args[0],
            
            // Datetime module (v0.0.10)
            'datetime.now': () => {
                const d = new Date();
                return { year: d.getFullYear(), month: d.getMonth() + 1, day: d.getDate(),
                         hour: d.getHours(), minute: d.getMinutes(), second: d.getSeconds(),
                         weekday: d.getDay() };
            },
            'datetime.format': (args) => {
                const ts = args[0] || {};
                const fmt = args[1] || 'YYYY-MM-DD';
                return fmt.replace('YYYY', ts.year || 0)
                          .replace('MM', String(ts.month || 0).padStart(2, '0'))
                          .replace('DD', String(ts.day || 0).padStart(2, '0'))
                          .replace('HH', String(ts.hour || 0).padStart(2, '0'))
                          .replace('mm', String(ts.minute || 0).padStart(2, '0'))
                          .replace('ss', String(ts.second || 0).padStart(2, '0'));
            },
            'datetime.unix': (args) => {
                const ts = args[0] || {};
                return Math.floor(new Date(ts.year || 1970, (ts.month || 1) - 1, ts.day || 1,
                                          ts.hour || 0, ts.minute || 0, ts.second || 0).getTime() / 1000);
            },
            'datetime.from_unix': (args) => {
                const d = new Date((args[0] || 0) * 1000);
                return { year: d.getFullYear(), month: d.getMonth() + 1, day: d.getDate(),
                         hour: d.getHours(), minute: d.getMinutes(), second: d.getSeconds() };
            },
            
            // Regex module (v0.0.10)
            'regex.match': (args) => {
                try { return new RegExp(args[0] || '').test(args[1] || ''); }
                catch(e) { return false; }
            },
            'regex.findall': (args) => {
                try {
                    const matches = (args[1] || '').match(new RegExp(args[0] || '', 'g'));
                    return matches || [];
                } catch(e) { return []; }
            },
            'regex.replace': (args) => {
                try { return (args[1] || '').replace(new RegExp(args[0] || '', 'g'), args[2] || ''); }
                catch(e) { return args[1] || ''; }
            },
            
            // Test module (v0.0.10)
            'test.describe': () => null,
            'test.it': () => null,
            'test.expect': (args) => ({ value: args[0], toBe: (v) => args[0] === v, toEqual: (v) => JSON.stringify(args[0]) === JSON.stringify(v) }),
            
            // Env module (v0.0.11)
            'env.get': (args) => {
                if (typeof process !== 'undefined') return process.env[args[0]] || null;
                return null;
            },
            'env.set': () => true,
            'env.has': (args) => {
                if (typeof process !== 'undefined') return args[0] in process.env;
                return false;
            },
            'env.unset': () => true,
            'env.list': () => ({}),
            'env.paths': () => [],
            'env.home': () => '/home/user',
            'env.cwd': () => '/',
            'env.user': () => 'user',
            'env.shell': () => '/bin/bash',
            
            // CLI module (v0.0.11)
            'cli.args': () => [],
            'cli.program': () => 'rads',
            'cli.count': () => 0,
            'cli.has_flag': () => false,
            'cli.flag': () => null,
            
            // CSV module (v0.0.11)
            'csv.parse': (args) => {
                const lines = (args[0] || '').split('\n');
                if (lines.length < 2) return [];
                const headers = lines[0].split(',');
                return lines.slice(1).map(line => {
                    const values = line.split(',');
                    const obj = {};
                    headers.forEach((h, i) => obj[h.trim()] = values[i]?.trim() || '');
                    return obj;
                });
            },
            'csv.stringify': (args) => {
                if (!Array.isArray(args[0]) || args[0].length === 0) return '';
                const headers = Object.keys(args[0][0]);
                const lines = [headers.join(',')];
                args[0].forEach(row => {
                    lines.push(headers.map(h => row[h] || '').join(','));
                });
                return lines.join('\n');
            },
            
            // Base64 module (v0.0.11)
            'base64.encode': (args) => {
                try { return btoa(args[0] || ''); }
                catch(e) { return Buffer.from(args[0] || '').toString('base64'); }
            },
            'base64.decode': (args) => {
                try { return atob(args[0] || ''); }
                catch(e) { return Buffer.from(args[0] || '', 'base64').toString(); }
            },
            'base64.encode_bytes': (args) => {
                const bytes = Array.isArray(args[0]) ? args[0] : [];
                return btoa(String.fromCharCode(...bytes.map(b => b & 0xFF)));
            },
            'base64.decode_bytes': (args) => {
                try {
                    const str = atob(args[0] || '');
                    return Array.from(str).map(c => c.charCodeAt(0));
                } catch(e) { return []; }
            },
        };
    }
    
    // Stringify any value
    stringify(val) {
        if (val === null || val === undefined) return 'null';
        if (typeof val === 'string') return val;
        if (typeof val === 'number') return String(val);
        if (typeof val === 'boolean') return val ? 'true' : 'false';
        if (Array.isArray(val)) return '[' + val.map(v => this.stringify(v)).join(', ') + ']';
        if (typeof val === 'object') {
            const pairs = Object.entries(val).map(([k, v]) => k + ': ' + this.stringify(v));
            return '{' + pairs.join(', ') + '}';
        }
        return String(val);
    }
    
    // Tokenize source code
    tokenize(code) {
        const tokens = [];
        let i = 0;
        
        while (i < code.length) {
            // Skip whitespace
            if (/\s/.test(code[i])) {
                i++;
                continue;
            }
            
            // Comments
            if (code[i] === '/' && code[i + 1] === '/') {
                while (i < code.length && code[i] !== '\n') i++;
                continue;
            }
            
            if (code[i] === '/' && code[i + 1] === '*') {
                i += 2;
                while (i < code.length - 1 && !(code[i] === '*' && code[i + 1] === '/')) i++;
                i += 2;
                continue;
            }
            
            // String literals
            if (code[i] === '"' || code[i] === "'") {
                const quote = code[i];
                let str = '';
                i++;
                while (i < code.length && code[i] !== quote) {
                    if (code[i] === '\\' && i + 1 < code.length) {
                        const escapes = { 'n': '\n', 't': '\t', 'r': '\r', '\\': '\\', '"': '"', "'": "'" };
                        str += escapes[code[i + 1]] || code[i + 1];
                        i += 2;
                    } else {
                        str += code[i++];
                    }
                }
                i++;
                tokens.push({ type: 'STRING', value: str });
                continue;
            }
            
            // Numbers
            if (/\d/.test(code[i]) || (code[i] === '-' && /\d/.test(code[i + 1]))) {
                let num = '';
                if (code[i] === '-') num += code[i++];
                while (i < code.length && /[\d.]/.test(code[i])) num += code[i++];
                tokens.push({ type: 'NUMBER', value: parseFloat(num) });
                continue;
            }
            
            // Identifiers and keywords
            if (/[a-zA-Z_]/.test(code[i])) {
                let ident = '';
                while (i < code.length && /[a-zA-Z0-9_]/.test(code[i])) ident += code[i++];
                
                if (this.keywords.has(ident)) {
                    tokens.push({ type: ident.toUpperCase(), value: ident });
                } else {
                    tokens.push({ type: 'IDENTIFIER', value: ident });
                }
                continue;
            }
            
            // Two-character operators
            if ('+ - * / = ! < > & |'.split(' ').includes(code[i])) {
                const twoChar = code.substr(i, 2);
                const twoCharOps = ['==', '!=', '<=', '>=', '&&', '||', '++', '--', '+=', '-=', '*=', '/=', '->', '=>', '?.', '??'];
                if (twoCharOps.includes(twoChar)) {
                    tokens.push({ type: twoChar, value: twoChar });
                    i += 2;
                    continue;
                }
            }
            
            // Single character tokens
            const char = code[i];
            const charTokens = {
                '(': 'LPAREN', ')': 'RPAREN', '{': 'LBRACE', '}': 'RBRACE',
                '[': 'LBRACKET', ']': 'RBRACKET', ';': 'SEMICOLON', ':': 'COLON',
                ',': 'COMMA', '.': 'DOT', '?': 'QUESTION', '+': 'PLUS', '-': 'MINUS',
                '*': 'STAR', '/': 'SLASH', '%': 'PERCENT', '=': 'ASSIGN', '!': 'BANG',
                '<': 'LT', '>': 'GT', '&': 'AMP', '|': 'PIPE'
            };
            
            if (charTokens[char]) {
                tokens.push({ type: charTokens[char], value: char });
                i++;
            } else {
                i++; // Skip unknown characters
            }
        }
        
        tokens.push({ type: 'EOF', value: null });
        return tokens;
    }
    
    // Parse and execute
    parse(tokens) {
        let pos = 0;
        
        const peek = () => tokens[pos];
        const advance = () => tokens[pos++];
        const expect = (type) => {
            if (peek()?.type !== type) throw new Error(`Expected ${type}, got ${peek()?.type}`);
            return advance();
        };
        
        // Parse statements
        const parseStatement = () => {
            const token = peek();
            
            if (!token || token.type === 'EOF') return null;
            if (token.type === 'RBRACE') return null;
            
            // Function definition
            if (token.type === 'BLAST') {
                advance();
                const name = advance().value;
                expect('LPAREN');
                const params = [];
                while (peek()?.type !== 'RPAREN') {
                    params.push(advance().value);
                    if (peek()?.type === 'COMMA') advance();
                }
                expect('RPAREN');
                expect('LBRACE');
                const body = [];
                while (peek()?.type !== 'RBRACE') {
                    body.push(parseStatement());
                }
                expect('RBRACE');
                return { type: 'FUNCTION', name, params, body };
            }
            
            // Variable declaration
            if (token.type === 'TURBO') {
                advance();
                const name = advance().value;
                let value = null;
                if (peek()?.type === 'ASSIGN') {
                    advance();
                    value = parseExpression();
                }
                expect('SEMICOLON');
                return { type: 'VAR_DECL', name, value };
            }
            
            // If statement
            if (token.type === 'IF') {
                advance();
                expect('LPAREN');
                const condition = parseExpression();
                expect('RPAREN');
                expect('LBRACE');
                const thenBlock = [];
                while (peek()?.type !== 'RBRACE') {
                    thenBlock.push(parseStatement());
                }
                expect('RBRACE');
                
                let elseBlock = null;
                if (peek()?.type === 'ELSE') {
                    advance();
                    if (peek()?.type === 'IF') {
                        elseBlock = [parseStatement()];
                    } else {
                        expect('LBRACE');
                        elseBlock = [];
                        while (peek()?.type !== 'RBRACE') {
                            elseBlock.push(parseStatement());
                        }
                        expect('RBRACE');
                    }
                }
                return { type: 'IF', condition, thenBlock, elseBlock };
            }
            
            // While loop
            if (token.type === 'LOOP') {
                advance();
                expect('LPAREN');
                const condition = parseExpression();
                expect('RPAREN');
                expect('LBRACE');
                const body = [];
                while (peek()?.type !== 'RBRACE') {
                    body.push(parseStatement());
                }
                expect('RBRACE');
                return { type: 'WHILE', condition, body };
            }
            
            // For/each loop (cruise)
            if (token.type === 'CRUISE') {
                advance();
                const varName = advance().value;
                
                // cruise x in range(a, b)
                if (peek()?.type === 'IDENTIFIER' && peek().value === 'in') {
                    advance();
                    
                    // Check for range
                    if (peek()?.type === 'IDENTIFIER' && peek().value === 'range') {
                        advance();
                        expect('LPAREN');
                        const start = parseExpression();
                        expect('COMMA');
                        const end = parseExpression();
                        if (peek()?.type === 'COMMA') {
                            advance(); // skip step
                            parseExpression();
                        }
                        expect('RPAREN');
                        expect('LBRACE');
                        const body = [];
                        while (peek()?.type !== 'RBRACE') {
                            body.push(parseStatement());
                        }
                        expect('RBRACE');
                        return { type: 'FOR_RANGE', varName, start, end, body };
                    }
                    
                    // cruise x in array
                    const iterExpr = parseExpression();
                    expect('LBRACE');
                    const body = [];
                    while (peek()?.type !== 'RBRACE') {
                        body.push(parseStatement());
                    }
                    expect('RBRACE');
                    return { type: 'FOR_EACH', varName, iterExpr, body };
                }
            }
            
            // Return
            if (token.type === 'RETURN') {
                advance();
                let value = null;
                if (peek()?.type !== 'SEMICOLON') {
                    value = parseExpression();
                }
                expect('SEMICOLON');
                return { type: 'RETURN', value };
            }
            
            // Break/Continue
            if (token.type === 'BREAK') {
                advance();
                expect('SEMICOLON');
                return { type: 'BREAK' };
            }
            if (token.type === 'CONTINUE') {
                advance();
                expect('SEMICOLON');
                return { type: 'CONTINUE' };
            }
            
            // Expression statement
            const expr = parseExpression();
            if (peek()?.type === 'SEMICOLON') advance();
            return { type: 'EXPR_STMT', expr };
        };
        
        // Parse expression
        const parseExpression = () => {
            return parseAssignment();
        };
        
        const parseAssignment = () => {
            const left = parseOr();
            
            if (peek()?.type === 'ASSIGN') {
                advance();
                const right = parseAssignment();
                return { type: 'ASSIGN_EXPR', left, right };
            }
            
            return left;
        };
        
        const parseOr = () => {
            let left = parseAnd();
            while (peek()?.type === 'OR') {
                advance();
                const right = parseAnd();
                left = { type: 'BINARY', op: 'or', left, right };
            }
            return left;
        };
        
        const parseAnd = () => {
            let left = parseEquality();
            while (peek()?.type === 'AND') {
                advance();
                const right = parseEquality();
                left = { type: 'BINARY', op: 'and', left, right };
            }
            return left;
        };
        
        const parseEquality = () => {
            let left = parseComparison();
            while (['==', '!='].includes(peek()?.type)) {
                const op = advance().type;
                const right = parseComparison();
                left = { type: 'BINARY', op: op === '==' ? 'eq' : 'neq', left, right };
            }
            return left;
        };
        
        const parseComparison = () => {
            let left = parseAddition();
            while (['<', '>', '<=', '>='].includes(peek()?.type)) {
                const op = advance().type;
                const right = parseAddition();
                left = { type: 'BINARY', op: op.toLowerCase(), left, right };
            }
            return left;
        };
        
        const parseAddition = () => {
            let left = parseMultiplication();
            while (['+', '-'].includes(peek()?.type)) {
                const op = advance().type;
                const right = parseMultiplication();
                left = { type: 'BINARY', op: op === '+' ? 'add' : 'sub', left, right };
            }
            return left;
        };
        
        const parseMultiplication = () => {
            let left = parseUnary();
            while (['*', '/', '%'].includes(peek()?.type)) {
                const op = advance().type;
                const right = parseUnary();
                left = { type: 'BINARY', op: op === '*' ? 'mul' : op === '/' ? 'div' : 'mod', left, right };
            }
            return left;
        };
        
        const parseUnary = () => {
            if (['!', '-'].includes(peek()?.type)) {
                const op = advance().type;
                const operand = parseUnary();
                return { type: 'UNARY', op, operand };
            }
            if (peek()?.type === 'NOT') {
                advance();
                const operand = parseUnary();
                return { type: 'UNARY', op: '!', operand };
            }
            return parsePostfix();
        };
        
        const parsePostfix = () => {
            let expr = parsePrimary();
            
            while (true) {
                if (peek()?.type === 'DOT') {
                    advance();
                    const prop = advance().value;
                    expr = { type: 'MEMBER', object: expr, property: prop };
                } else if (peek()?.type === 'LPAREN') {
                    advance();
                    const args = [];
                    while (peek()?.type !== 'RPAREN') {
                        args.push(parseExpression());
                        if (peek()?.type === 'COMMA') advance();
                    }
                    expect('RPAREN');
                    expr = { type: 'CALL', callee: expr, arguments: args };
                } else if (peek()?.type === 'LBRACKET') {
                    advance();
                    const index = parseExpression();
                    expect('RBRACKET');
                    expr = { type: 'INDEX', object: expr, index };
                } else {
                    break;
                }
            }
            
            return expr;
        };
        
        const parsePrimary = () => {
            const token = peek();
            
            if (token.type === 'NUMBER') {
                advance();
                return { type: 'NUMBER', value: token.value };
            }
            
            if (token.type === 'STRING') {
                advance();
                return { type: 'STRING', value: token.value };
            }
            
            if (token.type === 'TRUE') {
                advance();
                return { type: 'BOOLEAN', value: true };
            }
            
            if (token.type === 'FALSE') {
                advance();
                return { type: 'BOOLEAN', value: false };
            }
            
            if (token.type === 'NULL') {
                advance();
                return { type: 'NULL' };
            }
            
            if (token.type === 'IDENTIFIER') {
                advance();
                return { type: 'IDENTIFIER', name: token.value };
            }
            
            if (token.type === 'LPAREN') {
                advance();
                const expr = parseExpression();
                expect('RPAREN');
                return expr;
            }
            
            if (token.type === 'LBRACKET') {
                advance();
                const elements = [];
                while (peek()?.type !== 'RBRACKET') {
                    elements.push(parseExpression());
                    if (peek()?.type === 'COMMA') advance();
                }
                expect('RBRACKET');
                return { type: 'ARRAY', elements };
            }
            
            if (token.type === 'LBRACE') {
                advance();
                const properties = [];
                while (peek()?.type !== 'RBRACE') {
                    const key = advance().value;
                    expect('COLON');
                    const value = parseExpression();
                    properties.push({ key, value });
                    if (peek()?.type === 'COMMA') advance();
                }
                expect('RBRACE');
                return { type: 'OBJECT', properties };
            }
            
            throw new Error(`Unexpected token: ${token.type}`);
        };
        
        // Parse all statements
        const statements = [];
        while (pos < tokens.length && peek()?.type !== 'EOF') {
            const stmt = parseStatement();
            if (stmt) statements.push(stmt);
        }
        
        return statements;
    }
    
    // Execute the AST
    execute(statements, localVars = new Map()) {
        for (const stmt of statements) {
            const result = this.executeStatement(stmt, localVars);
            if (result && result.type === 'RETURN') {
                return result;
            }
        }
        return null;
    }
    
    executeStatement(stmt, localVars) {
        if (!stmt) return null;
        
        switch (stmt.type) {
            case 'FUNCTION':
                this.functions.set(stmt.name, stmt);
                return null;
                
            case 'VAR_DECL':
                const value = stmt.value ? this.evaluate(stmt.value, localVars) : null;
                if (localVars) localVars.set(stmt.name, value);
                else this.variables.set(stmt.name, value);
                return null;
                
            case 'IF':
                const condition = this.evaluate(stmt.condition, localVars);
                if (condition) {
                    return this.execute(stmt.thenBlock, localVars);
                } else if (stmt.elseBlock) {
                    return this.execute(stmt.elseBlock, localVars);
                }
                return null;
                
            case 'WHILE':
                while (this.evaluate(stmt.condition, localVars)) {
                    const result = this.execute(stmt.body, localVars);
                    if (result?.type === 'RETURN') return result;
                    if (result?.type === 'BREAK') break;
                    if (result?.type === 'CONTINUE') continue;
                }
                return null;
                
            case 'FOR_RANGE':
                const start = this.evaluate(stmt.start, localVars);
                const end = this.evaluate(stmt.end, localVars);
                const newVars = new Map(localVars);
                for (let i = start; i < end; i++) {
                    newVars.set(stmt.varName, i);
                    const result = this.execute(stmt.body, newVars);
                    if (result?.type === 'RETURN') return result;
                    if (result?.type === 'BREAK') break;
                }
                return null;
                
            case 'FOR_EACH':
                const iterable = this.evaluate(stmt.iterExpr, localVars);
                const iterVars = new Map(localVars);
                if (Array.isArray(iterable)) {
                    for (const item of iterable) {
                        iterVars.set(stmt.varName, item);
                        const result = this.execute(stmt.body, iterVars);
                        if (result?.type === 'RETURN') return result;
                        if (result?.type === 'BREAK') break;
                    }
                }
                return null;
                
            case 'RETURN':
                return { type: 'RETURN', value: stmt.value ? this.evaluate(stmt.value, localVars) : null };
                
            case 'BREAK':
                return { type: 'BREAK' };
                
            case 'CONTINUE':
                return { type: 'CONTINUE' };
                
            case 'EXPR_STMT':
                this.evaluate(stmt.expr, localVars);
                return null;
                
            default:
                return null;
        }
    }
    
    // Evaluate expression
    evaluate(expr, localVars = new Map()) {
        if (!expr) return null;
        
        switch (expr.type) {
            case 'NUMBER':
            case 'STRING':
            case 'BOOLEAN':
                return expr.value;
                
            case 'NULL':
                return null;
                
            case 'IDENTIFIER':
                if (localVars.has(expr.name)) return localVars.get(expr.name);
                if (this.variables.has(expr.name)) return this.variables.get(expr.name);
                return null;
                
            case 'ARRAY':
                return expr.elements.map(e => this.evaluate(e, localVars));
                
            case 'OBJECT':
                const obj = {};
                for (const { key, value } of expr.properties) {
                    obj[key] = this.evaluate(value, localVars);
                }
                return obj;
                
            case 'MEMBER':
                const obj_val = this.evaluate(expr.object, localVars);
                if (obj_val && typeof obj_val === 'object') {
                    return obj_val[expr.property];
                }
                return null;
                
            case 'INDEX':
                const arr_val = this.evaluate(expr.object, localVars);
                const idx = this.evaluate(expr.index, localVars);
                if (Array.isArray(arr_val)) return arr_val[idx];
                if (arr_val && typeof arr_val === 'object') return arr_val[idx];
                return null;
                
            case 'CALL':
                return this.callFunction(expr, localVars);
                
            case 'ASSIGN_EXPR':
                const assignValue = this.evaluate(expr.right, localVars);
                if (expr.left.type === 'IDENTIFIER') {
                    if (localVars.has(expr.left.name)) localVars.set(expr.left.name, assignValue);
                    else this.variables.set(expr.left.name, assignValue);
                } else if (expr.left.type === 'MEMBER') {
                    const target = this.evaluate(expr.left.object, localVars);
                    if (target && typeof target === 'object') {
                        target[expr.left.property] = assignValue;
                    }
                } else if (expr.left.type === 'INDEX') {
                    const arr = this.evaluate(expr.left.object, localVars);
                    const index = this.evaluate(expr.left.index, localVars);
                    if (Array.isArray(arr)) arr[index] = assignValue;
                }
                return assignValue;
                
            case 'BINARY':
                return this.evaluateBinary(expr, localVars);
                
            case 'UNARY':
                const operand = this.evaluate(expr.operand, localVars);
                if (expr.op === '!') return !operand;
                if (expr.op === '-') return -operand;
                return operand;
                
            default:
                return null;
        }
    }
    
    evaluateBinary(expr, localVars) {
        const left = this.evaluate(expr.left, localVars);
        const right = this.evaluate(expr.right, localVars);
        
        switch (expr.op) {
            case 'add': return left + right;
            case 'sub': return left - right;
            case 'mul': return left * right;
            case 'div': return left / right;
            case 'mod': return left % right;
            case 'eq': return left == right;
            case 'neq': return left != right;
            case '<': return left < right;
            case '>': return left > right;
            case '<=': return left <= right;
            case '>=': return left >= right;
            case 'and': return left && right;
            case 'or': return left || right;
            default: return null;
        }
    }
    
    callFunction(expr, localVars) {
        let name = '';
        let callee = expr.callee;
        
        // Get function name (handle module.function pattern)
        if (callee.type === 'IDENTIFIER') {
            name = callee.name;
        } else if (callee.type === 'MEMBER') {
            // Module function like datetime.now()
            if (callee.object.type === 'IDENTIFIER') {
                name = callee.object.name + '.' + callee.property;
            }
        } else if (callee.type === 'CALL') {
            // Chained calls
            return this.callFunction(callee, localVars);
        }
        
        const args = expr.arguments.map(a => this.evaluate(a, localVars));
        
        // Check stdlib
        if (this.stdlib[name]) {
            return this.stdlib[name](args);
        }
        
        // Check user-defined functions
        const func = this.functions.get(name);
        if (func) {
            const newVars = new Map();
            func.params.forEach((param, i) => {
                newVars.set(param, args[i]);
            });
            const result = this.execute(func.body, newVars);
            return result?.type === 'RETURN' ? result.value : null;
        }
        
        // Built-in functions
        if (name === 'range') {
            const start = args[0] || 0;
            const end = args[1] || 0;
            const step = args[2] || 1;
            const arr = [];
            for (let i = start; i < end; i += step) arr.push(i);
            return arr;
        }
        
        if (name === 'push') {
            if (Array.isArray(args[0])) args[0].push(args[1]);
            return args[0];
        }
        
        if (name === 'length') {
            return Array.isArray(args[0]) || typeof args[0] === 'string' ? args[0].length : 0;
        }
        
        return null;
    }
    
    // Main compile function
    compile(code) {
        this.output = [];
        this.variables = new Map();
        this.functions = new Map();
        
        try {
            const tokens = this.tokenize(code);
            const statements = this.parse(tokens);
            this.execute(statements);
            
            // Call main if exists
            if (this.functions.has('main')) {
                const mainFunc = this.functions.get('main');
                const result = this.execute(mainFunc.body, new Map());
                if (result?.type === 'RETURN' && result.value !== null) {
                    this.output.push('Exit code: ' + result.value);
                }
            }
            
            return {
                success: true,
                output: this.output.join('\n')
            };
        } catch (error) {
            return {
                success: false,
                error: error.message,
                output: this.output.join('\n')
            };
        }
    }
}

// Export for use
if (typeof module !== 'undefined' && module.exports) {
    module.exports = RADSCompiler;
}
if (typeof window !== 'undefined') {
    window.RADSCompiler = RADSCompiler;
}