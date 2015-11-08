#include "stdafx.h"

std::string xor(const std::string& value,const std::string& key){
	std::string retval(value);

	for(unsigned int i=0, j=0; i < value.length(); i++,j++) {
		if(j==key.length()) j=0;
        retval[i]=value[i]^key[j];
    }
    return retval;
}

void xor(BYTE buffer[],int bufsize, const BYTE key[], int keysize){    
	
    for(int i=0, j=0; i < bufsize; i++, j++)    {
        if(j==keysize) j=0;
        buffer[i] ^= key[j];        
	}

    /*
    for(BYTE *p=buffer, *pk=key; p-buffer<bufsize; p++, pk++)    {
        if(pk-key==keysize) pk=key;
        *p ^= *pk;        
	}
    */


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
	"\nXOR-encrypt[base16-encode|decode] stdin. Dmitry Unltd. ©2006\
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
    \n  ineractive use-type or paste your text,terminate by 'Enter' and ^Z:\
    \n      xor password > test.encrypted                 \
    \n  encrypt|base16encode|base16decode|decrypt->get original text:          \
    \n      echo foo|xor pwd|xor base16encode|xor base16decode|xor pwd \
	\n      xor foobarfoobar < xor.pch |xor base16encode|xor base16decode|xor foobarfoobar >xor.pch.fullcircle && echo. && sum xor.pch xor.pch.fullcircle \
    \n  \
    "
    );
    info();
} 

int from_hex(const std::string & from){
    std::stringstream parse(from);
    int result;
    parse >> std::hex >> result; //eventually calls strtol.c, which does the work
    return result; 
}


//this taken from CacheManager.cpp.
//also see AF_AUTHENT\AF_ISAPI\Utilities.cpp ByteToStr() etc.
void base16encode(const BYTE buffer_in[],int bufinsize, char buffer_out[], int bufoutsize){
    assert(bufoutsize==bufinsize*2 && "hex-representation of a byte is exactly 2 chars long!");
    
    for(int i=0,j=0; i < bufinsize; i++) 
        j += _sntprintf(buffer_out+j,bufoutsize-j,_T("%2.2x"),buffer_in[i]);
		//%2:minimum number of characters output 
	    //.2x: The precision specifies the minimum number of digits to be printed. If the number of digits in the argument is less than precision, the output value is padded on the left with zeros. The value is not truncated when the number of digits exceeds precision.
}

BYTE* base16decode2(const char base16buf[], int bufinsize, BYTE buffer_out[], int bufoutsize){
    assert(bufoutsize==bufinsize/2 && "hex-representation of a byte is exactly 2 chars long!");

    BYTE* pbOrig=buffer_out;
	for (int i = 0; i<bufinsize/2; i++){
        //char s[]={*base16buf++,*base16buf++,0};  //take 2 bytes of the string at a time + \0          
        char s[]={*base16buf++,*base16buf++};  //take 2 bytes of the string at a time
        sscanf(s, "%2hx", buffer_out++);         //treat 2 bytes as a hex #, put into pb and advance it
    }
    return pbOrig;
}


BYTE* base16decode(const char base16buf[], int bufinsize, BYTE buffer_out[], int bufoutsize){
	//TODO:take out terminating \n \f
    assert(bufinsize %2==0);//this will not work in Unix in interactive mode, because windows add LF/CR, Unix CR
    assert(bufoutsize==bufinsize/2 && "hex-representation of a byte is exactly 2 chars long!");

    BYTE* pbOrig=buffer_out;

	for (int i = 0; i<bufinsize/2; i++){
        char s[]={*base16buf++,*base16buf++,'\0'};    //take 2 bytes of the string at a time
		char* endc=0;
		long l=strtol(s,&endc,16);     //strtol returns 0 if no conversion can be performed
		buffer_out[i]=(BYTE)l;         //treat 2 bytes as a hex #, put into pb and advance it
    }
    return pbOrig;
}




int main(int argc, char* argv[]){
	//from_hex("BA");	from_hex("FF");

    if(argc<2 || strcmpi(argv[1],"-h")==0) {
        help(argc,argv);
        return 1;
    }

    BYTE* key=(BYTE*)argv[1];
    int keysize=strlen((char*)key);

    enum OP{do_xor,do_base16encode,do_base16decode} op=do_xor;

    if     (strcmpi(argv[1],"base16encode")==0) op=do_base16encode;
    else if(strcmpi(argv[1],"base16decode")==0) op=do_base16decode;

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
	fprintf(stderr,"\nreading from stdin(terminate by ^Z if using interactively):\n");

    int total_bytes_r=0,total_bytes_w=0;
    while(!feof(stdin)){
        
        BYTE buffer[4096]={0};
        size_t count_r=fread(buffer,sizeof(BYTE),sizeof(buffer),stdin);
        
        if(ferror(stdin)){
            perror("Read error");
            return 2;
        }

        fprintf(stderr,"\nread %d bytes\n",count_r);
        total_bytes_r+=count_r;

        size_t count_w=0;
        if(op==do_xor){
            xor(buffer,count_r,key,keysize);
            count_w=fwrite(buffer,sizeof(BYTE),count_r,stdout);
        }
        else if(op==do_base16encode){
            char base16buffer[sizeof(buffer)*2]={0};
            base16encode(buffer,count_r,base16buffer,count_r*2);
            count_w=fwrite(base16buffer,sizeof(char),count_r*2,stdout);
        }
        else if(op==do_base16decode){
            BYTE bytebuffer[sizeof(buffer)/2]={0};
            base16decode((const char*)buffer,count_r,bytebuffer,count_r/2);
            count_w=fwrite(bytebuffer,sizeof(BYTE),count_r/2,stdout);
        }       
        total_bytes_w+=count_w;
        fprintf(stderr,"\nwrote %d bytes",count_w);
    }

	fprintf(stderr,"\nread %d bytes total, wrote %d bytes total ",total_bytes_r,total_bytes_w);

	return 0;
}


#if 0
//strtol algorithm:
        for (;;) {      // exit in middle of loop 
                // convert c to value 
                if ( __ascii_isdigit((int)(unsigned char)c) )
                        digval = c - '0';
                else if ( __ascii_isalpha((int)(unsigned char)c) )
                        digval = __ascii_toupper(c) - 'A' + 10;
                else
                        break;
                if (digval >= (unsigned)ibase)
                        break;          // exit loop if bad digit found 

                // record the fact we have read one digit 
                flags |= FL_READDIGIT;

                // we now need to compute number = number * base + digval,
                //   but we need to know if overflow occured.  This requires
                //   a tricky pre-check. 

                if (number < maxval || (number == maxval &&
                (unsigned long)digval <= ULONG_MAX % ibase)) {
                        // we won't overflow, go ahead and multiply 
                        number = number * ibase + digval;
                }
                else {
                        // we would have overflowed -- set the overflow flag 
                        flags |= FL_OVERFLOW;
                }

                c = *p++;               // read next digit 
        }


//If the data was big-endian you would use the dec_uint32be function.
uint32_t dec_uint32le(const unsigned char *src){
	return src[0] | ((unsigned)src[1] << 8) | ((unsigned)src[2] << 16) | ((unsigned)src[3] << 24);
}

#endif

/*
BYTE* base16decode2(const char base16buf[], int bufinsize, BYTE buffer_out[], int bufoutsize){
    assert(bufinsize %2==0);//this will not work in Unix in interactive mode, because windows add LF/CR, Unix CR
    assert(bufoutsize==bufinsize/2 && "hex-representation of a byte is exactly 2 chars long!");

    BYTE* pbOrig=buffer_out;
	//ALL WRONG
	for (int i = 0; i<bufinsize/2; i++){
        int s[]={*base16buf++,*base16buf++};  //take 2 bytes of the string at a time
        BYTE b=((s[0]<<4)& 0xF0) |(s[1] & 0x0F);//RIGHT (more strict)
        BYTE b1=(s[0]<<4) |(s[1] & 0x0F);       //RIGHT
        BYTE b2=(s[1]<<4)|(s[0] & 16);          //WRONG!! F=1111=15 !!!
        buffer_out[i]=b;         //treat 2 bytes as a hex #, put into pb and advance it
    }
    return pbOrig;
}*/