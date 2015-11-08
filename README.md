# xor
Automatically exported from code.google.com/p/xore

```
This is a Windows program demonstrating xor encryption. 
It should be easy to port to other OSs, as there's not much if anything Windows-specific.
(I however did not try to cross-compile it)
Binary file is in Releases
c:\>xor

XOR-encrypt[base16-encode|decode] stdin. Dmitry Unltd. ⌐2006

USAGE:
      xor 'password' or [base16encode|base16decode]
      -if password is one of these: base16encode|base16decode the program encodes/decodes instead of xor
      -Data to be encrypted/decrypted/encoded/decoded is read from stdin and written to stdout
      -Diagnostic messages are written to stderr, redirect 2>/dev/null (Unix) or 2>NUL (Windows) if you don't want them
      -Binary files no problem. Key also could be binary, but then can't pass it as an arg

EXAMPLES:
  encrypt:
      xor password <test.original > test.encrypted
  decrypt:
      xor password <test.encrypted> test.decrypted
  check(should be no diff):
      diff test.original test.decrypted
  interactive use-type or paste your text,terminate by 'Enter' and ^Z:
      xor password > test.encrypted
  encrypt|base16encode|base16decode|decrypt->get original text:
      echo foo|xor pwd|xor base16encode|xor base16decode|xor pwd
      xor foobarfoobar < xor.pch |xor base16encode|xor base16decode|xor foobarfoobar >xor.pch.fullcircle && echo. && sum xor.pch xor.pch.fullcircle

INFO:
      XOR-encryption is very simple and quite strong. Search Google for more on XOR encryption.
      The encryption algorithm runs through each letter of the unencrypted phrase and XOR's it
      with one letter of the key. For example, if the unencrypted phrase was
      STARS, and the key was ABC, the encryption algorithm would go something like
      this: (S XOR A)(T XOR B)(A XOR C)(R XOR A)(S XOR B). XOR only works with two
      single letters at a time, which is why the algorithm needs to split both the
      phrase and the key letter by letter. Because of the nature of the algorithm,
      the length of the encrypted phrase is the same length as the unencrypted
      phrase.The beauty of XOR encryption comes in its decryption. The algorithm
      for encryption is the SAME as the one for decryption. For decryption, the
      key is XOR'ed against the encrypted phrase, and the result is the decrypted
      phrase.
```      
