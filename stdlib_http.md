# HTTP Module

Real HTTP client powered by libcurl. Supports GET, POST, PUT, DELETE, HEAD, URL encoding, and status code queries.

## Functions

| Function | Description |
|----------|-------------|
| `http.get(url)` | HTTP GET request, returns response body as string |
| `http.post(url, body [, content_type])` | HTTP POST request, returns response body |
| `http.put(url, body [, content_type])` | HTTP PUT request, returns response body |
| `http.delete(url)` | HTTP DELETE request, returns response body |
| `http.head(url)` | HTTP HEAD request, returns headers as array of strings |
| `http.status_code(url)` | Returns HTTP status code as integer |
| `http.url_encode(str)` | URL-encode a string |
| `http.url_decode(str)` | URL-decode a string |

## Examples

### Basic GET
```rads
let response = http.get("https://httpbin.org/get")
print(response)
```

### POST with JSON
```rads
let data = json.stringify({"name": "RADS", "version": "0.0.12"})
let result = http.post("https://httpbin.org/post", data, "application/json")
print(result)
```

### PUT request
```rads
let updated = http.put("https://api.example.com/resource/1", "new data", "text/plain")
print(updated)
```

### DELETE request
```rads
let result = http.delete("https://api.example.com/resource/1")
print(result)
```

### Check status code
```rads
let code = http.status_code("https://httpbin.org/status/404")
if code == 404 {
    print("Not found!")
}
```

### Inspect headers
```rads
let headers = http.head("https://example.com")
for h in headers {
    print(h)
}
```

### URL encoding
```rads
let encoded = http.url_encode("hello world & friends")
print(encoded)  // "hello%20world%20%26%20friends"

let decoded = http.url_decode("hello%20world")
print(decoded)  // "hello world"
```

## Python Equivalence

| RADS | Python (requests) |
|------|-------------------|
| `http.get(url)` | `requests.get(url).text` |
| `http.post(url, body, ct)` | `requests.post(url, data=body, headers={"Content-Type": ct}).text` |
| `http.put(url, body, ct)` | `requests.put(url, data=body).text` |
| `http.delete(url)` | `requests.delete(url).text` |
| `http.head(url)` | `requests.head(url).headers` |
| `http.status_code(url)` | `requests.get(url).status_code` |
| `http.url_encode(s)` | `urllib.parse.quote(s)` |
| `http.url_decode(s)` | `urllib.parse.unquote(s)` |

## Features

- **Follows redirects** (up to 10)
- **30-second timeout** on all requests
- **Custom User-Agent**: `RADS/0.0.12`
- **Custom Content-Type** on POST/PUT
- **Error reporting**: returns error string on failure
- **Lightweight**: single libcurl dependency, works on all platforms
