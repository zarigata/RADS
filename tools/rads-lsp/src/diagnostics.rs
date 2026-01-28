use tower_lsp::lsp_types::*;
use std::collections::HashMap;
use regex::Regex;

pub struct DiagnosticEngine {
    documents: HashMap<String, String>,
    cached_diagnostics: HashMap<String, Vec<Diagnostic>>,
}

impl DiagnosticEngine {
    pub fn new() -> Self {
        Self {
            documents: HashMap::new(),
            cached_diagnostics: HashMap::new(),
        }
    }

    pub fn register_document(&mut self, uri: &str, text: &str) {
        self.documents.insert(uri.to_string(), text.to_string());
        self.invalidate(uri);
    }

    pub fn unregister_document(&mut self, uri: &str) {
        self.documents.remove(uri);
        self.cached_diagnostics.remove(uri);
    }

    pub fn invalidate(&mut self, uri: &str) {
        self.cached_diagnostics.remove(uri);
    }

    pub fn get_diagnostics(&mut self, uri: &str) -> Vec<Diagnostic> {
        if let Some(cached) = self.cached_diagnostics.get(uri) {
            return cached.clone();
        }

        let text = self.documents.get(uri).cloned().unwrap_or_default();
        let diagnostics = self.analyze(&text);

        self.cached_diagnostics.insert(uri.to_string(), diagnostics.clone());
        diagnostics
    }

    fn analyze(&self, text: &str) -> Vec<Diagnostic> {
        let mut diagnostics = Vec::new();

        diagnostics.extend(self.check_syntax_errors(text));
        diagnostics.extend(self.check_semantic_errors(text));
        diagnostics.extend(self.check_type_errors(text));
        diagnostics.extend(self.check_style_issues(text));
        diagnostics.extend(self.check_performance_issues(text));

        diagnostics
    }

    fn check_syntax_errors(&self, text: &str) -> Vec<Diagnostic> {
        let mut diagnostics = Vec::new();

        for (line_num, line) in text.lines().enumerate() {
            let open_parens = line.chars().filter(|&c| c == '(').count();
            let close_parens = line.chars().filter(|&c| c == ')').count();

            if open_parens != close_parens {
                diagnostics.push(Diagnostic {
                    range: Range {
                        start: Position { line: line_num as u32, character: 0 },
                        end: Position { line: line_num as u32, character: line.len() as u32 },
                    },
                    severity: Some(DiagnosticSeverity::Error),
                    message: format!(
                        "Mismatched parentheses: {} opening, {} closing",
                        open_parens, close_parens
                    ),
                    source: Some("rads-lsp".to_string()),
                    ..Default::default()
                });
            }

            let open_braces = line.chars().filter(|&c| c == '{').count();
            let close_braces = line.chars().filter(|&c| c == '}').count();

            if open_braces != close_braces {
                diagnostics.push(Diagnostic {
                    range: Range {
                        start: Position { line: line_num as u32, character: 0 },
                        end: Position { line: line_num as u32, character: line.len() as u32 },
                    },
                    severity: Some(DiagnosticSeverity::Error),
                    message: format!(
                        "Mismatched braces: {} opening, {} closing",
                        open_braces, close_braces
                    ),
                    source: Some("rads-lsp".to_string()),
                    ..Default::default()
                });
            }

            if line.contains("blast ") && !line.contains('(') && !line.contains('{') {
                diagnostics.push(Diagnostic {
                    range: Range {
                        start: Position { line: line_num as u32, character: line.find("blast ").map(|p| p as u32).unwrap_or(0) },
                        end: Position { line: line_num as u32, character: line.len() as u32 },
                    },
                    severity: Some(DiagnosticSeverity::Error),
                    message: "Function definition missing parameters or body".to_string(),
                    source: Some("rads-lsp".to_string()),
                    ..Default::default()
                });
            }

            if line.contains("turbo ") && !line.contains('=') {
                diagnostics.push(Diagnostic {
                    range: Range {
                        start: Position { line: line_num as u32, character: line.find("turbo ").map(|p| p as u32).unwrap_or(0) },
                        end: Position { line: line_num as u32, character: line.len() as u32 },
                    },
                    severity: Some(DiagnosticSeverity::Error),
                    message: "Variable declaration missing assignment".to_string(),
                    source: Some("rads-lsp".to_string()),
                    ..Default::default()
                });
            }
        }

        diagnostics
    }

    fn check_semantic_errors(&self, text: &str) -> Vec<Diagnostic> {
        let mut diagnostics = Vec::new();

        let defined_functions: Vec<&str> = text
            .lines()
            .filter_map(|line| {
                if line.starts_with("blast ") {
                    line.strip_prefix("blast ")
                        .and_then(|s| s.split('(').next())
                        .map(|name| name.trim())
                } else {
                    None
                }
            })
            .collect();

        for (line_num, line) in text.lines().enumerate() {
            let call_re = Regex::new(r"(\w+)\(").unwrap();

            for caps in call_re.captures_iter(line) {
                if let Some(func_name) = caps.get(1) {
                    let name = func_name.as_str();
                    if name == "if" || name == "else" || name == "loop" || name == "cruise" {
                        continue;
                    }

                    if !defined_functions.contains(&name) && !self.is_stdlib_function(name) {
                        diagnostics.push(Diagnostic {
                            range: Range {
                                start: Position {
                                    line: line_num as u32,
                                    character: func_name.start() as u32,
                                },
                                end: Position {
                                    line: line_num as u32,
                                    character: func_name.end() as u32,
                                },
                            },
                            severity: Some(DiagnosticSeverity::Error),
                            message: format!("Undefined function: {}", name),
                            source: Some("rads-lsp".to_string()),
                            ..Default::default()
                        });
                    }
                }
            }
        }

        diagnostics
    }

    fn check_type_errors(&self, text: &str) -> Vec<Diagnostic> {
        let mut diagnostics = Vec::new();

        for (line_num, line) in text.lines().enumerate() {
            if line.contains("=") && !line.contains("==") && !line.contains("!=") && !line.contains("turbo ") && !line.contains("loop ") {
                if let Some(pos) = line.find('=') {
                    diagnostics.push(Diagnostic {
                        range: Range {
                            start: Position { line: line_num as u32, character: pos as u32 },
                            end: Position { line: line_num as u32, character: (pos + 1) as u32 },
                        },
                        severity: Some(DiagnosticSeverity::Warning),
                        message: "Possible assignment (=) in expression, did you mean comparison (==)?".to_string(),
                        source: Some("rads-lsp".to_string()),
                        ..Default::default()
                    });
                }
            }

            if line.contains("io.p") && !line.contains("io.print") && !line.contains("io.println") {
                if let Some(pos) = line.find("io.p") {
                    diagnostics.push(Diagnostic {
                        range: Range {
                            start: Position { line: line_num as u32, character: pos as u32 },
                            end: Position { line: line_num as u32, character: (pos + 4) as u32 },
                        },
                        severity: Some(DiagnosticSeverity::Error),
                        message: "Unknown io function. Did you mean io.print or io.println?".to_string(),
                        source: Some("rads-lsp".to_string()),
                        ..Default::default()
                    });
                }
            }

            if line.contains("math.sin(") && !line.contains("math.radians") {
                if let Some(pos) = line.find("math.sin(") {
                    diagnostics.push(Diagnostic {
                        range: Range {
                            start: Position { line: line_num as u32, character: pos as u32 },
                            end: Position { line: line_num as u32, character: (pos + 8) as u32 },
                        },
                        severity: Some(DiagnosticSeverity::Warning),
                        message: "math.sin() expects radians. Consider wrapping with math.radians()".to_string(),
                        source: Some("rads-lsp".to_string()),
                        ..Default::default()
                    });
                }
            }

            if line.contains("math.cos(") && !line.contains("math.radians") {
                if let Some(pos) = line.find("math.cos(") {
                    diagnostics.push(Diagnostic {
                        range: Range {
                            start: Position { line: line_num as u32, character: pos as u32 },
                            end: Position { line: line_num as u32, character: (pos + 8) as u32 },
                        },
                        severity: Some(DiagnosticSeverity::Warning),
                        message: "math.cos() expects radians. Consider wrapping with math.radians()".to_string(),
                        source: Some("rads-lsp".to_string()),
                        ..Default::default()
                    });
                }
            }
        }

        diagnostics
    }

    fn check_style_issues(&self, text: &str) -> Vec<Diagnostic> {
        let mut diagnostics = Vec::new();

        for (line_num, line) in text.lines().enumerate() {
            if line.len() > 100 {
                diagnostics.push(Diagnostic {
                    range: Range {
                        start: Position { line: line_num as u32, character: 100 },
                        end: Position { line: line_num as u32, character: line.len() as u32 },
                    },
                    severity: Some(DiagnosticSeverity::Information),
                    message: format!("Line too long ({} characters), consider splitting", line.len()),
                    source: Some("rads-lsp".to_string()),
                    code: Some("line-too-long".to_string()),
                    ..Default::default()
                });
            }

            if line.contains("if ") && line.contains("if ") {
                let first_if = line.find("if ").unwrap();
                let second_if = line[first_if + 3..].find("if ");

                if let Some(offset) = second_if {
                    diagnostics.push(Diagnostic {
                        range: Range {
                            start: Position { line: line_num as u32, character: (first_if + offset) as u32 },
                            end: Position { line: line_num as u32, character: (first_if + offset + 2) as u32 },
                        },
                        severity: Some(DiagnosticSeverity::Warning),
                        message: "Nested if statement. Consider using elif or restructuring logic".to_string(),
                        source: Some("rads-lsp".to_string()),
                        code: Some("nested-if".to_string()),
                        ..Default::default()
                    });
                }
            }

            if line.chars().filter(|&c| c == ' ').count() > 10 {
                let spaces = line.chars().take_while(|&c| c == ' ').count();

                if spaces % 2 != 0 && spaces > 0 {
                    diagnostics.push(Diagnostic {
                        range: Range {
                            start: Position { line: line_num as u32, character: 0 },
                            end: Position { line: line_num as u32, character: spaces as u32 },
                        },
                        severity: Some(DiagnosticSeverity::Information),
                        message: format!("Indentation is {} spaces, consider using multiples of 2", spaces),
                        source: Some("rads-lsp".to_string()),
                        code: Some("indentation".to_string()),
                        ..Default::default()
                    });
                }
            }
        }

        diagnostics
    }

    fn check_performance_issues(&self, text: &str) -> Vec<Diagnostic> {
        let mut diagnostics = Vec::new();

        for (line_num, line) in text.lines().enumerate() {
            if line.contains("io.print") && line.contains("loop ") {
                if let Some(pos) = line.find("io.print") {
                    diagnostics.push(Diagnostic {
                        range: Range {
                            start: Position { line: line_num as u32, character: pos as u32 },
                            end: Position { line: line_num as u32, character: (pos + 8) as u32 },
                        },
                        severity: Some(DiagnosticSeverity::Warning),
                        message: "io.print inside loop may impact performance. Consider buffering or batching".to_string(),
                        source: Some("rads-lsp".to_string()),
                        tags: Some(vec![DiagnosticTag::UNNECESSARY]),
                        ..Default::default()
                    });
                }
            }

            let array_concat = Regex::new(r"array\.concat\([^)]+\)").unwrap();

            for caps in array_concat.captures_iter(line) {
                if let Some(mat) = caps.get(0) {
                    diagnostics.push(Diagnostic {
                        range: Range {
                            start: Position {
                                line: line_num as u32,
                                character: mat.start() as u32,
                            },
                            end: Position {
                                line: line_num as u32,
                                character: mat.end() as u32,
                            },
                        },
                        severity: Some(DiagnosticSeverity::Information),
                        message: "array.concat() creates new arrays. Consider using array.push() in loop".to_string(),
                        source: Some("rads-lsp".to_string()),
                        ..Default::default()
                    });
                }
            }
        }

        diagnostics
    }

    fn is_stdlib_function(&self, name: &str) -> bool {
        let stdlib_modules = vec![
            "io", "math", "string", "array", "json", "fs", "net", "http", "db", "async",
        ];

        for module in stdlib_modules {
            if name.starts_with(module) {
                return true;
            }
        }

        let stdlib_functions = vec![
            "echo", "blast", "turbo", "loop", "cruise", "if", "else", "elif",
            "return", "break", "continue", "import", "export", "spawn", "await",
        ];

        stdlib_functions.contains(&name)
    }
}
