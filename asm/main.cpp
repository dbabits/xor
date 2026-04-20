#include "../StdAfx.h"

// ARM64 assembly implementations declared here; defined in xor_asm.s
extern "C" {
    void xor_encrypt(BYTE buffer[], int bufsize, const BYTE key[], int keysize);
    void base16encode(const BYTE buffer_in[], int bufinsize, char buffer_out[], int bufoutsize);
    BYTE* base16decode(const char base16buf[], int bufinsize, BYTE buffer_out[], int bufoutsize);
}

int main(int argc, char* argv[]){

    if(argc<2 || strcasecmp(argv[1],"-h")==0) {
        fprintf(stderr,
        "\nXOR-encrypt[base16-encode|decode] stdin. Dmitry Unltd. \xa9""2006 (ARM64 NEON build)\
    \n  \
    \nUSAGE: \
    \n      xor_asm 'password' or [base16encode|base16decode]  \
    \n      -if password is one of these: base16encode|base16decode the program encodes/decodes instead of xor\
    \n      -Data to be encrypted/decrypted/encoded/decoded is read from stdin and written to stdout \
    \n      -Diagnostic messages are written to stderr, redirect 2>/dev/null (Unix) or 2>NUL (Windows) if you don't want it\
    \n      -Binary files no problem. Key also could be binary, but then can't pass it as an arg \
    \n \
    \nEXAMPLES: \
    \n  encrypt:\
    \n      xor_asm password <test.original > test.encrypted  \
    \n  decrypt:\
    \n      xor_asm password <test.encrypted> test.decrypted  \
    \n  check(should be no diff):\
    \n      diff test.original test.decrypted             \
    \n  interactive use-type or paste your text,terminate by 'Enter' and ^D (Unix) or ^Z (Windows):\
    \n      xor_asm password > test.encrypted                 \
    \n  encrypt|base16encode|base16decode|decrypt->get original text:          \
    \n      echo foo|xor_asm pwd|xor_asm base16encode|xor_asm base16decode|xor_asm pwd \
    \n      xor_asm foobarfoobar < xor.cpp |xor_asm base16encode|xor_asm base16decode|xor_asm foobarfoobar >xor.cpp.fullcircle && diff -s xor.cpp xor.cpp.fullcircle \
    \n  \
    \nINFO: \
    \n      XOR-encryption is very simple and quite strong. Search Google for more on XOR encryption. \
    \n      The encryption algorithm runs through each letter of the unencrypted phrase and XOR's it \
    \n      with one letter of the key. For example, if the unencrypted phrase was  \
    \n      STARS, and the key was ABC, the encryption algorithm would go something like    \
    \n      this: (S XOR A)(T XOR B)(A XOR C)(R XOR A)(S XOR B). XOR only works with two    \
    \n      single letters at a time, which is why the algorithm needs to split both the    \
    \n      phrase and the key letter by letter. Because of the nature of the algorithm,    \
    \n      the length of the encrypted phrase is the same length as the unencrypted        \
    \n      phrase.The beauty of XOR encryption comes in its decryption. The algorithm      \
    \n      for encryption is the SAME as the one for decryption. For decryption, the       \
    \n      key is XOR'ed against the encrypted phrase, and the result is the decrypted     \
    \n      phrase. \
    \n \
    \n      This build uses ARM64 NEON SIMD instructions (Raspberry Pi 5 / AArch64). \
    \n      Performance comparisons (NEON vs C vs scalar ASM): \
    \n      https://github.com/dbabits/xor \
    \n "
        );
        return 1;
    }

    BYTE* key=(BYTE*)argv[1];
    int keysize=strlen((char*)key);

    enum OP{do_xor,do_base16encode,do_base16decode} op=do_xor;

    if     (strcasecmp(argv[1],"base16encode")==0) op=do_base16encode;
    else if(strcasecmp(argv[1],"base16decode")==0) op=do_base16decode;
    else if(keysize==0) {
        fprintf(stderr,"\nError: XOR key cannot be empty\n");
        return 1;
    }

    int prev_mode_stdin =_setmode(_fileno(stdin), _O_BINARY);
    int prev_mode_stdout=_setmode(_fileno(stdout),_O_BINARY);

    if(prev_mode_stdin==-1) {
        perror("cannot _setmode(_fileno(stdin),_O_BINARY)");
        return 1;
    }
    if(prev_mode_stdout==-1) {
        perror("cannot _setmode(_fileno(stdout),_O_BINARY)");
        return 1;
    }

    fprintf(stderr,"\nold stdin mode=%d,old stdout mode=%d, _O_TEXT=%d, new mode=_O_BINARY",prev_mode_stdin,prev_mode_stdout,_O_TEXT);
    fprintf(stderr,"\nkey=%s,keysize=%d",key,keysize);
    fprintf(stderr,"\nreading from stdin(terminate by ^D (Unix) or ^Z (Windows) if using interactively):\n");

    int total_bytes_r=0,total_bytes_w=0;
    BYTE buffer[65536];
    size_t count_r;
    while((count_r=fread(buffer,sizeof(BYTE),sizeof(buffer),stdin)) > 0){

        fprintf(stderr,"\nread %zu bytes\n",count_r);
        total_bytes_r+=count_r;

        size_t count_w=0;
        if(op==do_xor){
            xor_encrypt(buffer,count_r,key,keysize);
            count_w=fwrite(buffer,sizeof(BYTE),count_r,stdout);
        }
        else if(op==do_base16encode){
            char base16buffer[sizeof(buffer)*2+1];
            base16encode(buffer,count_r,base16buffer,count_r*2+1);
            count_w=fwrite(base16buffer,sizeof(char),count_r*2,stdout);
        }
        else if(op==do_base16decode){
            if(count_r % 2 != 0) {
                fprintf(stderr,"\nError: base16decode input length must be even (got %zu bytes)\n",count_r);
                return 1;
            }
            BYTE bytebuffer[sizeof(buffer)/2];
            base16decode((const char*)buffer,count_r,bytebuffer,count_r/2);
            count_w=fwrite(bytebuffer,sizeof(BYTE),count_r/2,stdout);
        }
        total_bytes_w+=count_w;
        fprintf(stderr,"\nwrote %zu bytes",count_w);
    }

    if(ferror(stdin)){
        perror("Read error");
        return 2;
    }

    fprintf(stderr,"\nread %d bytes total, wrote %d bytes total\n",total_bytes_r,total_bytes_w);

    return 0;
}
