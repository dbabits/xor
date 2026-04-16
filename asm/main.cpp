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
        "\nXOR-encrypt[base16-encode|decode] stdin. Dmitry Unltd. 2006 (ARM64 asm build)\
        \nUSAGE: \
        \n      xor_asm 'password' or [base16encode|base16decode]  \
        \n      -Data to be encrypted/decrypted/encoded/decoded is read from stdin and written to stdout \
        \n      -Diagnostic messages are written to stderr, redirect 2>/dev/null (Unix) if you don't want them\
        \n"
        );
        return 1;
    }

    BYTE* key=(BYTE*)argv[1];
    int keysize=strlen((char*)key);

    enum OP{do_xor,do_base16encode,do_base16decode} op=do_xor;

    if     (strcasecmp(argv[1],"base16encode")==0) op=do_base16encode;
    else if(strcasecmp(argv[1],"base16decode")==0) op=do_base16decode;

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
