#include "StdAfx.h"

void xor_encrypt(BYTE buffer[],int bufsize, const BYTE key[], int keysize){

    for(int i=0, j=0; i < bufsize; i++, j++)    {
        if(j==keysize) j=0;
        buffer[i] ^= key[j];
	}
}


void info(){
    fprintf(stderr,
	"\nINFO: \
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
	\n      phrase.\
	\n      http://64.233.161.104/search?q=cache:gyGb1oznqlEJ:celtickane.com/programming/crypt/xor.php+encryption+algorithm+runs+through+each+letter+of+the+unencrypted+phrase+and+XOR&hl=en&gl=us&ct=clnk&cd=1 \
	\n "
    );
}

void help(int argc, char* argv[]){
    fprintf(stderr,
	"\nXOR-encrypt[base16-encode|decode] stdin. Dmitry Unltd. �2006\
    \n  \
    \nUSAGE: \
    \n      xor 'password' or [base16encode|base16decode]  \
    \n      -if password is one of these: base16encode|base16decode the program encodes/decodes instead of xor\
    \n      -Data to be encrypted/decrypted/encoded/decoded is read from stdin and written to stdout \
    \n      -Diagnostic messages are written to stderr, redirect 2>/dev/null (Unix) or 2>NUL (Windows) if you don't want it\
    \n      -Binary files no problem. Key also could be binary, but then can't pass it as an arg \
    \n \
    \nEXAMPLES: \
    \n  encrypt:\
    \n      xor password <test.original > test.encrypted  \
    \n  decrypt:\
    \n      xor password <test.encrypted> test.decrypted  \
    \n  check(should be no diff):\
    \n      diff test.original test.decrypted             \
    \n  interactive use-type or paste your text,terminate by 'Enter' and ^D (Unix) or ^Z (Windows):\
    \n      xor password > test.encrypted                 \
    \n  encrypt|base16encode|base16decode|decrypt->get original text:          \
    \n      echo foo|xor pwd|xor base16encode|xor base16decode|xor pwd \
	\n      xor foobarfoobar < xor.cpp |xor base16encode|xor base16decode|xor foobarfoobar >xor.cpp.fullcircle && diff -s xor.cpp xor.cpp.fullcircle \
    \n  \
    "
    );
    info();
}

//this taken from CacheManager.cpp.
//also see AF_AUTHENT\AF_ISAPI\Utilities.cpp ByteToStr() etc.
void base16encode(const BYTE buffer_in[],int bufinsize, char buffer_out[], int bufoutsize){
    // bufoutsize must be bufinsize*2+1 to hold hex chars plus null terminator
    assert(bufoutsize==bufinsize*2+1 && "hex-representation of a byte is exactly 2 chars long!");

    static const char hex[] = "0123456789abcdef";
    for(int i=0; i < bufinsize; i++){
        buffer_out[i*2]   = hex[buffer_in[i] >> 4];
        buffer_out[i*2+1] = hex[buffer_in[i] & 0xf];
    }
    buffer_out[bufinsize*2] = '\0';
}


BYTE* base16decode(const char base16buf[], int bufinsize, BYTE buffer_out[], int bufoutsize){
    assert(bufinsize %2==0);
    assert(bufoutsize==bufinsize/2 && "hex-representation of a byte is exactly 2 chars long!");

    // Map ASCII character to nibble value; 0xff for invalid.
    static const BYTE unhex[256] = {
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // 0x00
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // 0x10
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // 0x20
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0xff,0xff,0xff,0xff,0xff,0xff, // 0x30 '0'-'9'
        0xff,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // 0x40 'A'-'F'
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // 0x50
        0xff,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // 0x60 'a'-'f'
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // 0x70
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // 0x80
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // 0x90
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // 0xa0
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // 0xb0
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // 0xc0
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // 0xd0
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // 0xe0
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  // 0xf0
    };

    for(int i = 0; i < bufinsize/2; i++){
        BYTE hi = unhex[(BYTE)base16buf[i*2]];
        BYTE lo = unhex[(BYTE)base16buf[i*2+1]];
        buffer_out[i] = (hi << 4) | lo;
    }
    return buffer_out;
}




int main(int argc, char* argv[]){
	//from_hex("BA");	from_hex("FF");

    if(argc<2 || strcasecmp(argv[1],"-h")==0) {
        help(argc,argv);
        return 1;
    }

    BYTE* key=(BYTE*)argv[1];
    int keysize=strlen((char*)key);

    enum OP{do_xor,do_base16encode,do_base16decode} op=do_xor;

    if     (strcasecmp(argv[1],"base16encode")==0) op=do_base16encode;
    else if(strcasecmp(argv[1],"base16decode")==0) op=do_base16decode;

    //change stream to binary mode. must do this on Windows but not required on Unix/Linux i believe, as they don't have
	//this distinction
	int prev_mode_stdin=_setmode(_fileno(stdin),_O_BINARY);
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