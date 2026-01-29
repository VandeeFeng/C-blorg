use orgize::{Org, export::{from_fn, Container, Event, TraversalContext, Traverser}, SyntaxKind};
use orgize::export::HtmlEscape;
use orgize::ast::{Headline, Keyword, Link, List, ListItem, OrgTableRow, SourceBlock, Timestamp};
use orgize::config::{ParseConfig, UseSubSuperscript};
use orgize::rowan::ast::AstNode;
use std::collections::HashMap;
use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::ptr;
use std::fmt::Write;
use slugify::slugify;

struct HtmlExportWithUrls {
    output: String,
    in_descriptive_list: Vec<bool>,
    pending_attributes: Option<HashMap<String, String>>,
    paragraph_start_len: Vec<usize>,
    in_verbatim_or_code: bool,
}

impl HtmlExportWithUrls {
    fn new() -> Self {
        HtmlExportWithUrls {
            output: String::new(),
            in_descriptive_list: Vec::new(),
            pending_attributes: None,
            paragraph_start_len: Vec::new(),
            in_verbatim_or_code: false,
        }
    }

    fn append_escaped_char(&mut self, c: char) {
        let mut buf = [0; 4];
        let s = c.encode_utf8(&mut buf);
        let _ = write!(&mut self.output, "{}", HtmlEscape(s));
    }

    fn open_tag(&mut self, tag: &str) {
        let _ = write!(&mut self.output, "<{}>", tag);
    }

    fn close_tag(&mut self, tag: &str) {
        let _ = write!(&mut self.output, "</{}>", tag);
    }

    fn enter_inline_code(&mut self) {
        self.in_verbatim_or_code = true;
        self.open_tag("code");
    }

    fn leave_inline_code(&mut self) {
        self.in_verbatim_or_code = false;
        self.close_tag("code");
    }

    fn attrs_string(attrs: &HashMap<String, String>, skip_empty_alt: bool) -> String {
        let mut attrs_str = String::new();
        for (key, value) in attrs {
            if skip_empty_alt && key.eq_ignore_ascii_case("alt") && value.is_empty() {
                continue;
            }
            let _ = write!(attrs_str, r#" {}="{}""#, key, HtmlEscape(value));
        }
        attrs_str
    }

    fn take_pending_attrs_string(&mut self, skip_empty_alt: bool) -> String {
        match self.pending_attributes.take() {
            Some(attrs) => Self::attrs_string(&attrs, skip_empty_alt),
            None => String::new(),
        }
    }

    fn merge_pending_attributes(&mut self, parsed: HashMap<String, String>) {
        if let Some(existing) = &mut self.pending_attributes {
            existing.extend(parsed);
        } else {
            self.pending_attributes = Some(parsed);
        }
    }

    fn escape_text_only(&mut self, text: &str) {
        let _ = write!(&mut self.output, "{}", HtmlEscape(text));
    }

    fn handle_document_enter(&mut self) {
        self.output.push_str("<main>");
    }

    fn handle_document_leave(&mut self) {
        self.output.push_str("</main>");
    }

    fn handle_headline_enter(&mut self, headline: &Headline, ctx: &mut TraversalContext) {
        let level = std::cmp::min(headline.level() + 1, 6);
        let (_title, id) = headline_title_and_id(headline);
        let _ = write!(&mut self.output, "<h{} id=\"{}\">", level, id);
        for elem in headline.title() {
            self.element(elem, ctx);
        }
        let _ = write!(&mut self.output, "</h{}>", level);
    }

    fn handle_paragraph_enter(&mut self) {
        self.paragraph_start_len.push(self.output.len());
        self.output.push_str("<p>");
    }

    fn handle_paragraph_leave(&mut self) {
        let start_len = self.paragraph_start_len.pop().unwrap_or(0);
        let paragraph_open_len = "<p>".len();
        if self.output.len() == start_len + paragraph_open_len {
            self.output.truncate(start_len);
        } else {
            self.output.push_str("</p>");
        }
        self.pending_attributes = None;
    }

    fn handle_source_block_enter(&mut self, block: &SourceBlock) {
        self.output.push_str(r#"<div class="org-src-container">"#);
        if let Some(language) = block.language() {
            let _ = write!(
                &mut self.output,
                r#"<pre class="src src-{}">"#,
                HtmlEscape(&language)
            );
        } else {
            self.output.push_str(r#"<pre class="src">"#);
        }
    }

    fn handle_list_enter(&mut self, list: &List) {
        if list.is_ordered() {
            self.in_descriptive_list.push(false);
            self.output.push_str("<ol>");
            return;
        }

        if list.is_descriptive() {
            self.in_descriptive_list.push(true);
            self.output.push_str("<dl>");
            return;
        }

        self.in_descriptive_list.push(false);
        self.output.push_str("<ul>");
    }

    fn handle_list_leave(&mut self) {
        self.in_descriptive_list.pop();
    }

    fn handle_list_item_enter(&mut self, list_item: &ListItem, ctx: &mut TraversalContext) {
        if let Some(&true) = self.in_descriptive_list.last() {
            self.output.push_str("<dt>");
            for elem in list_item.tag() {
                self.element(elem, ctx);
            }
            self.output.push_str("</dt><dd>");
        } else {
            self.output.push_str("<li>");
        }
    }

    fn handle_list_item_leave(&mut self) {
        if let Some(&true) = self.in_descriptive_list.last() {
            self.output.push_str("</dd>");
        } else {
            self.output.push_str("</li>");
        }
    }

    fn handle_table_row_enter(&mut self, row: &OrgTableRow, ctx: &mut TraversalContext) {
        if row.is_rule() {
            ctx.skip();
            return;
        }

        self.output.push_str("<tr>");
    }

    fn handle_table_row_leave(&mut self, row: &OrgTableRow, ctx: &mut TraversalContext) {
        if row.is_rule() {
            ctx.skip();
            return;
        }

        self.output.push_str("</tr>");
    }

    fn handle_link_enter(&mut self, link: &Link, ctx: &mut TraversalContext) {
        let path = link.path();
        let path = path.trim_start_matches("file:");
        let attrs_str = self.take_pending_attrs_string(false);

        if link.is_image() {
            let _ = write!(&mut self.output, r#"<img src="{}"{}>"#, HtmlEscape(&path), attrs_str);
            return ctx.skip();
        }

        let _ = write!(&mut self.output, r#"<a href="{}"{}>"#, HtmlEscape(&path), attrs_str);

        if !link.has_description() {
            let _ = write!(&mut self.output, "{}</a>", HtmlEscape(&path));
            ctx.skip();
        }
    }

    fn handle_text(&mut self, text: &str) {
        if self.in_verbatim_or_code {
            self.escape_text_only(text);
        } else {
            self.process_text_with_urls(text);
        }
    }

    fn handle_timestamp(&mut self, timestamp: &Timestamp) {
        self.output.push_str(r#"<span class="timestamp-wrapper"><span class="timestamp">"#);
        for e in timestamp.syntax().children_with_tokens() {
            match e {
                orgize::rowan::NodeOrToken::Token(t) if t.kind() == SyntaxKind::MINUS2 => {
                    self.output.push_str("&#x2013;");
                }
                orgize::rowan::NodeOrToken::Token(t) => {
                    self.output.push_str(t.text());
                }
                _ => {}
            }
        }
        self.output.push_str(r#"</span></span>"#);
    }

    fn handle_keyword(&mut self, keyword: &Keyword, ctx: &mut TraversalContext) {
        let key = keyword.key();
        if key.eq_ignore_ascii_case("attr_html") {
            let value = keyword.value();
            let parsed = Self::parse_attr_html(&value);
            self.merge_pending_attributes(parsed);
        }
        ctx.skip();
    }

    fn process_text_with_urls(&mut self, text: &str) {
        let mut char_pos = 0;
        let chars: Vec<char> = text.chars().collect();

        while char_pos < chars.len() {
            if char_pos + 7 <= chars.len()
                && chars[char_pos] == 'h'
                && chars[char_pos + 1] == 't'
                && chars[char_pos + 2] == 't'
                && chars[char_pos + 3] == 'p'
            {
                let start = char_pos;
                let maybe_result = if char_pos + 8 <= chars.len()
                    && chars[char_pos + 4] == 's'
                    && chars[char_pos + 5] == ':'
                    && chars[char_pos + 6] == '/'
                    && chars[char_pos + 7] == '/'
                {
                    Self::extract_url(&chars, char_pos + 8, "https://")
                } else if char_pos + 7 <= chars.len()
                    && chars[char_pos + 4] == ':'
                    && chars[char_pos + 5] == '/'
                    && chars[char_pos + 6] == '/'
                {
                    Self::extract_url(&chars, char_pos + 7, "http://")
                } else {
                    None
                };

                if let Some((url, end_pos)) = maybe_result {
                    if url.len() > 10 {
                        let use_image = Self::is_image_url(&url) && self.pending_attributes.is_some();
                        let attrs_str = self.take_pending_attrs_string(true);
                        if use_image {
                            let _ = write!(
                                &mut self.output,
                                r#"<img src="{}"{}>"#,
                                HtmlEscape(&url),
                                attrs_str
                            );
                        } else {
                            let _ = write!(
                                &mut self.output,
                                r#"<a href="{}"{}>{}</a>"#,
                                HtmlEscape(&url),
                                attrs_str,
                                HtmlEscape(&url)
                            );
                        }
                        char_pos = end_pos;
                        continue;
                    }
                }
                self.append_escaped_char(chars[start]);
                char_pos += 1;
            } else {
                self.append_escaped_char(chars[char_pos]);
                char_pos += 1;
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

    fn parse_attr_html(value: &str) -> HashMap<String, String> {
        fn attr_key(token: &str) -> Option<&str> {
            token.strip_prefix(':').or_else(|| token.strip_prefix('：'))
        }

        let mut attrs = HashMap::new();
        let mut tokens = Vec::new();
        let mut current = String::new();
        let mut in_quotes = false;
        let mut chars = value.chars().peekable();

        let push_token = |tokens: &mut Vec<String>, current: &mut String| {
            if !current.is_empty() {
                tokens.push(std::mem::take(current));
            }
        };

        while let Some(c) = chars.next() {
            if c == '"' {
                if in_quotes && chars.peek() == Some(&'"') {
                    current.push('"');
                    chars.next();
                } else {
                    in_quotes = !in_quotes;
                }
                continue;
            }

            if !in_quotes && c.is_whitespace() {
                push_token(&mut tokens, &mut current);
                continue;
            }

            current.push(c);
        }

        push_token(&mut tokens, &mut current);

        let mut index = 0;
        while index < tokens.len() {
            let token = &tokens[index];
            let Some(key) = attr_key(token) else {
                index += 1;
                continue;
            };

            if key.is_empty() {
                index += 1;
                continue;
            }

            let mut value = String::new();
            if index + 1 < tokens.len() {
                let next = &tokens[index + 1];
                if attr_key(next).is_none() {
                    value = next.clone();
                    index += 1;
                }
            }

            attrs.insert(key.to_string(), value);
            index += 1;
        }

        attrs
    }

    fn is_image_url(url: &str) -> bool {
        let lower = url.to_ascii_lowercase();
        let mut trimmed = lower.as_str();
        if let Some(split) = trimmed.split(|c| c == '?' || c == '#').next() {
            trimmed = split;
        }
        trimmed.ends_with(".png")
            || trimmed.ends_with(".jpg")
            || trimmed.ends_with(".jpeg")
            || trimmed.ends_with(".gif")
            || trimmed.ends_with(".webp")
            || trimmed.ends_with(".svg")
            || trimmed.ends_with(".bmp")
            || trimmed.ends_with(".avif")
    }

    fn finish(self) -> String {
        self.output
    }
}

struct TocBuilder {
    output: String,
}

impl TocBuilder {
    fn new() -> Self {
        TocBuilder {
            output: String::new(),
        }
    }

    fn finish(self) -> String {
        self.output
    }

    fn handle_headline_enter(&mut self, headline: &Headline) {
        let (title, id) = headline_title_and_id(headline);
        let has_children = headline.headlines().next().is_some();
        let _ = write!(&mut self.output, "<li><a href=\"#{}\">{}</a>", id, title);
        if has_children {
            self.output.push_str("<ul>");
        }
    }

    fn handle_headline_leave(&mut self, headline: &Headline) {
        if headline.headlines().next().is_some() {
            self.output.push_str("</ul>");
        }
        self.output.push_str("</li>");
    }
}

fn parse_org_with_config(org_str: &str) -> Org {
    let mut config = ParseConfig::default();
    config.use_sub_superscript = UseSubSuperscript::Brace;
    config.parse(org_str)
}

fn input_to_str(input: *const c_char, len: usize) -> Option<String> {
    if input.is_null() || len == 0 {
        return None;
    }

    unsafe { CStr::from_ptr(input).to_str().ok().map(|value| value.to_string()) }
}

fn into_c_string(value: &str) -> *mut c_char {
    match CString::new(value) {
        Ok(s) => s.into_raw(),
        Err(_) => ptr::null_mut(),
    }
}

fn into_c_string_optional(value: Option<&str>) -> *mut c_char {
    match value {
        Some(text) => into_c_string(text.trim()),
        None => ptr::null_mut(),
    }
}

fn headline_title_and_id(headline: &Headline) -> (String, String) {
    let title = headline.title().map(|e| e.to_string()).collect::<String>();
    let id = slugify!(&title);
    (title, id)
}

impl Traverser for HtmlExportWithUrls {
    fn event(&mut self, event: Event, ctx: &mut TraversalContext) {
        match event {
            Event::Enter(Container::Document(_)) => self.handle_document_enter(),
            Event::Leave(Container::Document(_)) => self.handle_document_leave(),

            Event::Enter(Container::Headline(headline)) => self.handle_headline_enter(&headline, ctx),
            Event::Leave(Container::Headline(_)) => {},

            Event::Enter(Container::Paragraph(_)) => self.handle_paragraph_enter(),
            Event::Leave(Container::Paragraph(_)) => self.handle_paragraph_leave(),

            Event::Enter(Container::Section(_)) => self.open_tag("section"),
            Event::Leave(Container::Section(_)) => self.close_tag("section"),

            Event::Enter(Container::Italic(_)) => self.open_tag("i"),
            Event::Leave(Container::Italic(_)) => self.close_tag("i"),

            Event::Enter(Container::Bold(_)) => self.open_tag("b"),
            Event::Leave(Container::Bold(_)) => self.close_tag("b"),

            Event::Enter(Container::Strike(_)) => self.open_tag("s"),
            Event::Leave(Container::Strike(_)) => self.close_tag("s"),

            Event::Enter(Container::Underline(_)) => self.open_tag("u"),
            Event::Leave(Container::Underline(_)) => self.close_tag("u"),

            Event::Enter(Container::Verbatim(_)) => {
                self.enter_inline_code();
            }
            Event::Leave(Container::Verbatim(_)) => {
                self.leave_inline_code();
            }

            Event::Enter(Container::Code(_)) => {
                self.enter_inline_code();
            }
            Event::Leave(Container::Code(_)) => {
                self.leave_inline_code();
            }

            Event::Enter(Container::SourceBlock(block)) => self.handle_source_block_enter(&block),
            Event::Leave(Container::SourceBlock(_)) => self.output.push_str("</pre></div>"),

            Event::Enter(Container::QuoteBlock(_)) => self.open_tag("blockquote"),
            Event::Leave(Container::QuoteBlock(_)) => self.close_tag("blockquote"),

            Event::Enter(Container::VerseBlock(_)) => self.output.push_str("<p class=\"verse\">"),
            Event::Leave(Container::VerseBlock(_)) => self.close_tag("p"),

            Event::Enter(Container::ExampleBlock(_)) => self.output.push_str("<pre class=\"example\">"),
            Event::Leave(Container::ExampleBlock(_)) => self.close_tag("pre"),

            Event::Enter(Container::CenterBlock(_)) => self.output.push_str("<div class=\"center\">"),
            Event::Leave(Container::CenterBlock(_)) => self.close_tag("div"),

            Event::Enter(Container::CommentBlock(_)) => self.output.push_str("<!--"),
            Event::Leave(Container::CommentBlock(_)) => self.output.push_str("-->"),

            Event::Enter(Container::Comment(_)) => self.output.push_str("<!--"),
            Event::Leave(Container::Comment(_)) => self.output.push_str("-->"),

            Event::Enter(Container::Subscript(_)) => self.open_tag("sub"),
            Event::Leave(Container::Subscript(_)) => self.close_tag("sub"),

            Event::Enter(Container::Superscript(_)) => self.open_tag("sup"),
            Event::Leave(Container::Superscript(_)) => self.close_tag("sup"),

            Event::Enter(Container::List(list)) => self.handle_list_enter(&list),
            Event::Leave(Container::List(_)) => self.handle_list_leave(),

            Event::Enter(Container::ListItem(list_item)) => self.handle_list_item_enter(&list_item, ctx),
            Event::Leave(Container::ListItem(_)) => self.handle_list_item_leave(),

            Event::Enter(Container::OrgTable(_)) => self.open_tag("table"),
            Event::Leave(Container::OrgTable(_)) => self.close_tag("table"),

            Event::Enter(Container::OrgTableRow(row)) => self.handle_table_row_enter(&row, ctx),
            Event::Leave(Container::OrgTableRow(row)) => self.handle_table_row_leave(&row, ctx),

            Event::Enter(Container::OrgTableCell(_)) => self.open_tag("td"),
            Event::Leave(Container::OrgTableCell(_)) => self.close_tag("td"),

            Event::Enter(Container::Link(link)) => self.handle_link_enter(&link, ctx),
            Event::Leave(Container::Link(_)) => self.close_tag("a"),

            Event::Text(text) => self.handle_text(&text),

            Event::LineBreak(_) => self.output.push_str("<br/>"),

            Event::Snippet(snippet) => {
                if snippet.backend().eq_ignore_ascii_case("html") {
                    self.output += &snippet.value();
                }
            }

            Event::Rule(_) => self.output.push_str("<hr/>"),

            Event::Timestamp(timestamp) => self.handle_timestamp(&timestamp),

            Event::LatexFragment(latex) => {
                let _ = write!(&mut self.output, "{}", latex.syntax());
            }
            Event::LatexEnvironment(latex) => {
                let _ = write!(&mut self.output, "{}", latex.syntax());
            }

            Event::Enter(Container::Keyword(keyword)) => self.handle_keyword(&keyword, ctx),

            Event::Entity(entity) => self.output += entity.html(),

            _ => {}
        }
    }
}

impl Traverser for TocBuilder {
    fn event(&mut self, event: Event, _ctx: &mut TraversalContext) {
        match event {
            Event::Enter(Container::Headline(headline)) => self.handle_headline_enter(&headline),
            Event::Leave(Container::Headline(headline)) => self.handle_headline_leave(&headline),
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
    let org_str = match input_to_str(input, len) {
        Some(value) => value,
        None => return ptr::null_mut(),
    };

    let org = parse_org_with_config(org_str.as_str());
    let mut exporter = HtmlExportWithUrls::new();
    org.traverse(&mut exporter);
    let html = exporter.finish();
    let body_content = extract_body_content(&html);

    match CString::new(body_content) {
        Ok(c_string) => c_string.into_raw(),
        Err(_) => ptr::null_mut(),
    }
}

#[no_mangle]
pub extern "C" fn org_extract_metadata(input: *const c_char, len: usize) -> *mut OrgMetadata {
    let org_str = match input_to_str(input, len) {
        Some(value) => value,
        None => return ptr::null_mut(),
    };

    let org = parse_org_with_config(org_str.as_str());

    let mut collector = MetadataCollector::new();
    let mut handler = from_fn(|event| collector.collect_from_event(event));

    org.traverse(&mut handler);

    let title_c = into_c_string_optional(collector.title.as_deref());
    let date_c = into_c_string_optional(collector.date.as_deref());
    let description_c = into_c_string_optional(collector.description.as_deref());

    let tags_str = collector.tags.join(" ");
    let tags_c = if tags_str.is_empty() {
        ptr::null_mut()
    } else {
        into_c_string(&tags_str)
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

#[no_mangle]
pub extern "C" fn org_extract_toc(input: *const c_char, len: usize) -> *mut c_char {
    let org_str = match input_to_str(input, len) {
        Some(value) => value,
        None => return ptr::null_mut(),
    };

    let org = parse_org_with_config(org_str.as_str());
    let mut toc_builder = TocBuilder::new();
    org.traverse(&mut toc_builder);
    let toc = toc_builder.finish();

    let toc_html = format!("<nav class=\"toc\"><ul>{}</ul></nav>", toc);

    match CString::new(toc_html) {
        Ok(c_string) => c_string.into_raw(),
        Err(_) => ptr::null_mut(),
    }
}
