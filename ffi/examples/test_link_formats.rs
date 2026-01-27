use orgize::Org;

fn main() {
    let test_cases = vec![
        // Standard links
        "[[https://example.com][Example Site]]",
        "[[https://example.com]]",
        
        // File links
        "[[file:document.org][Document]]",
        "[[file:document.org]]",
        "[[./relative.org][Relative Link]]",
        
        // Internal links
        "[[#anchor][Anchor Link]]",
        "[[#anchor]]",
        
        // Description with spaces
        "[[https://example.com][Link with multiple words]]",
        
        // Link in paragraph
        "This is text with [[https://example.com][a link]] inside.",
    ];
    
    for (i, input) in test_cases.iter().enumerate() {
        let full_input = format!("* Heading {}\n", input);
        let org = Org::parse(&full_input);
        let html = org.to_html();
        
        println!("Test case {}:", i + 1);
        println!("  Input: {}", input);
        println!("  HTML: {}", html.trim());
        println!();
    }
}
