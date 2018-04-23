#include "mbed.h"
#include "aes.h"
#include "stdlib.h"
#include "time.h"
#include "rsa.h"
#include "stuff.h"
#include "FreescaleIAP.h"


int main()
{


    //On start, read the address for where the tabel ends to know where to put new entry
    int addrEndTable = flash_size() - 10 * SECTOR_SIZE;           //Write in last sector, sector size = 4096 bytes
    int* endTable = (int*)addrEndTable;

    //program_flash saves in LITTLE ENDIAN, BE CAREFUL WHEN READING BYTE ARRAY
    //if our current storage address is not under the addrEndTable then reset the flash pointer
    if(endTable[0] < flash_size() - 10 * SECTOR_SIZE) {
        endTable = &addrEndTable;
        program_flash(addrEndTable, (char*)endTable ,4); //int where to write, char* what to write, int number of bytes
    }
    
    //if(debug) printf("\r\n data at address %x is %x \r\n", data, *data);

    unsigned char pixCount[1] = {0x00}; //incrememted per picture taken

    int* newEntryStart = (int*) endTable[0];
    
    char entry[36];

    if(DEBUG) printf("\r\n AAA \r\n");

    srand(time(NULL));
    unsigned char temp[16];
    unsigned char initVec[16];
    unsigned char initVecFlash[16];

    //STARTUP PROCEDURE, generating a "random" sym key
    //Security is decent enough in the sense that you would need to know
    //exact time of start up according to the cmaera's time
    //TODO: extend cbc to be longer if time allows
    for (int ii = 0; ii < 16; ii++)
        temp[ii] = rand() % 256;
    //initVec
    for (int ii = 0; ii < 16; ii++)
        initVec[ii] = rand() % 256;
    //initVecFlash, this one will be saved for the flash
    for (int ii = 0; ii < 16; ii++)
        initVecFlash[ii] = initVec[ii];

    for(int ii = 0; ii < 16; ii++) {
        entry[ii] = temp[ii];
        entry[ii+16] = initVec[ii];
        if(ii < 4) entry[ii+32] = 0;    
    }
    
     program_flash((int) newEntryStart, entry, 36);


    if(DEBUG) printf("\r\n BBB --- SYMKEY GEN'D: \r\n");
    if(DEBUG) {
        for(int ii = 0; ii < 16; ii++) printf("%x ", temp[ii]);
        printf("\r\n");
    }

    const unsigned char *symKey = temp;

    //AES_CBC context
    mbedtls_aes_context AESCtx = {.nr = 10U, .rk = roundKey, .buf = NULL};
    //Initializing AES context
    mbedtls_aes_init(&AESCtx);
    if(mbedtls_aes_setkey_enc(&AESCtx, symKey,128)) {
        printf("\r\n setKey failed\r\n");
        return 1;
    }
    if(mbedtls_aes_self_test(0)) {
        printf("\r\n self test failed \r\n");
        return 1;
    }

    //test for encryption setup failed, stop program
    if (mbedtls_aes_self_test(0)) return 1;

    if(DEBUG) printf("\r\n CCC initialzed, tested and set to enc mode \r\n");

    //ASSUME WE HAVE A DATA STREAM IN AND WE KNOW THE SIZE OF THE IMAGE IN ADVANCE
    int dataSize = 2560;
    unsigned char dataIn[dataSize];
    unsigned char dataOut[dataSize];
    unsigned char check[dataSize];
    //GNEREATING RANDOM "IAMGE"
    for (int ii = 0; ii < dataSize; ii++)
        dataIn[ii] = rand() % 256; //generates a random byte for dummer image

    //ECB ENCRYPTION WITH SYM KEY


    if(DEBUG) printf("\r\n IMG ORIGINAL --- ENCRYPED WITH iv1: \r\n");
    if(DEBUGV) {
        for(int ii = 0; ii < dataSize; ii++) {
            printf("%x ", dataIn[ii]);
            if(ii%16 == 0) printf("\r\n");
        }
    }

    mbedtls_aes_crypt_cbc(&AESCtx, MBEDTLS_AES_ENCRYPT, dataSize, initVec, dataIn, dataOut); //ECB for the image encryption

    if(DEBUG) printf("\r\n DDD --- ENCRYPED WITH iv1: \r\n");
    if(DEBUGV) {
        for(int ii = 0; ii < dataSize; ii++) {
            printf("%x ", dataOut[ii]);
            if(ii%16 == 0) printf("\r\n");
        }
    }
    if(DEBUG) printf("\r\n EEE --- ENCRYPED WITH iv1: \r\n");

    //STARTING DECRYPTION
    if(mbedtls_aes_setkey_dec(&AESCtx, symKey,128)) {
        printf("\r\n setKey failed PART 2\r\n");
        return 1;
    }
    if(DEBUG) printf("\r\n FFF --- DECRYPED MODE SET \r\n");
    if(DEBUG) mbedtls_aes_crypt_cbc(&AESCtx, MBEDTLS_AES_DECRYPT, dataSize, initVecFlash, dataOut, check);
    if(DEBUG) printf("\r\n GGG --- IMAGE DECRYPTED WITH iv2: \r\n");

    if(DEBUG) printf("\r\n IMG2 --- DECRYPRED WITH iv2: \r\n");
    if(DEBUGV) {
        for(int ii = 0; ii < dataSize; ii++) {
            printf("%x ", check[ii]);
            if(ii%16 == 0) printf("\r\n");
        }
    }

    if(DEBUGV) for(int ii = 1; ii < dataSize; ii++) if(dataIn[ii] != check[ii]) {
                printf("DECRYPTIION ERROR at byte %d \r\n", ii);
                return 1;
            }


    if(DEBUGV) printf("\r\n HHH --- DECRYPTION TEST PASSED \r\n");

//    unsigned char encryptedSymKey[16];
//
//    //RSA Encrypt the sym key with the rsa pub key above
//    mbedtls_rsa_context RSACtx;
//    mbedtls_mpi N, E, D;
//    
//    mbedtls_mpi_init(&N);
//    mbedtls_mpi_init(&D);
//    mbedtls_mpi_init(&E);
//    
//    if(DEBUG) printf("\r\n JJJ --- MPIS INITIALIZED \r\n");
//    
//    mbedtls_mpi_read_binary(&N, NArray, 128); //read in hex array for N
//    mbedtls_mpi_read_binary(&E, EArray, 1);
//    mbedtls_mpi_read_binary(&D, DArray, 128);
//    
//    if(DEBUG) printf("\r\n KKK --- PUBLIC PARAMS READ IN \r\n");
//
//    //RSA Context and parameter initialization
//    mbedtls_rsa_import_raw(&RSACtx, NArray, 128, NULL, 0, NULL, 0, DArray, 128, EArray,1); //ctx, N, P, Q, D, E
//    
//    if(DEBUG) printf("\r\n LLL --- CONTEXT READY \r\n");
//    mbedtls_rsa_complete(&RSACtx);
//    mbedtls_rsa_init(&RSACtx, MBEDTLS_RSA_PKCS_V15, NULL);
//
//    if(DEBUG) printf("\r\n III --- RSA CONTEXT WORKING \r\n");
//
//    mbedtls_rsa_pkcs1_encrypt(&RSACtx, NULL, NULL, MBEDTLS_RSA_PUBLIC, 16, symKey, encryptedSymKey);
//
//    printf("encrpyted key %s \r\n", encryptedSymKey);
    printf("\r\n END OF PROGRAM \r\n");
    return 0;
}