use tower_lsp::lsp_types::*;
use std::collections::HashMap;
use regex::Regex;

/// Refactoring operations for RADS code
pub struct RefactorEngine {
    documents: HashMap<String, String>,
}

impl RefactorEngine {
    pub fn new() -> Self {
        Self {
            documents: HashMap::new(),
        }
    }

    /// Register a document for refactoring
    pub fn register_document(&mut self, uri: &str, text: &str) {
        self.documents.insert(uri.to_string(), text.to_string());
    }

    /// Unregister a document
    pub fn unregister_document(&mut self, uri: &str) {
        self.documents.remove(uri);
    }

    /// Rename a symbol across the document
    pub fn rename_symbol(
        &self,
        uri: &str,
        position: Position,
        new_name: &str,
    ) -> Result<Vec<TextEdit>, String> {
        let text = self.documents.get(uri)
            .ok_or_else(|| "Document not found".to_string())?;

        let lines: Vec<&str> = text.lines().collect();
        let line = lines.get(position.line as usize)
            .ok_or_else(|| "Line not found".to_string())?;

        let word = self.get_word_at(line, position.character as usize);

        if word.is_empty() {
            return Err("No symbol found at position".to_string());
        }

        let mut edits = Vec::new();

        for (line_num, doc_line) in lines.iter().enumerate() {
            let re = Regex::new(&format!(r"\b{}\b", regex::escape(&word)))
                .map_err(|e| format!("Invalid regex: {}", e))?;

            for mat in re.find_iter(doc_line) {
                let edit_range = Range {
                    start: Position {
                        line: line_num as u32,
                        character: mat.start() as u32,
                    },
                    end: Position {
                        line: line_num as u32,
                        character: mat.end() as u32,
            },
        });

        edits.push(TextEdit {
                    range: edit_range,
                    new_text: new_name.to_string(),
                });
            }
        }

        if edits.is_empty() {
            return Err("No occurrences found".to_string());
        }

        Ok(edits)
    }

    /// Extract a selection into a new function
    pub fn extract_function(
        &self,
        uri: &str,
        range: Range,
        function_name: &str,
    ) -> Result<(Vec<TextEdit>, String), String> {
        let text = self.documents.get(uri)
            .ok_or_else(|| "Document not found".to_string())?;

        let lines: Vec<&str> = text.lines().collect();

        let start_line = range.start.line as usize;
        let end_line = range.end.line as usize;

        if start_line >= lines.len() || end_line >= lines.len() {
            return Err("Invalid range".to_string());
        }

        let selected_lines: Vec<&str> = lines[start_line..=end_line].to_vec();
        let extracted_code = selected_lines.join("\n");

        let mut parameters = Vec::new();
        let param_re = Regex::new(r"\bturbo\s+(\w+)\s*=")
            .map_err(|e| format!("Invalid regex: {}", e))?;

        for line in &selected_lines {
            if let Some(caps) = param_re.captures(line) {
                if let Some(var_name) = caps.get(1) {
                    if !parameters.contains(&var_name.as_str()) {
                        parameters.push(var_name.as_str());
                    }
                }
            }
        }

        let function_params = parameters.join(", ");
        let function_def = if function_params.is_empty() {
            format!("\nblast {}() {{\n{}\n}}\n", function_name, extracted_code)
        };

        let mut insert_line = 0;
        for (i, line) in lines.iter().enumerate() {
            if i < start_line && line.starts_with("blast ") {
                insert_line = i;
            }
        }

        let param_names: Vec<&str> = new_params
                    .split(',')
                    .map(|p| p.trim().split_whitespace().last().unwrap_or(""))
                    .filter(|p| !p.is_empty())
                    .collect();

                let call_args = param_names.join(", ");

                edits.push(TextEdit {
                    range: Range {
                        start: Position {
                            line: i as u32,
                            character: mat.start() as u32,
                        },
                        end: Position {
                            line: i as u32,
                            character: mat.end() as u32,
                        },
                    },
                    new_text: format!("{}({});", function_name, call_args),
                });
            }
        }

        Ok(edits)
    }

    /// Get all symbols in the document for refactoring
    pub fn get_all_symbols(&self, uri: &str) -> Vec<(String, Position, SymbolKind)> {
        let mut symbols = Vec::new();

        if let Some(text) = self.documents.get(uri) {
            for (line_num, line) in text.lines().enumerate() {
                if line.starts_with("blast ") {
                    if let Some(start) = line.find("blast ") {
                        let func_name: String = line[start + 6..]
                            .chars()
                            .take_while(|c| *c != '(')
                            .collect();

                        if !func_name.is_empty() {
                            symbols.push((
                                func_name,
                                Position {
                                    line: line_num as u32,
                                    character: (start + 6) as u32,
                                },
                                SymbolKind::Function,
                            ));
                        }
                    }
                }

                if line.contains("turbo ") && line.contains("=") {
                    if let Some(start) = line.find("turbo ") {
                        let var_name: String = line[start + 6..]
                            .chars()
                            .take_while(|c| *c != '=' && *c != ';' && !c.is_whitespace())
                            .collect();

                        if !var_name.is_empty() {
                            symbols.push((
                                var_name,
                                Position {
                                    line: line_num as u32,
                                    character: (start + 6) as u32,
                                },
                                SymbolKind::Variable,
                            ));
                        }
                    }
                }
            }
        }

        symbols
    }

    /// Get word at a specific position
    fn get_word_at(&self, line: &str, column: usize) -> String {
        let mut start = column;
        while start > 0 {
            let c = line.chars().nth(start - 1).unwrap_or(' ');
            if c.is_whitespace() || c == '(' || c == ')' || c == ',' || c == ';' {
                break;
            }
            start -= 1;
        }

        let mut end = column;
        while end < line.len() {
            let c = line.chars().nth(end).unwrap_or(' ');
            if c.is_whitespace() || c == '(' || c == ')' || c == ',' || c == ';' {
                break;
            }
            end += 1;
        }

        line[start..end].trim().to_string()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_get_word_at() {
        let engine = RefactorEngine::new();
        let line = "turbo x = 42;";

        assert_eq!(engine.get_word_at(line, 6), "x");
        assert_eq!(engine.get_word_at(line, 10), "42");
    }
}
