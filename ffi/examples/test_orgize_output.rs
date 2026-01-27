use orgize::Org;

fn main() {
    let input = "* Heading\n\nTest link [[https://example.com][Example Site]]";
    let org = Org::parse(input);
    let html = org.to_html();
    
    println!("Full HTML output from orgize:");
    println!("{}", html);
    println!("\n--- End of output ---\n");
    
    // Check what body_start finds
    let body_start = html.find("<body>").and_then(|pos| {
        html[pos + 6..].find('>').map(|end| pos + 6 + end + 1)
    });
    
    let body_end = html.find("</body>");
    
    println!("body_start: {:?}", body_start);
    println!("body_end: {:?}", body_end);
    
    if let (Some(start), Some(end)) = (body_start, body_end) {
        println!("Extracted body content:");
        println!("{}", &html[start..end]);
    }
}
