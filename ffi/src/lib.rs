use orgize::{Org, export::{from_fn, Container, Event, TraversalContext, Traverser}, SyntaxKind};
use orgize::export::HtmlEscape;
use orgize::rowan::ast::AstNode;
use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::ptr;
use std::fmt::Write;

struct HtmlExportWithUrls {
    output: String,
    in_descriptive_list: Vec<bool>,
}

impl HtmlExportWithUrls {
    fn new() -> Self {
        HtmlExportWithUrls {
            output: String::new(),
            in_descriptive_list: Vec::new(),
        }
    }

    fn process_text_with_urls(output: &mut String, text: &str) {
        let mut byte_pos = 0;
        let chars: Vec<char> = text.chars().collect();

        while byte_pos < chars.len() {
            if byte_pos + 7 <= chars.len()
                && chars[byte_pos] == 'h'
                && chars[byte_pos + 1] == 't'
                && chars[byte_pos + 2] == 't'
                && chars[byte_pos + 3] == 'p'
            {
                let start = byte_pos;
                let maybe_result = if byte_pos + 8 <= chars.len()
                    && chars[byte_pos + 4] == 's'
                    && chars[byte_pos + 5] == ':'
                    && chars[byte_pos + 6] == '/'
                    && chars[byte_pos + 7] == '/'
                {
                    Self::extract_url(&chars, byte_pos + 8, "https://")
                } else if byte_pos + 7 <= chars.len()
                    && chars[byte_pos + 4] == ':'
                    && chars[byte_pos + 5] == '/'
                    && chars[byte_pos + 6] == '/'
                {
                    Self::extract_url(&chars, byte_pos + 7, "http://")
                } else {
                    None
                };

                if let Some((url, end_pos)) = maybe_result {
                    if url.len() > 10 {
                        let _ = write!(output, r#"<a href="{}">{}</a>"#, HtmlEscape(&url), HtmlEscape(&url));
                        byte_pos = end_pos;
                    } else {
                        let c = chars[start];
                        let _ = write!(output, "{}", HtmlEscape(&c.to_string()));
                        byte_pos += 1;
                    }
                } else {
                    let c = chars[start];
                    let _ = write!(output, "{}", HtmlEscape(&c.to_string()));
                    byte_pos += 1;
                }
            } else {
                let c = chars[byte_pos];
                let _ = write!(output, "{}", HtmlEscape(&c.to_string()));
                byte_pos += 1;
            }
        }
    }

    fn extract_url(chars: &[char], start: usize, scheme: &str) -> Option<(String, usize)> {
        let mut url_end = start;
        let mut paren_depth = 0;

        while url_end < chars.len() {
            let c = chars[url_end];

            if c == '(' {
                paren_depth += 1;
            } else if c == '（' {
                paren_depth += 1;
            } else if c == ')' {
                if paren_depth > 0 {
                    paren_depth -= 1;
                } else {
                    break;
                }
            } else if c == '）' {
                if paren_depth > 0 {
                    paren_depth -= 1;
                } else {
                    break;
                }
            } else if c.is_whitespace() || c == '<' || c == '>' || c == '"' || c == '\''
                || c == '。' || c == '，' || c == '！' || c == '？' || c == '；' || c == '：'
            {
                break;
            }
            url_end += 1;
        }

        let url_str: String = chars[start..url_end].iter().collect();
        let trimmed_len = url_str.trim_end_matches(|c: char| {
            c == '.' || c == ',' || c == '!' || c == '?' || c == ';' || c == ':'
        }).chars().count();

        let actual_end = start + trimmed_len;
        let trimmed_url: String = chars[start..actual_end].iter().collect();

        let url = format!("{}{}", scheme, trimmed_url);
        if url.len() > 10 {
            Some((url, actual_end))
        } else {
            None
        }
    }

    fn finish(self) -> String {
        self.output
    }
}

impl Traverser for HtmlExportWithUrls {
    fn event(&mut self, event: Event, ctx: &mut TraversalContext) {
        match event {
            Event::Enter(Container::Document(_)) => self.output += "<main>",
            Event::Leave(Container::Document(_)) => self.output += "</main>",

            Event::Enter(Container::Headline(headline)) => {
                let level = std::cmp::min(headline.level(), 6);
                let _ = write!(&mut self.output, "<h{level}>");
                for elem in headline.title() {
                    self.element(elem, ctx);
                }
                let _ = write!(&mut self.output, "</h{level}>");
            }
            Event::Leave(Container::Headline(_)) => {}

            Event::Enter(Container::Paragraph(_)) => self.output += "<p>",
            Event::Leave(Container::Paragraph(_)) => self.output += "</p>",

            Event::Enter(Container::Section(_)) => self.output += "<section>",
            Event::Leave(Container::Section(_)) => self.output += "</section>",

            Event::Enter(Container::Italic(_)) => self.output += "<i>",
            Event::Leave(Container::Italic(_)) => self.output += "</i>",

            Event::Enter(Container::Bold(_)) => self.output += "<b>",
            Event::Leave(Container::Bold(_)) => self.output += "</b>",

            Event::Enter(Container::Strike(_)) => self.output += "<s>",
            Event::Leave(Container::Strike(_)) => self.output += "</s>",

            Event::Enter(Container::Underline(_)) => self.output += "<u>",
            Event::Leave(Container::Underline(_)) => self.output += "</u>",

            Event::Enter(Container::Verbatim(_)) => self.output += "<code>",
            Event::Leave(Container::Verbatim(_)) => self.output += "</code>",

            Event::Enter(Container::Code(_)) => self.output += "<code>",
            Event::Leave(Container::Code(_)) => self.output += "</code>",

            Event::Enter(Container::SourceBlock(block)) => {
                self.output += r#"<div class="org-src-container">"#;
                if let Some(language) = block.language() {
                    let _ = write!(
                        &mut self.output,
                        r#"<pre class="src src-{}">"#,
                        HtmlEscape(&language)
                    );
                } else {
                    self.output += r#"<pre class="src">"#
                }
            }
            Event::Leave(Container::SourceBlock(_)) => self.output += "</pre></div>",

            Event::Enter(Container::QuoteBlock(_)) => self.output += "<blockquote>",
            Event::Leave(Container::QuoteBlock(_)) => self.output += "</blockquote>",

            Event::Enter(Container::VerseBlock(_)) => self.output += "<p class=\"verse\">",
            Event::Leave(Container::VerseBlock(_)) => self.output += "</p>",

            Event::Enter(Container::ExampleBlock(_)) => self.output += "<pre class=\"example\">",
            Event::Leave(Container::ExampleBlock(_)) => self.output += "</pre>",

            Event::Enter(Container::CenterBlock(_)) => self.output += "<div class=\"center\">",
            Event::Leave(Container::CenterBlock(_)) => self.output += "</div>",

            Event::Enter(Container::CommentBlock(_)) => self.output += "<!--",
            Event::Leave(Container::CommentBlock(_)) => self.output += "-->",

            Event::Enter(Container::Comment(_)) => self.output += "<!--",
            Event::Leave(Container::Comment(_)) => self.output += "-->",

            Event::Enter(Container::Subscript(_)) => self.output += "<sub>",
            Event::Leave(Container::Subscript(_)) => self.output += "</sub>",

            Event::Enter(Container::Superscript(_)) => self.output += "<sup>",
            Event::Leave(Container::Superscript(_)) => self.output += "</sup>",

            Event::Enter(Container::List(list)) => {
                self.output += if list.is_ordered() {
                    self.in_descriptive_list.push(false);
                    "<ol>"
                } else if list.is_descriptive() {
                    self.in_descriptive_list.push(true);
                    "<dl>"
                } else {
                    self.in_descriptive_list.push(false);
                    "<ul>"
                };
            }
            Event::Leave(Container::List(_)) => {
                self.in_descriptive_list.pop();
            }

            Event::Enter(Container::ListItem(list_item)) => {
                if let Some(&true) = self.in_descriptive_list.last() {
                    self.output += "<dt>";
                    for elem in list_item.tag() {
                        self.element(elem, ctx);
                    }
                    self.output += "</dt><dd>";
                } else {
                    self.output += "<li>";
                }
            }
            Event::Leave(Container::ListItem(_)) => {
                if let Some(&true) = self.in_descriptive_list.last() {
                    self.output += "</dd>";
                } else {
                    self.output += "</li>";
                }
            }

            Event::Enter(Container::OrgTable(_)) => self.output += "<table>",
            Event::Leave(Container::OrgTable(_)) => self.output += "</table>",

            Event::Enter(Container::OrgTableRow(row)) => {
                if !row.is_rule() {
                    self.output += "<tr>";
                } else {
                    ctx.skip();
                }
            }
            Event::Leave(Container::OrgTableRow(row)) => {
                if !row.is_rule() {
                    self.output += "</tr>";
                } else {
                    ctx.skip();
                }
            }

            Event::Enter(Container::OrgTableCell(_)) => self.output += "<td>",
            Event::Leave(Container::OrgTableCell(_)) => self.output += "</td>",

            Event::Enter(Container::Link(link)) => {
                let path = link.path();
                let path = path.trim_start_matches("file:");

                if link.is_image() {
                    let _ = write!(&mut self.output, r#"<img src="{}">"#, HtmlEscape(&path));
                    return ctx.skip();
                }

                let _ = write!(&mut self.output, r#"<a href="{}">"#, HtmlEscape(&path));

                if !link.has_description() {
                    let _ = write!(&mut self.output, "{}</a>", HtmlEscape(&path));
                    ctx.skip();
                }
            }
            Event::Leave(Container::Link(_)) => self.output += "</a>",

            Event::Text(text) => {
                Self::process_text_with_urls(&mut self.output, &text);
            }

            Event::LineBreak(_) => self.output += "<br/>",

            Event::Snippet(snippet) => {
                if snippet.backend().eq_ignore_ascii_case("html") {
                    self.output += &snippet.value();
                }
            }

            Event::Rule(_) => self.output += "<hr/>",

            Event::Timestamp(timestamp) => {
                self.output += r#"<span class="timestamp-wrapper"><span class="timestamp">"#;
                for e in timestamp.syntax().children_with_tokens() {
                    match e {
                        orgize::rowan::NodeOrToken::Token(t) if t.kind() == SyntaxKind::MINUS2 => {
                            self.output += "&#x2013;";
                        }
                        orgize::rowan::NodeOrToken::Token(t) => {
                            self.output += t.text();
                        }
                        _ => {}
                    }
                }
                self.output += r#"</span></span>"#;
            }

            Event::LatexFragment(latex) => {
                let _ = write!(&mut self.output, "{}", latex.syntax());
            }
            Event::LatexEnvironment(latex) => {
                let _ = write!(&mut self.output, "{}", latex.syntax());
            }

            Event::Enter(Container::Keyword(_)) => ctx.skip(),

            Event::Entity(entity) => self.output += entity.html(),

            _ => {}
        }
    }
}

fn extract_body_content(html: &str) -> String {
    let body_start = html.find("<body>").and_then(|pos| {
        html[pos + 6..].find('>').map(|end| pos + 6 + end + 1)
    });

    let body_end = html.find("</body>");

    match (body_start, body_end) {
        (Some(start), Some(end)) if start < end => html[start..end].trim().to_string(),
        _ => html.to_string(),
    }
}

#[repr(C)]
pub struct OrgMetadata {
    title: *mut c_char,
    date: *mut c_char,
    description: *mut c_char,
    tags: *mut c_char,
    tags_array: *mut *mut c_char,
    tags_count: usize,
}

struct MetadataCollector {
    title: Option<String>,
    date: Option<String>,
    description: Option<String>,
    tags: Vec<String>,
}

impl MetadataCollector {
    fn new() -> Self {
        MetadataCollector {
            title: None,
            date: None,
            description: None,
            tags: Vec::new(),
        }
    }
}

impl MetadataCollector {
    fn collect_from_event(&mut self, event: Event) {
        if let Event::Enter(Container::Keyword(keyword)) = event {
            let key = keyword.key();
            let value = keyword.value();

            if key.eq_ignore_ascii_case("TITLE") && self.title.is_none() {
                self.title = Some(value.to_string());
            } else if key.eq_ignore_ascii_case("DATE") && self.date.is_none() {
                self.date = Some(value.to_string());
            } else if key.eq_ignore_ascii_case("DESCRIPTION") && self.description.is_none() {
                self.description = Some(value.to_string());
            } else if key.eq_ignore_ascii_case("FILETAGS") && self.tags.is_empty() {
                self.tags = value.split_whitespace()
                    .map(|s| s.to_string())
                    .collect();
            }
        }
    }
}

#[no_mangle]
pub extern "C" fn org_parse_to_html(input: *const c_char, len: usize) -> *mut c_char {
    if input.is_null() || len == 0 {
        return ptr::null_mut();
    }

    unsafe {
        let c_str = CStr::from_ptr(input);

        let org_str = match c_str.to_str() {
            Ok(s) => s,
            Err(_) => return ptr::null_mut(),
        };

        let org = Org::parse(org_str);
        let mut exporter = HtmlExportWithUrls::new();
        org.traverse(&mut exporter);
        let html = exporter.finish();
        let body_content = extract_body_content(&html);

        match CString::new(body_content) {
            Ok(c_string) => c_string.into_raw(),
            Err(_) => ptr::null_mut(),
        }
    }
}

#[no_mangle]
pub extern "C" fn org_extract_metadata(input: *const c_char, len: usize) -> *mut OrgMetadata {
    if input.is_null() || len == 0 {
        return ptr::null_mut();
    }

    unsafe {
        let c_str = CStr::from_ptr(input);
        let org_str = match c_str.to_str() {
            Ok(s) => s,
            Err(_) => return ptr::null_mut(),
        };

        let org = Org::parse(org_str);

        let mut collector = MetadataCollector::new();
        let mut handler = from_fn(|event| collector.collect_from_event(event));

        org.traverse(&mut handler);

        let title_c = match collector.title {
            Some(ref s) => match CString::new(s.trim()) {
                Ok(s) => s.into_raw(),
                Err(_) => ptr::null_mut(),
            },
            None => ptr::null_mut(),
        };

        let date_c = match CString::new("") {
            Ok(s) => s.into_raw(),
            Err(_) => ptr::null_mut(),
        };

        let description_c = match CString::new("") {
            Ok(s) => s.into_raw(),
            Err(_) => ptr::null_mut(),
        };

        let tags_str = collector.tags.join(" ");
        let tags_c = if !tags_str.is_empty() {
            match CString::new(tags_str) {
                Ok(s) => s.into_raw(),
                Err(_) => ptr::null_mut(),
            }
        } else {
            ptr::null_mut()
        };

        let tags_len = collector.tags.len();

        let tags_array: Vec<*mut c_char> = collector.tags
            .into_iter()
            .filter_map(|tag| CString::new(tag).ok())
            .map(|s| s.into_raw())
            .collect();

        let tags_array_ptr = if tags_array.is_empty() {
            ptr::null_mut()
        } else {
            let mut boxed = tags_array.into_boxed_slice();
            let ptr = boxed.as_mut_ptr();
            std::mem::forget(boxed);
            ptr
        };

        let metadata = Box::new(OrgMetadata {
            title: title_c,
            date: date_c,
            description: description_c,
            tags: tags_c,
            tags_array: tags_array_ptr,
            tags_count: if tags_array_ptr.is_null() { 0 } else { tags_len },
        });

        Box::into_raw(metadata)
    }
}

#[no_mangle]
pub extern "C" fn org_free_string(s: *mut c_char) {
    if !s.is_null() {
        unsafe {
            let _ = CString::from_raw(s);
        }
    }
}

#[no_mangle]
pub extern "C" fn org_free_metadata(meta: *mut OrgMetadata) {
    if !meta.is_null() {
        unsafe {
            let meta_box = Box::from_raw(meta);

            if !meta_box.title.is_null() {
                let _ = CString::from_raw(meta_box.title);
            }
            if !meta_box.date.is_null() {
                let _ = CString::from_raw(meta_box.date);
            }
            if !meta_box.description.is_null() {
                let _ = CString::from_raw(meta_box.description);
            }
            if !meta_box.tags.is_null() {
                let _ = CString::from_raw(meta_box.tags);
            }
            if !meta_box.tags_array.is_null() && meta_box.tags_count > 0 {
                let slice = std::slice::from_raw_parts_mut(meta_box.tags_array, meta_box.tags_count);
                for tag_ptr in slice {
                    if !tag_ptr.is_null() {
                        let _ = CString::from_raw(*tag_ptr);
                    }
                }
                let _ = Box::from_raw(meta_box.tags_array);
            }
        }
    }
}

#[no_mangle]
pub extern "C" fn org_meta_get_title(meta: *const OrgMetadata) -> *const c_char {
    if meta.is_null() {
        return ptr::null();
    }
    unsafe {
        (*meta).title
    }
}

#[no_mangle]
pub extern "C" fn org_meta_get_date(meta: *const OrgMetadata) -> *const c_char {
    if meta.is_null() {
        return ptr::null();
    }
    unsafe {
        (*meta).date
    }
}

#[no_mangle]
pub extern "C" fn org_meta_get_description(meta: *const OrgMetadata) -> *const c_char {
    if meta.is_null() {
        return ptr::null();
    }
    unsafe {
        (*meta).description
    }
}

#[no_mangle]
pub extern "C" fn org_meta_get_tags(meta: *const OrgMetadata) -> *const c_char {
    if meta.is_null() {
        return ptr::null();
    }
    unsafe {
        (*meta).tags
    }
}

#[no_mangle]
pub extern "C" fn org_meta_get_tags_array(meta: *const OrgMetadata, out_count: *mut usize) -> *const *const c_char {
    if meta.is_null() || out_count.is_null() {
        return ptr::null();
    }
    unsafe {
        *out_count = (*meta).tags_count;
        (*meta).tags_array as *const *const c_char
    }
}
