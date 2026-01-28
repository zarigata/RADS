use tower_lsp::lsp_types::*;
use super::refactor::RefactorEngine;
use std::collections::HashMap;

pub struct CodeActionProvider {
    refactor_engine: RefactorEngine,
}

impl CodeActionProvider {
    pub fn new() -> Self {
        Self {
            refactor_engine: RefactorEngine::new(),
        }
    }

    pub fn register_document(&mut self, uri: &str, text: &str) {
        self.refactor_engine.register_document(uri, text);
    }

    pub fn unregister_document(&mut self, uri: &str) {
        self.refactor_engine.unregister_document(uri);
    }

    pub fn get_code_actions(
        &self,
        uri: &str,
        range: Range,
        diagnostics: Option<Vec<Diagnostic>>,
    ) -> Vec<CodeAction> {
        let mut actions = Vec::new();

        actions.extend(self.get_quick_fixes(uri, range, diagnostics.as_ref()));
        actions.extend(self.get_refactorings(uri, range));

        actions
    }

    fn get_quick_fixes(
        &self,
        uri: &str,
        range: Range,
        diagnostics: Option<&Vec<Diagnostic>>,
    ) -> Vec<CodeAction> {
        let mut fixes = Vec::new();

        if let Some(diags) = diagnostics {
            for diag in diags {
                if !self.range_overlaps(&range, &diag.range) {
                    continue;
                }

                if diag.message.contains("Possible assignment") {
                    fixes.push(CodeAction {
                        title: "Replace = with ==".to_string(),
                        kind: Some(CodeActionKind::QUICK_FIX),
                        diagnostics: Some(vec![diag.clone()]),
                        is_preferred: Some(true),
                        edit: Some(WorkspaceEdit {
                            document_changes: Some(DocumentChanges::Edits(vec![TextDocumentEdit {
                                text_document: OptionalVersionedTextDocumentIdentifier {
                                    uri: Url::parse(uri).unwrap(),
                                    version: None,
                                },
                                edits: vec![OneOf::Left(TextEdit {
                                    range: diag.range.clone(),
                                    new_text: "==".to_string(),
                                })],
                            }])),
                            ..Default::default()
                        }),
                        ..Default::default()
                    });
                }

                if diag.message.contains("Unknown io function") {
                    fixes.push(CodeAction {
                        title: "Change to io.print".to_string(),
                        kind: Some(CodeActionKind::QUICK_FIX),
                        diagnostics: Some(vec![diag.clone()]),
                        is_preferred: Some(true),
                        edit: Some(WorkspaceEdit {
                            document_changes: Some(DocumentChanges::Edits(vec![TextDocumentEdit {
                                text_document: OptionalVersionedTextDocumentIdentifier {
                                    uri: Url::parse(uri).unwrap(),
                                    version: None,
                                },
                                edits: vec![OneOf::Left(TextEdit {
                                    range: diag.range.clone(),
                                    new_text: "io.print".to_string(),
                                })],
                            }])),
                            ..Default::default()
                        }),
                        ..Default::default()
                    });
                }

                if diag.message.contains("Math.sin requires radians") {
                    fixes.push(CodeAction {
                        title: "Wrap in math.radians()".to_string(),
                        kind: Some(CodeActionKind::QUICK_FIX),
                        diagnostics: Some(vec![diag.clone()]),
                        is_preferred: Some(true),
                        edit: Some(WorkspaceEdit {
                            document_changes: Some(DocumentChanges::Edits(vec![TextDocumentEdit {
                                text_document: OptionalVersionedTextDocumentIdentifier {
                                    uri: Url::parse(uri).unwrap(),
                                    version: None,
                                },
                                edits: vec![OneOf::Left(TextEdit {
                                    range: diag.range.clone(),
                                    new_text: "math.radians(".to_string(),
                                })],
                            }])),
                            ..Default::default()
                        }),
                        ..Default::default()
                    });

                    fixes.push(CodeAction {
                        title: "Convert to degrees manually".to_string(),
                        kind: Some(CodeActionKind::QUICK_FIX),
                        diagnostics: Some(vec![diag.clone()]),
                        edit: Some(WorkspaceEdit {
                            document_changes: Some(DocumentChanges::Edits(vec![TextDocumentEdit {
                                text_document: OptionalVersionedTextDocumentIdentifier {
                                    uri: Url::parse(uri).unwrap(),
                                    version: None,
                                },
                                edits: vec![OneOf::Left(TextEdit {
                                    range: diag.range.clone(),
                                    new_text: format!("math.sin(deg * ({:?} / 180))", std::f64::consts::PI),
                                })],
                            }])),
                            ..Default::default()
                        }),
                        ..Default::default()
                    });
                }

                if diag.message.contains("Missing return statement") {
                    fixes.push(CodeAction {
                        title: "Add return statement".to_string(),
                        kind: Some(CodeActionKind::QUICK_FIX),
                        diagnostics: Some(vec![diag.clone()]),
                        is_preferred: Some(true),
                        edit: Some(WorkspaceEdit {
                            document_changes: Some(DocumentChanges::Edits(vec![TextDocumentEdit {
                                text_document: OptionalVersionedTextDocumentIdentifier {
                                    uri: Url::parse(uri).unwrap(),
                                    version: None,
                                },
                                edits: vec![OneOf::Left(TextEdit {
                                    range: diag.range.clone(),
                                    new_text: "\n    return null;".to_string(),
                                })],
                            }])),
                            ..Default::default()
                        }),
                        ..Default::default()
                    });
                }

                if diag.message.contains("Unused variable") {
                    let var_name = diag.range.start;
                    fixes.push(CodeAction {
                        title: "Remove unused variable".to_string(),
                        kind: Some(CodeActionKind::QUICK_FIX),
                        diagnostics: Some(vec![diag.clone()]),
                        edit: Some(WorkspaceEdit {
                            document_changes: Some(DocumentChanges::Edits(vec![TextDocumentEdit {
                                text_document: OptionalVersionedTextDocumentIdentifier {
                                    uri: Url::parse(uri).unwrap(),
                                    version: None,
                                },
                                edits: vec![OneOf::Left(TextEdit {
                                    range: Range {
                                        start: Position {
                                            line: var_name.line,
                                            character: 0,
                                        },
                                        end: Position {
                                            line: var_name.line + 1,
                                            character: 0,
                                        },
                                    },
                                    new_text: String::new(),
                                })],
                            }])),
                            ..Default::default()
                        }),
                        ..Default::default()
                    });

                    fixes.push(CodeAction {
                        title: "Prefix with _ to indicate intentionally unused".to_string(),
                        kind: Some(CodeActionKind::QUICK_FIX),
                        diagnostics: Some(vec![diag.clone()]),
                        edit: Some(WorkspaceEdit {
                            document_changes: Some(DocumentChanges::Edits(vec![TextDocumentEdit {
                                text_document: OptionalVersionedTextDocumentIdentifier {
                                    uri: Url::parse(uri).unwrap(),
                                    version: None,
                                },
                                edits: vec![OneOf::Left(TextEdit {
                                    range: Range {
                                        start: Position {
                                            line: var_name.line,
                                            character: var_name.character,
                                        },
                                        end: Position {
                                            line: var_name.line,
                                            character: var_name.character,
                                        },
                                    },
                                    new_text: "_".to_string(),
                                })],
                            }])),
                            ..Default::default()
                        }),
                        ..Default::default()
                    });
                }
            }
        }

        fixes
    }

    fn get_refactorings(&self, uri: &str, range: Range) -> Vec<CodeAction> {
        let mut refactorings = Vec::new();

        refactorings.push(CodeAction {
            title: "Extract Function".to_string(),
            kind: Some(CodeActionKind::REFACTOR),
            edit: None,
            command: Some(Command {
                title: "Extract Function".to_string(),
                command: "rads.extractFunction".to_string(),
                arguments: Some(vec![
                    serde_json::to_value(uri).unwrap(),
                    serde_json::to_value(&range).unwrap(),
                ]),
            }),
            ..Default::default()
        });

        refactorings.push(CodeAction {
            title: "Extract Variable".to_string(),
            kind: Some(CodeActionKind::REFACTOR),
            edit: None,
            command: Some(Command {
                title: "Extract Variable".to_string(),
                command: "rads.extractVariable".to_string(),
                arguments: Some(vec![
                    serde_json::to_value(uri).unwrap(),
                    serde_json::to_value(&range).unwrap(),
                ]),
            }),
            ..Default::default()
        });

        refactorings.push(CodeAction {
            title: "Inline Function".to_string(),
            kind: Some(CodeActionKind::REFACTOR),
            edit: None,
            command: Some(Command {
                title: "Inline Function".to_string(),
                command: "rads.inlineFunction".to_string(),
                arguments: Some(vec![
                    serde_json::to_value(uri).unwrap(),
                    serde_json::to_value(&range.start).unwrap(),
                ]),
            }),
            ..Default::default()
        });

        refactorings.push(CodeAction {
            title: "Add Async to Function".to_string(),
            kind: Some(CodeActionKind::REFACTOR),
            edit: None,
            command: Some(Command {
                title: "Add Async to Function".to_string(),
                command: "rads.addAsync".to_string(),
                arguments: Some(vec![
                    serde_json::to_value(uri).unwrap(),
                    serde_json::to_value(&range.start).unwrap(),
                ]),
            }),
            ..Default::default()
        });

        refactorings.push(CodeAction {
            title: "Add Return Type".to_string(),
            kind: Some(CodeActionKind::REFACTOR),
            edit: None,
            command: Some(Command {
                title: "Add Return Type".to_string(),
                command: "rads.addReturnType".to_string(),
                arguments: Some(vec![
                    serde_json::to_value(uri).unwrap(),
                    serde_json::to_value(&range.start).unwrap(),
                ]),
            }),
            ..Default::default()
        });

        refactorings.push(CodeAction {
            title: "Simplify Expression".to_string(),
            kind: Some(CodeActionKind::REFACTOR),
            edit: None,
            command: Some(Command {
                title: "Simplify Expression".to_string(),
                command: "rads.simplifyExpression".to_string(),
                arguments: Some(vec![
                    serde_json::to_value(uri).unwrap(),
                    serde_json::to_value(&range).unwrap(),
                ]),
            }),
            ..Default::default()
        });

        refactorings.push(CodeAction {
            title: "Add Error Handling".to_string(),
            kind: Some(CodeActionKind::REFACTOR),
            edit: None,
            command: Some(Command {
                title: "Add Error Handling".to_string(),
                command: "rads.addErrorHandling".to_string(),
                arguments: Some(vec![
                    serde_json::to_value(uri).unwrap(),
                    serde_json::to_value(&range).unwrap(),
                ]),
            }),
            ..Default::default()
        });

        refactorings
    }

    fn range_overlaps(&self, range1: &Range, range2: &Range) -> bool {
        range1.start.line <= range2.end.line
            && range1.end.line >= range2.start.line
    }
}

pub fn organize_imports(uri: &str, text: &str) -> WorkspaceEdit {
    let mut imports = Vec::new();
    let mut other_lines = Vec::new();

    for line in text.lines() {
        if line.trim().starts_with("import ") {
            imports.push(line.to_string());
        } else {
            other_lines.push(line.to_string());
        }
    }

    imports.sort();
    imports.dedup();

    let new_text = format!("{}\n\n{}", imports.join("\n"), other_lines.join("\n"));

    WorkspaceEdit {
        document_changes: Some(DocumentChanges::Edits(vec![TextDocumentEdit {
            text_document: OptionalVersionedTextDocumentIdentifier {
                uri: Url::parse(uri).unwrap(),
                version: None,
            },
            edits: vec![OneOf::Left(TextEdit {
                range: Range {
                    start: Position { line: 0, character: 0 },
                    end: Position {
                        line: text.lines().count() as u32,
                        character: 0,
                    },
                },
                new_text,
            })],
        }])),
        ..Default::default()
    }
}

pub fn format_document(uri: &str, text: &str) -> WorkspaceEdit {
    let mut formatted_lines = Vec::new();

    for (i, line) in text.lines().enumerate() {
        let indented = match line.trim_start() {
            l if l.starts_with("}") => format!("{}}}", "  ".repeat(i as usize.saturating_sub(1))),
            l if l.starts_with("{") || l.starts_with("blast ") => format!("{}{}", "  ".repeat(i as usize), l),
            l if l.starts_with("turbo ") || l.starts_with("if ") || l.starts_with("elif ") || l.starts_with("else ") || l.starts_with("loop ") || l.starts_with("cruise ") => {
                format!("{}{}", "  ".repeat(i as usize), l)
            }
            l => format!("{}{}", "  ".repeat(i as usize.saturating_sub(1)), l),
        };

        formatted_lines.push(if line.trim().is_empty() {
            String::new()
        } else {
            indented
        });
    }

    WorkspaceEdit {
        document_changes: Some(DocumentChanges::Edits(vec![TextDocumentEdit {
            text_document: OptionalVersionedTextDocumentIdentifier {
                uri: Url::parse(uri).unwrap(),
                version: None,
            },
            edits: vec![OneOf::Left(TextEdit {
                range: Range {
                    start: Position { line: 0, character: 0 },
                    end: Position {
                        line: text.lines().count() as u32,
                        character: 0,
                    },
                },
                new_text: formatted_lines.join("\n"),
            })],
        }])),
        ..Default::default()
    }
}

pub fn fix_all_errors(uri: &str, text: &str, diagnostics: &[Diagnostic]) -> Option<WorkspaceEdit> {
    if diagnostics.is_empty() {
        return None;
    }

    let mut edits = Vec::new();

    for diag in diagnostics {
        if diag.message.contains("Possible assignment") {
            edits.push(TextEdit {
                range: diag.range.clone(),
                new_text: "==".to_string(),
            });
        }

        if diag.message.contains("Unknown io function") {
            edits.push(TextEdit {
                range: diag.range.clone(),
                new_text: "io.print".to_string(),
            });
        }
    }

    if edits.is_empty() {
        return None;
    }

    Some(WorkspaceEdit {
        document_changes: Some(DocumentChanges::Edits(vec![TextDocumentEdit {
            text_document: OptionalVersionedTextDocumentIdentifier {
                uri: Url::parse(uri).unwrap(),
                version: None,
            },
            edits: edits.into_iter().map(OneOf::Left).collect(),
        }])),
        ..Default::default()
    })
}
