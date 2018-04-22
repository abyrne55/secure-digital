//#include "mbed.h"
#include "aes.h"
#include "stdlib.h"
#include "time.h"
#include "rsa.h"
#include "stuff.h"

int debug = 1;

int main()
{
    
    if(debug) printf("\r\n AAA \r\n");
    
    srand(time(NULL));
    unsigned char temp[16];

    //STARTUP PROCEDURE, generating a "random" sym key
    //Security is decent enough in the sense that you would need to know
    //exact time of start up according to the cmaera's time
    for (int ii = 0; ii < 16; ii++)
        temp[ii] = rand() % 256;
    
    if(debug) printf("\r\n BBB --- SYMKEY GEN'D: \r\n");
    if(debug){for(int ii = 0; ii < 16; ii++) printf("%x ", temp[ii]); printf("\r\n");}
    
    const unsigned char *symKey = &temp;

    //AES_CBC context
    mbedtls_aes_context AESCtx = {.nr = 10U, .rk = roundKey, .buf = NULL};
    //Initializing AES context
    mbedtls_aes_init(&AESCtx);
    if(mbedtls_aes_setkey_enc(&AESCtx, symKey,128)) {printf("\r\n setKey failed\r\n"); return 1;}
    if(mbedtls_aes_self_test(0)){printf("\r\n self test failed \r\n"); return 1;}

    //test for encryption setup failed, stop program
    if (mbedtls_aes_self_test(0)) return 1; 

    if(debug) printf("\r\n CCC initialzed, tested and set to enc mode \r\n");

    //ASSUME WE HAVE A DATA STREAM IN AND WE KNOW THE SIZE OF THE IMAGE IN ADVANCE
    int dataSize = 2560;
    unsigned char dataIn[dataSize];
    unsigned char dataOut[dataSize];
    unsigned char check[dataSize];
    //GNEREATING RANDOM "IAMGE"
    for (int ii = 0; ii < dataSize; ii++)
        dataIn[ii] = rand() % 256; //generates a random byte for dummer image
    
    //ECB ENCRYPTION WITH SYM KEY
    unsigned char iv1[16] = {0xD1,0x87,0xCF,0x25,0x95,0xCF,0xBF,0x2B,0xE6,0xCA,0xDE,0x52,0xB3,0x77,0x29,0x1E};
    unsigned char iv2[16] = {0xD1,0x87,0xCF,0x25,0x95,0xCF,0xBF,0x2B,0xE6,0xCA,0xDE,0x52,0xB3,0x77,0x29,0x1E};
    
    if(!debug) printf("\r\n IMG ORIGINAL --- ENCRYPED WITH iv1: \r\n");
    if(!debug){
        for(int ii = 0; ii < dataSize; ii++) {
            printf("%x ", dataIn[ii]); 
            if(ii%16 == 0) printf("\r\n");
            }
        }
    
    mbedtls_aes_crypt_cbc(&AESCtx, MBEDTLS_AES_ENCRYPT, dataSize, iv1, dataIn, dataOut); //ECB for the image encryption
    //mbedtls_aes_crypt_cbc(&AESCtx, MBEDTLS_AES_ENCRYPT, dataSize, iv1, dataIn, dataOut); //ECB for the image encryption
    
    if(!debug) printf("\r\n DDD --- ENCRYPED WITH iv1: \r\n");
    if(!debug){
        for(int ii = 0; ii < dataSize; ii++) {
            printf("%x ", dataOut[ii]); 
            if(ii%16 == 0) printf("\r\n");
            }
        }
    if(debug) printf("\r\n EEE --- ENCRYPED WITH iv1: \r\n");

    //STARTING DECRYPTION
    if(mbedtls_aes_setkey_dec(&AESCtx, symKey,128)) {printf("\r\n setKey failed PART 2\r\n"); return 1;}
    if(debug) printf("\r\n FFF --- DECRYPED MODE SET \r\n");
    mbedtls_aes_crypt_cbc(&AESCtx, MBEDTLS_AES_DECRYPT, dataSize, iv2, dataOut, check);
    if(debug) printf("\r\n GGG --- IMAGE DECRYPTED WITH iv2: \r\n");
    
    if(!debug) printf("\r\n IMG2 --- DECRYPRED WITH iv2: \r\n");
    if(!debug){
        for(int ii = 0; ii < dataSize; ii++) {
            printf("%x ", check[ii]); 
            if(ii%16 == 0) printf("\r\n");
            }
        }
    
    for(int ii = 1; ii < dataSize; ii++) if(dataIn[ii] != check[ii]) { printf("DECRYPTIION ERROR at byte %d \r\n", ii); return 1;}
    
    
    
    if(debug) printf("\r\n HHH --- DECRYPTION TEST PASSED \r\n");

    unsigned char encryptedSymKey[16];

    //RSA Encrypt the sym key with the rsa pub key above
    mbedtls_rsa_context RSACtx;
//    
//    mbedtls_rsa_import(&RSACtx, &N, NULL, NULL, &D, &E);
//    mbedtls_rsa_complete(&RSACtx);
//    mbedtls_rsa_init(&RSACtx, MBEDTLS_RSA_PKCS_V15, NULL);
//    
//    if(debug) printf("\r\n III --- RSA CONTEXT WORKING \r\n");

//    mbedtls_rsa_pkcs1_encrypt(&RSACtx, NULL, NULL, MBEDTLS_RSA_PUBLIC, 16, symKey, encryptedSymKey);
//
//    printf("encrpyted key %s \r\n", encryptedSymKey);
//
//    //Assume encrpyted key written to memory somewhere
//    //TODO: build a table of keys and photo ranges that is loaded
    return 0;
}