use std::collections::HashMap;
use std::sync::Arc;
use tokio::net::TcpListener;
use tokio_tungstenite::acceptor::TlsAcceptor;
use tokio_tungstenite::tls::Identity;
use tower_lsp::{jsonrpc::Error, LspService, LanguageServer};
use tower_lsp::lsp_types::*;
use serde::{Deserialize, Serialize};
use serde_json::Value;

#[derive(Debug, Serialize, Deserialize)]
struct RadsConfig {
    version: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct CompletionItem {
    label: String,
    kind: CompletionItemKind,
    detail: Option<String>,
    documentation: Option<StringOrMarkupContent>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct CompletionItemKind {
    #[serde(rename = "type")]
    kind: String,
}

struct RadsLanguageServer {
    config: Option<RadsConfig>,
    documents: HashMap<String, String>,
}

impl RadsLanguageServer {
    fn new() -> Self {
        Self {
            config: None,
            documents: HashMap::new(),
        }
    }

    fn get_completions(&self, text: &str, line: usize, column: usize) -> Vec<CompletionItem> {
        let mut completions = Vec::new();

        let line_start = text[..line].rfind('\n').map_or(0, |p| p + 1);
        let prefix = &text[line_start + column..];

        let tokens = self.tokenize_line(&text[line_start..]);

        for token in tokens {
            if token.starts_with(prefix) && token.len() > prefix.len() {
                let kind = self.get_completion_kind(&token);
                completions.push(CompletionItem {
                    label: token.to_string(),
                    kind,
                    detail: Some("RADS Standard Library".to_string()),
                    documentation: Some(StringOrMarkupContent::String(format!("`{}` - {}", token, "RADS function"))),
                });
            }
        }

        completions.sort_by(|a, b| a.label.cmp(&b.label));
        completions
    }

    fn tokenize_line(&self, line: &str) -> Vec<&str> {
        let mut tokens = Vec::new();
        let keywords = vec![
            "blast", "turbo", "loop", "cruise", "if", "else", "elif",
            "return", "break", "continue", "import", "export", "async", "await",
            "spawn", "echo", "db.", "http.", "fs.", "io.", "math.", "net.",
            "string.", "array.", "json.", "struct", "enum", "switch", "case", "default",
        ];

        let words: Vec<&str> = line.split_whitespace().collect();

        for word in words {
            for keyword in &keywords {
                if word.contains(keyword) {
                    tokens.push(*keyword);
                }
            }

            if word.contains('.') {
                let parts: Vec<&str> = word.split('.').collect();
                if parts.len() == 2 {
                    let module = parts[0];
                    let func = parts[1];
                    tokens.push(format!("{}.{}", module, func).as_str());
                }
            }
        }

        tokens.dedup()
    }

    fn get_completion_kind(&self, token: &str) -> CompletionItemKind {
        if token.starts_with("blast") || token.starts_with("turbo") {
            return CompletionItemKind { kind: "Keyword".to_string() };
        }

        if token.contains('.') {
            let parts: Vec<&str> = token.split('.').collect();
            if parts.len() == 2 {
                let module = parts[0];
                if vec!["io", "math", "string", "array", "json", "fs", "net", "http", "db"].contains(&module) {
                    return CompletionItemKind { kind: "Function".to_string() };
                }
            }
        }

        CompletionItemKind { kind: "Text".to_string() }
    }

    fn get_definition(&self, text: &str, line: usize, column: usize) -> Option<Location> {
        let line_start = text[..line].rfind('\n').map_or(0, |p| p + 1);
        let word = self.get_word_at(&text[line_start..], column);

        if word.starts_with("io.") || word.starts_with("math.") || word.starts_with("string.") {
            return Some(Location {
                uri: Url::parse("file:///stdio.rads").unwrap(),
                range: Range {
                    start: Position { line: 0, character: 0 },
                    end: Position { line: 0, character: 0 },
                },
            });
        }

        None
    }

    fn get_word_at(&self, line: &str, column: usize) -> String {
        let mut start = column;
        while start > 0 && !line.chars().nth(start - 1).unwrap_or(' ').is_whitespace() {
            start -= 1;
        }

        let mut end = column;
        while end < line.len() && !line.chars().nth(end).unwrap_or(' ').is_whitespace() {
            end += 1;
        }

        line[start..end].to_string()
    }

    fn get_diagnostics(&self, text: &str) -> Vec<Diagnostic> {
        let mut diagnostics = Vec::new();

        for (line_num, line) in text.lines().enumerate() {
            if line.contains("=") && !line.contains("==") && !line.contains("!=") {
                if let Some(pos) = line.find('=') {
                    diagnostics.push(Diagnostic {
                        range: Range {
                            start: Position { line: line_num as u32, character: pos as u32 },
                            end: Position { line: line_num as u32, character: (pos + 1) as u32 },
                        },
                        severity: Some(DiagnosticSeverity::Warning),
                        message: "Possible assignment (=), did you mean comparison (==)?".to_string(),
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

            if line.contains("math.sin(") {
                if let Some(pos) = line.find("math.sin(") {
                    diagnostics.push(Diagnostic {
                        range: Range {
                            start: Position { line: line_num as u32, character: pos as u32 },
                            end: Position { line: line_num as u32, character: (pos + 8) as u32 },
                        },
                        severity: Some(DiagnosticSeverity::Warning),
                        message: "Math.sin requires radians. Use math.radians(degree) first.".to_string(),
                        source: Some("rads-lsp".to_string()),
                        ..Default::default()
                    });
                }
            }
        }

        diagnostics
    }

    fn get_document_symbols(&self, text: &str) -> Vec<SymbolInformation> {
        let mut symbols = Vec::new();

        for (line_num, line) in text.lines().enumerate() {
            if line.starts_with("blast ") {
                if let Some(start) = line.find("blast ") {
                    let func_name: String = line[start + 6..]
                        .chars()
                        .take_while(|c| *c != '(')
                        .collect();
                    symbols.push(SymbolInformation {
                        name: func_name.clone(),
                        kind: SymbolKind::Function,
                        location: Location {
                            uri: Url::parse("file:///current.rads").unwrap(),
                            range: Range {
                                start: Position { line: line_num as u32, character: 0 },
                                end: Position { line: line_num as u32, character: line.len() as u32 },
                            },
                        },
                    });
                }
            }

            if line.starts_with("turbo ") {
                if let Some(start) = line.find("turbo ") {
                    let var_name: String = line[start + 6..]
                        .chars()
                        .take_while(|c| *c != '=' && *c != ';')
                        .collect();
                    symbols.push(SymbolInformation {
                        name: var_name.trim().to_string(),
                        kind: SymbolKind::Variable,
                        location: Location {
                            uri: Url::parse("file:///current.rads").unwrap(),
                            range: Range {
                                start: Position { line: line_num as u32, character: 0 },
                                end: Position { line: line_num as u32, character: line.len() as u32 },
                            },
                        },
                    });
                }
            }
        }

        symbols
    }
}

#[tower_lsp::async_trait]
impl LspService for RadsLanguageServer {
    async fn initialize(&self, _: InitializeParams) -> Result<InitializeResult, Error> {
        log::info!("RADS LSP Server initializing");

        let capabilities = ServerCapabilities {
            text_document_sync: Some(TextDocumentSyncCapability::Kind(TextDocumentSyncKind::Full)),
            completion_provider: Some(CompletionOptions {
                resolve_provider: Some(false),
                trigger_characters: Some(vec![".".to_string()]),
                ..Default::default()
            }),
            definition_provider: Some(true),
            hover_provider: Some(HoverProviderCapability::Simple(true)),
            document_symbol_provider: Some(true),
            diagnostic_provider: Some(DiagnosticServerCapabilities::Simple(true)),
            ..Default::default()
        };

        Ok(InitializeResult {
            capabilities,
            server_info: Some(ServerInfo {
                name: "rads-lsp".to_string(),
                version: "0.1.0".to_string(),
            }),
            ..Default::default()
        })
    }

    async fn initialized(&self, _: InitializedParams) {
        log::info!("RADS LSP Server initialized");
    }

    async fn shutdown(&self) -> Result<(), Error> {
        log::info!("RADS LSP Server shutting down");
        Ok(())
    }

    async fn did_open(&self, params: DidOpenTextDocumentParams) {
        let uri = params.text_document.uri.to_string();
        let text = params.text_document.text.clone();
        log::info!("Opened document: {}", uri);
        self.documents.insert(uri, text);
    }

    async fn did_change(&self, params: DidChangeTextDocumentParams) {
        let uri = params.text_document.uri.to_string();
        let content = params
            .content_changes
            .iter()
            .last()
            .map(|c| c.text.clone())
            .unwrap_or_default();
        log::info!("Changed document: {}", uri);
        self.documents.insert(uri, content);
    }

    async fn did_close(&self, params: DidCloseTextDocumentParams) {
        let uri = params.text_document.uri.to_string();
        log::info!("Closed document: {}", uri);
        self.documents.remove(&uri);
    }

    async fn completion(&self, params: CompletionParams) -> Result<Option<CompletionResponse>, Error> {
        let uri = params.text_document_position.text_document.uri.as_str();
        let position = params.text_document_position.position;

        let text = self.documents.get(uri).cloned().unwrap_or_default();
        let line_idx = position.line as usize;

        let lines: Vec<&str> = text.lines().collect();
        let line = lines.get(line_idx).cloned().unwrap_or_default();
        let column = position.character as usize;

        let completions = self.get_completions(&text, line_idx, column);

        if completions.is_empty() {
            return Ok(None);
        }

        let items: Vec<CompletionItem> = completions
            .into_iter()
            .map(|item| serde_json::to_value(&item).unwrap())
            .collect();

        Ok(Some(CompletionResponse::Array(items)))
    }

    async fn goto_definition(&self, params: GotoDefinitionParams) -> Result<Option<GotoDefinitionResponse>, Error> {
        let uri = params.text_document_position_params.text_document.uri.as_str();
        let position = params.text_document_position_params.position;

        let text = self.documents.get(uri).cloned().unwrap_or_default();
        let line_idx = position.line as usize;
        let column = position.character as usize;

        if let Some(location) = self.get_definition(&text, line_idx, column) {
            Ok(Some(GotoDefinitionResponse::Scalar(LocationLink {
                target_uri: location.uri,
                target_range: location.range,
                target_selection_range: None,
            })))
        } else {
            Ok(None)
        }
    }

    async fn hover(&self, params: HoverParams) -> Result<Option<Hover>, Error> {
        let uri = params.text_document_position_params.text_document.uri.as_str();
        let position = params.text_document_position_params.position;

        let text = self.documents.get(uri).cloned().unwrap_or_default();
        let line_idx = position.line as usize;
        let column = position.character as usize;

        let line_start = text[..line_idx].rfind('\n').map_or(0, |p| p + 1);
        let word = self.get_word_at(&text[line_start..], column);

        if word.starts_with("io.print") {
            return Ok(Some(Hover {
                contents: HoverContents::Markup(MarkupContent {
                    kind: MarkupKind::Markdown,
                    value: "**print(text)**\n\nPrints text to stdout".to_string(),
                }),
                range: None,
            }));
        }

        if word.starts_with("math.sin") {
            return Ok(Some(Hover {
                contents: HoverContents::Markup(MarkupContent {
                    kind: MarkupKind::Markdown,
                    value: "**sin(x)**\n\nReturns the sine of x (in radians)".to_string(),
                }),
                range: None,
            }));
        }

        Ok(None)
    }

    async fn document_symbol(&self, params: DocumentSymbolParams) -> Result<Option<DocumentSymbolResponse>, Error> {
        let uri = params.text_document.uri.as_str();
        let text = self.documents.get(uri).cloned().unwrap_or_default();

        let symbols = self.get_document_symbols(&text);
        let document_symbols: Vec<DocumentSymbol> = symbols
            .into_iter()
            .map(|sym| DocumentSymbol {
                name: sym.name.clone(),
                detail: None,
                kind: Some(match sym.kind {
                    SymbolKind::Function => SymbolKind::Function,
                    SymbolKind::Variable => SymbolKind::Variable,
                    _ => SymbolKind::Struct,
                }),
                tags: None,
                deprecated: None,
                range: Some(sym.location.range),
                selection_range: None,
                children: None,
            })
            .collect();

        Ok(Some(DocumentSymbolResponse::Flat(document_symbols)))
    }
}

#[tokio::main]
async fn main() {
    env_logger::init();

    log::info!("RADS LSP Server v0.1.0 starting");

    let stdin = tokio::io::stdin();
    let stdout = tokio::io::stdout();

    let server = RadsLanguageServer::new();
    let (service, socket) = LspService::build(stdin, stdout)
        .with_service(server)
        .finish();

    socket.serve().await;
}
