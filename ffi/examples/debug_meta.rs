use orgize::{Org, export::from_fn};
use orgize::export::{Container, Event};

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

    fn collect_from_event(&mut self, event: Event) {
        if let Event::Enter(Container::Keyword(keyword)) = event {
            let key = keyword.key();
            let value = keyword.value();

            let key_str = key.as_ref();

            println!("Before check: key='{}' (len={}), bytes={:?}), value='{}' (len={})",
                key_str, key_str.len(), key_str.as_bytes(), value, value.len());
            println!("Conditions: title.is_none()={}, date.is_none()={}, desc.is_none()={}, tags.is_empty()={}",
                self.title.is_none(), self.date.is_none(), self.description.is_none(), self.tags.is_empty());

            if key_str == "TITLE" && self.title.is_none() {
                self.title = Some(value.to_string());
                println!("Set title to: {}", self.title.as_ref().unwrap());
            } else if key_str == "DATE" && self.date.is_none() {
                self.date = Some(value.to_string());
                println!("Set date to: {}", self.date.as_ref().unwrap());
            } else if key_str == "DESCRIPTION" && self.description.is_none() {
                self.description = Some(value.to_string());
                println!("Set description to: {}", self.description.as_ref().unwrap());
            } else if key_str == "FILETAGS" && self.tags.is_empty() {
                self.tags = value.split_whitespace()
                    .map(|s| s.to_string())
                    .collect();
                println!("Set tags to: {:?}", self.tags);
            } else {
                println!("Did NOT set any field");
            }

            println!("After check: title={:?}, date={:?}, desc={:?}, tags={:?}",
                self.title, self.date, self.description, self.tags);
        }
    }
}

fn main() {
    let org_content = r#"#+title: Test Post
#+date: 2024-01-26
#+description: Test FFI parser
#+filetags: test rust

* Introduction

This is a test post."#;

    let org = Org::parse(org_content);

    let mut collector = MetadataCollector::new();
    let mut handler = from_fn(|event| collector.collect_from_event(event));

    org.traverse(&mut handler);

    println!("\nCollected metadata:");
    println!("Title: {:?}", collector.title);
    println!("Date: {:?}", collector.date);
    println!("Description: {:?}", collector.description);
    println!("Tags: {:?}", collector.tags);
}
