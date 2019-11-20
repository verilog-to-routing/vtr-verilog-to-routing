/* Copyright (c) 2004-2012 Sergey Lyubka <valenok@gmail.com>
 All rights reserved

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.

 This is a regular expression library that implements a subset of Perl RE.
 Please refer to http://slre.googlecode.com for detailed description.

 Supported syntax:
    ^        Match beginning of a buffer
    $        Match end of a buffer
    ()       Grouping and substring capturing
    [...]    Match any character from set
    [^...]   Match any character but ones from set
    \s       Match whitespace
    \S       Match non-whitespace
    \d       Match decimal digit
    \r       Match carriage return
    \n       Match newline
    +        Match one or more times (greedy)
    +?       Match one or more times (non-greedy)
    *        Match zero or more times (greedy)
    *?       Match zero or more times (non-greedy)
    ?        Match zero or once
    \xDD     Match byte with hex value 0xDD
    \meta    Match one of the meta character: ^$().[*+\?

 Match string buffer "buf" of length "buf_len" against "regexp", which should
 conform the syntax outlined above. "options" could be either 0 or
 SLRE_CASE_INSENSITIVE for case-insensitive match. If regular expression
 "regexp" contains brackets, slre_match() will capture the respective
 substring into the passed placeholder. Thus, each opening parenthesis
 should correspond to three arguments passed:
   placeholder_type, placeholder_size, placeholder_address

 Usage example: parsing HTTP request line.

  char method[10], uri[100];
  int http_version_minor, http_version_major;
  const char *error;
  const char *request = " \tGET /index.html HTTP/1.0\r\n\r\n";

  error = slre_match(0, "^\\s*(GET|POST)\\s+(\\S+)\\s+HTTP/(\\d)\\.(\\d)",
                     request, strlen(request),
                     SLRE_STRING,  sizeof(method), method,
                     SLRE_STRING, sizeof(uri), uri,
                     SLRE_INT,sizeof(http_version_major),&http_version_major,
                     SLRE_INT,sizeof(http_version_minor),&http_version_minor);
 
  if (error != NULL) {
    printf("Error parsing HTTP request: %s\n", error);
  } else {
    printf("Requested URI: %s\n", uri);
  }


 Return:
   NULL: string matched and all captures successfully made
   non-NULL: in this case, the return value is an error string */

#ifndef SLRE_H
#define SLRE_H

enum slre_option {SLRE_CASE_INSENSITIVE = 1};
enum slre_capture {SLRE_STRING, SLRE_INT, SLRE_FLOAT};
const char *slre_match(enum slre_option options, const char *regexp,
                       const char *buf, int buf_len, ...);

#endif /* SLRE_H */
